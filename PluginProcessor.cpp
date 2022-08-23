/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AudioProcessor2AudioProcessor::AudioProcessor2AudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ), params(*this, nullptr, "Parameters", std::make_unique<juce::AudioParameterBool>("Play", "Play", false))
                        , tree(*this, nullptr), lowPassFilter(juce::dsp::IIR::Coefficients<float>::makeLowPass(44100, 20000.0f, 0.1))
                        , apvts(*this, nullptr, "Parameters", createParameters())
#endif
{
    audioFormatManager.registerBasicFormats();
    audioFile.getSpecialLocation(juce::File::SpecialLocationType::userHomeDirectory);

    juce::NormalisableRange<float> cutoffRange(20.0f, 20000.0f);
    juce::NormalisableRange<float> resRange(0.1f, 1.0f);

    tree.createAndAddParameter("cutoff", "Cutoff", "cutoff", cutoffRange, 100.0f, nullptr, nullptr);
    tree.createAndAddParameter("resonance", "Resonance", "resonance", resRange, 0.1f, nullptr, nullptr);

    apvts.addParameterListener("RATE", this);
    apvts.addParameterListener("FEEDBACK", this);
    apvts.addParameterListener("MIX", this);

    //addParameter(gain = new juce::AudioParameterFloat("gain", "Gain", 0.0f, 1.0f, 0.0f));
    //addParameter(mS = new juce::AudioParameterFloat("mS", "MilliSeconds", 10.0f, 5000.0f, 500.0f));

}

AudioProcessor2AudioProcessor::~AudioProcessor2AudioProcessor()
{
    apvts.removeParameterListener("RATE", this);
    apvts.removeParameterListener("FEEDBACK", this);
    apvts.removeParameterListener("MIX", this);
}

//==============================================================================
const juce::String AudioProcessor2AudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AudioProcessor2AudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool AudioProcessor2AudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool AudioProcessor2AudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double AudioProcessor2AudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int AudioProcessor2AudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int AudioProcessor2AudioProcessor::getCurrentProgram()
{
    return 0;
}

void AudioProcessor2AudioProcessor::setCurrentProgram (int index)
{
}

const juce::String AudioProcessor2AudioProcessor::getProgramName (int index)
{
    return {};
}

void AudioProcessor2AudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

void AudioProcessor2AudioProcessor::openFile()                                  //******************************************
{
    juce::FileChooser fileChooser{ "Choose an audio file", audioFile, "" };

    if (fileChooser.browseForFileToOpen())
    {
        juce::File choice = fileChooser.getResult();
        loadFile(choice);
    }
}

void AudioProcessor2AudioProcessor::loadFile(juce::File& file)
{
    transportSource.stop();
    transportSource.setSource(nullptr);
    audioFileSource = nullptr;

    juce::AudioFormatReader* reader = audioFormatManager.createReaderFor(file);

    if (reader != nullptr)
    {
        audioFileSource = std::make_unique<juce::AudioFormatReaderSource>(reader, true);
        transportSource.setSource(audioFileSource.get());
    }
}



//==============================================================================
void AudioProcessor2AudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..                                        //***************************************
    transportSource.prepareToPlay(samplesPerBlock, sampleRate);

    lastSampleRate = sampleRate;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = getTotalNumOutputChannels();


    lowPassFilter.prepare(spec);
    lowPassFilter.reset();

    //juce::dsp::ProcessSpec spec;
    spec.maximumBlockSize = samplesPerBlock;
    spec.sampleRate = sampleRate;
    spec.numChannels = 2;

    delay.prepare(spec);
    linear.prepare(spec);
    mixer.prepare(spec);

    for (auto& volume : delayFeedbackVolume)
        volume.reset(spec.sampleRate, 0.05);

    linear.reset();
    std::fill(lastDelayOutput.begin(), lastDelayOutput.end(), 0.0f);

}

void AudioProcessor2AudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
    transportSource.releaseResources();
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool AudioProcessor2AudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void AudioProcessor2AudioProcessor::updateFilter()
{
    float freq = *tree.getRawParameterValue("cutoff");
    float res = *tree.getRawParameterValue("resonance");

    *lowPassFilter.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(lastSampleRate, freq, res);
}

