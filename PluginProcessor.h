/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
*/
class AudioProcessor2AudioProcessor  : public juce::AudioProcessor, public juce::AudioProcessorValueTreeState::Listener
{
public:
    //==============================================================================
    AudioProcessor2AudioProcessor();
    ~AudioProcessor2AudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState params;
    void openFile();
    void loadFile(juce::File& file);

    float mGain{ 0.5 };

    void updateFilter();


    juce::AudioProcessorValueTreeState tree;

    juce::AudioProcessorValueTreeState apvts;

    int menuChoice;
    float thresh = 0.0f;
    float mix = 0.0f;


private:
    //==============================================================================
    juce::AudioFormatManager audioFormatManager;
    juce::File audioFile;

    juce::AudioTransportSource transportSource;
    std::unique_ptr<juce::AudioFormatReaderSource> audioFileSource;

    juce::dsp::ProcessorDuplicator <juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients <float>> lowPassFilter;

    float lastSampleRate;

    static constexpr auto effectDelaySamples = 192000;
    juce::dsp::DelayLine<float> delay{ effectDelaySamples };
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> linear{ effectDelaySamples };
    juce::dsp::DryWetMixer<float> mixer;

    std::array<float, 2> delayValue{ {} };
    std::array<float, 2> lastDelayOutput;
    std::array<juce::LinearSmoothedValue<float>, 2> delayFeedbackVolume;

    void parameterChanged(const juce::String& parameterID, float newValue) override;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameters();
    

    //juce::AudioParameterFloat* gain;
    //juce::AudioParameterFloat* mS;
    //float nextRad = 0;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioProcessor2AudioProcessor)
};

//==========================================================================================================

/*class StatePresets
{
public:
    StatePresets(juce::AudioProcessor& proc, const juce::String& presetFileLocation);
    ~StatePresets();

    void savePreset(const juce::String& presetName); // preset already exists? confirm overwrite
    void loadPreset(int presetID);
    void deletePreset();

    juce::StringArray getPresetNames() const;
    int getNumPresets() const;
    int getCurrentPresetId() const;

private:
    juce::AudioProcessor& pluginProcessor;
    juce::XmlElement presetXml{ "PRESETS" }; // local, in-plugin representation
    juce::File presetFile;                  // on-disk representation
    int currentPresetID{ 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StatePresets);
};

void createFileIfNonExistant(const juce::File& file);
void parseFileToXmlElement(const juce::File& file, juce::XmlElement& xml);
void writeXmlElementToFile(const juce::XmlElement& xml, juce::File& file);
juce::String getNextAvailablePresetID(const juce::XmlElement& presetXml);

void saveStateToXml(const juce::AudioProcessor& processor, juce::XmlElement& xml);
void loadStateFromXml(const juce::XmlElement& xml, juce::AudioProcessor& processor);

*/