void AudioProcessor2AudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

   
        for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
            buffer.clear(i, 0, buffer.getNumSamples());

        const auto numChannels = juce::jmax(totalNumInputChannels, totalNumOutputChannels);

        auto audioBlock = juce::dsp::AudioBlock<float>(buffer).getSubsetChannelBlock(0, (size_t)numChannels);
        auto context = juce::dsp::ProcessContextReplacing<float>(audioBlock);
        const auto& input = context.getInputBlock();
        const auto& output = context.getOutputBlock();


    mixer.pushDrySamples(input);

    if (params.getParameterAsValue("Play") == true)
    {
        if (audioFileSource != nullptr)
        {
            transportSource.start();
        }
    }
    else
    {
        transportSource.stop();
        transportSource.setPosition(0.0);
    }


    transportSource.getNextAudioBlock(juce::AudioSourceChannelInfo(buffer));
    
    juce::dsp::AudioBlock <float> block(buffer);
    updateFilter();
    lowPassFilter.process(juce::dsp::ProcessContextReplacing<float>(block));
    
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer (channel);

        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            channelData[sample] = channelData[sample] * juce::Decibels::decibelsToGain(mGain);
        }


        // ..do something to the data...
    }
    for (size_t channel = 0; channel < numChannels; ++channel)
    {
        auto* samplesIn = input.getChannelPointer(channel);
        auto* samplesOut = output.getChannelPointer(channel);

        for (size_t sample = 0; sample < input.getNumSamples(); ++sample)
        {
            auto input = samplesIn[sample] - lastDelayOutput[channel];
            auto delayAmount = delayValue[channel];

            linear.pushSample(int(channel), input);
            linear.setDelay((float)delayAmount);
            samplesOut[sample] = linear.popSample((int)channel);

            lastDelayOutput[channel] = samplesOut[sample] * delayFeedbackVolume[channel].getNextValue();
        }
    }

    mixer.mixWetSamples(output);

    /*auto* channeldataL = buffer.getWritePointer(0);
    auto* channeldataR = buffer.getWritePointer(1);

    float gSlider = gain->get();

    for (int i = 0; i < buffer.getNumSamples(); i++)
    {
        auto inputL = channeldataL[i];
        auto inputR = channeldataR[i];

        inputL = inputL * juce::Decibels::decibelsToGain(gSlider);
        inputR = inputR * juce::Decibels::decibelsToGain(gSlider);

        channeldataL[i] = inputL;
        channeldataR[i] = inputR;
    }*/

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);

        for (int i = 0; i < buffer.getNumSamples(); ++i) {

            auto input = channelData[i];
            auto cleanOut = channelData[i];

            if (menuChoice == 1)
                //Hard Clipping
            {
                if (input > thresh)
                {
                    input = thresh;
                }
                else if (input < -thresh)
                {
                    input = -thresh;
                }
                else
                {
                    input = input;
                }
            }
            if (menuChoice == 2)
                //Soft Clipping Exp
            {
                if (input > thresh)
                {
                    input = 1.0f - expf(-input);
                }
                else
                {
                    input = -1.0f + expf(input);
                }
            }
            if (menuChoice == 3)
                //Half-Wave Rectifier
            {
                if (input > thresh)
                {
                    input = input;
                }
                else
                {
                    input = 0;
                }
            }
            channelData[i] = ((1 - mix) * cleanOut) + (mix * input);
        }
    }
}

//==============================================================================
bool AudioProcessor2AudioProcessor::hasEditor() const
{   
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* AudioProcessor2AudioProcessor::createEditor()
{
    if (juce::TopLevelWindow::getNumTopLevelWindows() == 1)
    {
        juce::TopLevelWindow* w = juce::TopLevelWindow::getTopLevelWindow(0);
        w->setUsingNativeTitleBar(true);
    }
    return new AudioProcessor2AudioProcessorEditor (*this);
    
}

//==============================================================================
void AudioProcessor2AudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void AudioProcessor2AudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudioProcessor2AudioProcessor();
}

void AudioProcessor2AudioProcessor::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (parameterID == "RATE")
        std::fill(delayValue.begin(), delayValue.end(), newValue / 1000.0 * getSampleRate());

    if (parameterID == "MIX")
        mixer.setWetMixProportion(newValue);

    if (parameterID == "FEEDBACK")
    {
        const auto feedbackGain = juce::Decibels::decibelsToGain(newValue, -100.0f);

        for (auto& volume : delayFeedbackVolume)
            volume.setTargetValue(feedbackGain);
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout AudioProcessor2AudioProcessor::createParameters()
{
    juce::AudioProcessorValueTreeState::ParameterLayout params;

    using Range = juce::NormalisableRange<float>;

    params.add(std::make_unique<juce::AudioParameterFloat>("RATE", "Rate", 0.01f, 1000.0f, 0));
    params.add(std::make_unique<juce::AudioParameterFloat>("FEEDBACK", "Feedback", -100.0f, 0.0f, -100.0f));
    params.add(std::make_unique<juce::AudioParameterFloat>("MIX", "Mix", Range{ 0.0f, 1.0f, 0.01f }, 0.0f));

    return params;
}
//**************************************************************************************************************************

/*void saveStateToXml(const juce::AudioProcessor& proc, juce::XmlElement& xml)
{
    xml.removeAllAttributes(); // clear first

    for (const auto& param : proc.getParameters())
        if (auto* p = dynamic_cast<juce::AudioProcessorParameterWithID*> (param))
            xml.setAttribute(p->paramID, p->getValue()); // 0to1
}

void loadStateFromXml(const juce::XmlElement& xml, juce::AudioProcessor& proc)
{
    for (const auto& param : proc.getParameters())
        if (auto* p = dynamic_cast<juce::AudioProcessorParameterWithID*> (param))
            // if not in xml set current
            p->setValueNotifyingHost((float)xml.getDoubleAttribute(p->paramID, p->getValue()));
}

StateAB::StateAB(juce::AudioProcessor& p)
    : pluginProcessor{ p }
{
    copyAB();
}

void StateAB::toggleAB()
{
    juce::XmlElement temp{ "Temp" };
    saveStateToXml(pluginProcessor, temp); // current to temp
    loadStateFromXml(ab, pluginProcessor); // ab to current
    ab = temp;                              // temp to ab
}

void StateAB::copyAB()
{
    saveStateToXml(pluginProcessor, ab);
}

void createFileIfNonExistant(const juce::File& file)
{
    if (!file.exists())
        file.create();
    jassert(file.exists());
}

void parseFileToXmlElement(const juce::File& file, juce::XmlElement& xml)                  // what could go wrong here?
{
    std::unique_ptr<juce::XmlElement> parsed{ juce::XmlDocument::parse(file) };
    if (parsed)
        xml = *parsed;
}

void writeXmlElementToFile(const juce::XmlElement& xml, juce::File& file)
{
    createFileIfNonExistant(file);
    xml.writeToFile(file, "");         // "" is DTD (unused)
}

juce::String getNextAvailablePresetID(const juce::XmlElement& presetXml)
{
    int newPresetIDNumber = presetXml.getNumChildElements() + 1; // 1 indexed to match ComboBox
    return "preset" + static_cast<juce::String> (newPresetIDNumber);   // format: preset##
}


StatePresets::StatePresets(juce::AudioProcessor& proc, const juce::String& presetFileLocation)
    : pluginProcessor{ proc },
    presetFile{ juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                  .getChildFile(presetFileLocation) }
{
    parseFileToXmlElement(presetFile, presetXml);
}

StatePresets::~StatePresets()
{
    writeXmlElementToFile(presetXml, presetFile);
}

void StatePresets::savePreset(const juce::String& presetName)
{
    juce::String newPresetID = getNextAvailablePresetID(presetXml); // presetID format: "preset##"

    std::unique_ptr<juce::XmlElement> currentState{ new juce::XmlElement {newPresetID} };    // must be pointer as
    saveStateToXml(pluginProcessor, *currentState);                            // parent takes ownership
    currentState->setAttribute("presetName", presetName);

    presetXml.addChildElement(currentState.release());                         // will be deleted by parent element
}

void StatePresets::loadPreset(int presetID)
{
    if (1 <= presetID && presetID <= presetXml.getNumChildElements()) // 1 indexed to match ComboBox
    {
        juce::XmlElement loadThisChild{ *presetXml.getChildElement(presetID - 1) }; // (0 indexed method)
        loadStateFromXml(loadThisChild, pluginProcessor);
    }
    currentPresetID = presetID; // allow 0 for 'no preset selected' (?)
}

void StatePresets::deletePreset()
{
    juce::XmlElement* childToDelete{ presetXml.getChildElement(currentPresetID - 1) };
    if (childToDelete)
        presetXml.removeChildElement(childToDelete, true);
}

juce::StringArray StatePresets::getPresetNames() const
{
    juce::StringArray names;

    forEachXmlChildElement(presetXml, child)                                    // should avoid macro?
    {
        juce::String n = child->getStringAttribute("presetName");
        if (n == "")
            n = "(Unnamed preset)";
        names.add(n);
    }
    return names; // hopefully moves
}

int StatePresets::getNumPresets() const
{
    return presetXml.getNumChildElements();
}

int StatePresets::getCurrentPresetId() const
{
    return currentPresetID;
}

*/
