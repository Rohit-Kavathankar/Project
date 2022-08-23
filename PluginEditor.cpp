/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AudioProcessor2AudioProcessorEditor::AudioProcessor2AudioProcessorEditor (AudioProcessor2AudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.

    juce::Timer::startTimerHz(60);

    playButton.setToggleState(false, juce::NotificationType::dontSendNotification);
    playButton.setClickingTogglesState(true);
    //playButton.onClick = [this]() { audioProcessor.timerCallback(); };
    addAndMakeVisible(playButton);

    buttonState = std::make_unique <juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.params, "Play", playButton);

    openButton.onClick = [this]() { audioProcessor.openFile(); };
    addAndMakeVisible(openButton);

    mGainSlider.setSliderStyle(juce::Slider::SliderStyle::LinearHorizontal);
    mGainSlider.setTextBoxStyle(juce::Slider::NoTextBox, true, 50, 20);
    GainLabel.setText("Volume", juce::dontSendNotification);
    //GainLabel.attachToComponent(&mGainSlider, false);
    mGainSlider.setRange(-60.0f, 0.0f, 0.01f);
    mGainSlider.setValue(-20.0f);
    mGainSlider.addListener(this);
    addAndMakeVisible(mGainSlider);
    addAndMakeVisible(GainLabel);

        filterCutoffDial.setSliderStyle(juce::Slider::SliderStyle::LinearVertical);
        filterCutoffDial.setRange(20.0f, 20000.0f, 0.01f);
        filterCutoffDial.setValue(20000.0f);
        filterCutoffDial.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        filterCutoffDial.setPopupDisplayEnabled(true, true, this);
        Cutofflabel.setText("Cutoff", juce::dontSendNotification);
        //Cutofflabel.attachToComponent(&filterCutoffDial, true);
        addAndMakeVisible(Cutofflabel);
        addAndMakeVisible(&filterCutoffDial);

        filterResDial.setSliderStyle(juce::Slider::SliderStyle::LinearVertical);
        filterResDial.setRange(0.1f, 1.0f);
        filterResDial.setValue(2.0f);
        filterResDial.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        filterResDial.setPopupDisplayEnabled(true, true, this);
        Reslabel.setText("Res", juce::dontSendNotification);
        //Reslabel.attachToComponent(&filterResDial, true);
        addAndMakeVisible(Reslabel);
        addAndMakeVisible(&filterResDial);
    
    filterCutoffValue = new juce::AudioProcessorValueTreeState::SliderAttachment(audioProcessor.tree, "cutoff", filterCutoffDial);
    filterResValue = new juce::AudioProcessorValueTreeState::SliderAttachment(audioProcessor.tree, "resonance", filterResDial);
    //filterCutoffDial.setSkewFactorFromMidPoint(1000.0f);

    getLookAndFeel().setColour(juce::Slider::ColourIds::thumbColourId, juce::Colour::fromRGB(242, 202, 16));
    getLookAndFeel().setColour(juce::Slider::ColourIds::rotarySliderFillColourId, juce::Colour::fromRGB(115, 155, 184));
    getLookAndFeel().setColour(juce::Slider::ColourIds::rotarySliderOutlineColourId, juce::Colour::fromRGB(44, 53, 57));

    using SliderStyle = juce::Slider::SliderStyle;
    using Attachment = juce::SliderParameterAttachment;
    const auto boxWidth = 35;
    const auto boxHeight = 15;

        rateSlider.setSliderStyle(SliderStyle::RotaryVerticalDrag);
        rateSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, boxWidth, boxHeight);
        addAndMakeVisible(rateSlider);

        feedbackSlider.setSliderStyle(SliderStyle::RotaryVerticalDrag);
        feedbackSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, boxWidth, boxHeight);
        addAndMakeVisible(feedbackSlider);

        mixSlider.setSliderStyle(juce::Slider::SliderStyle::RotaryVerticalDrag);
        mixSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, boxWidth, boxHeight);
        addAndMakeVisible(mixSlider);

        rateLabel.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(rateLabel);
        feedbackLabel.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(feedbackLabel);
        mixLabel.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(mixLabel);
    
    //pluginTitle.setFont(currentFont.boldened().withHeight(60.0f));
    /*pluginTitle.setColour(juce::Label::ColourIds::textColourId, juce::Colours::white);
    pluginTitle.setSize(100, 50);
    addAndMakeVisible(pluginTitle);*/

    auto& apvts = audioProcessor.apvts;

    rateSliderAttachment = std::make_unique<Attachment>(*apvts.getParameter("RATE"), rateSlider);
    feedbackSliderAttachment = std::make_unique<Attachment>(*apvts.getParameter("FEEDBACK"), feedbackSlider);
    mixSliderAttachment = std::make_unique<Attachment>(*apvts.getParameter("MIX"), mixSlider);

    /*gSlider.setSliderStyle(SliderStyle::LinearVertical);
    gSlider.setRange(0.0f, 1.0f, 0.0f);
    gSlider.setValue(1.0f);
    addAndMakeVisible(gSlider);*/
        addAndMakeVisible(&disChoice);
        disChoice.addItem("Hard Clip", 1);
        disChoice.addItem("Soft Clip", 2);
        disChoice.addItem("Half-Wave Rect", 3);
        disChoice.setSelectedId(1);
        disChoice.addListener(this);

        addAndMakeVisible(&Threshold);
        Threshold.setRange(0.0f, 1.0f, 0.001);
        Threshold.addListener(this);

        addAndMakeVisible(&Mix);
        Mix.setRange(0.0f, 1.0f, 0.001);
        Mix.addListener(this);
    

    addAndMakeVisible(&theme);
    theme.addItem("Default", 4);
    theme.addItem("Dark", 5);
    theme.addItem("Light", 6);
    theme.setSelectedId(4);
    theme.addListener(this);

    filt.setText("Filter", juce::dontSendNotification);
    dist.setText("Distortion", juce::dontSendNotification);
    del.setText("Delay", juce::dontSendNotification);
    themelabel.setText("Theme", juce::dontSendNotification);

    addAndMakeVisible(filt);
    addAndMakeVisible(dist);
    addAndMakeVisible(del);
    addAndMakeVisible(themelabel);

    setSize (500, 550);
}

AudioProcessor2AudioProcessorEditor::~AudioProcessor2AudioProcessorEditor()
{
}

//==============================================================================

void AudioProcessor2AudioProcessorEditor::timerCallback()
{
    if (playButton.getToggleState() == false)
    {
        playButton.setColour(juce::TextButton::buttonColourId, juce::Colours::green);
        playButton.setButtonText("Play");
    }
    else
    {
        playButton.setColour(juce::TextButton::buttonColourId, juce::Colours::red);
        playButton.setButtonText("Stop");
    }
}


void AudioProcessor2AudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)

    //g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
    
    //g.fillAll(juce::Colours::springgreen);

    if (themechoice == 4)
    {
        g.fillAll(juce::Colours::springgreen);
    }

    if (themechoice == 5)
    {
        g.fillAll(juce::Colours::black);
    }
    
    if (themechoice == 6)
    {
        g.fillAll(juce::Colours::orange);
    }
    
    setResizable(false, false);


    //g.setColour (juce::Colours::beige);
    g.setFont (15.0f);

    //juce::Rectangle<int> titleArea(0, 10, getWidth(), 20);

    //g.fillAll(juce::Colours::black);
    //g.setColour(juce::Colours::white);
    //g.drawText("Filter", titleArea, juce::Justification::centredTop);
    //g.drawText("Cutoff", 46, 70, 50, 25, juce::Justification::centredLeft);
    //g.drawText("Resonance", 107, 70, 70, 25, juce::Justification::centredLeft);
    
    juce::Rectangle <float> area(50, 185, 145, 165);

    g.setColour(juce::Colours::red);
    g.drawRoundedRectangle(area, 1.0f, 2.0f);

    juce::Rectangle <float> drect(240, 185, 210, 165);

    g.setColour(juce::Colours::red);
    g.drawRoundedRectangle(drect, 1.0f, 2.0f);
    
    juce::Rectangle <float> dlrect(50, 380, 400, 145);

    g.setColour(juce::Colours::red);
    g.drawRoundedRectangle(dlrect, 1.0f, 2.0f);

    //g.drawFittedText ("", getLocalBounds(), juce::Justification::centred, 1);

}

void AudioProcessor2AudioProcessorEditor::resized()
{
    juce::Rectangle<int> bounds = getLocalBounds();
    juce::FlexBox flexbox;

    flexbox.flexDirection = juce::FlexBox::Direction::column;
    flexbox.alignContent = juce::FlexBox::AlignContent::stretch;
    flexbox.alignItems = juce::FlexBox::AlignItems::stretch;
    flexbox.justifyContent = juce::FlexBox::JustifyContent::center;

    //flexbox.items.add(juce::FlexItem(50, 50, openButton));
    //flexbox.items.add(juce::FlexItem(50, 50, playButton));
    //flexbox.performLayout(bounds);

    openButton.setBounds(190, 30, 100, 25);
    playButton.setBounds(190, 70, 100, 25);
    mGainSlider.setBounds(170, 120, 145, 20);
    GainLabel.setBounds(210, 140, 100, 25);

    //juce::Rectangle<int> area = getLocalBounds().reduced(40);


        filterCutoffDial.setBounds(80, 190, 20, 135);
        filterResDial.setBounds(150, 190, 20, 135);
        Cutofflabel.setBounds(65, 320, 100, 25);
        Reslabel.setBounds(150, 320, 100, 25);
 

    //gSlider.setBounds(290, 100, 20, 135);

    /*const auto column0 = 0.05f;
    const auto column1 = 0.28f;
    const auto column2 = 0.52f;
    const auto row0 = 0.27f;
    const auto row1 = 0.67f;
    const auto row2 = 0.40f;
    const auto dialSize = 0.30f;
    const auto labelSpace = 0.03f;
    const auto labelHeight = 0.05f;*/

    //pluginTitle.setBounds(50, 355, 100, 25);


        feedbackLabel.setBounds(115, 385, 100, 25);
        feedbackSlider.setBounds(115, 405, 80, 100);

        rateLabel.setBounds(210, 385, 100, 25);
        rateSlider.setBounds(210, 405, 80, 100);

        mixLabel.setBounds(305, 385, 100, 25);
        mixSlider.setBounds(305, 405, 80, 100);



        disChoice.setBounds(260, 200, 170, 30);
        Threshold.setBounds(260, 255, 170, 50);
        Mix.setBounds(260, 305, 170, 50);
    

    theme.setBounds(350, 115, 90, 25);

    filt.setBounds(50, 163, 100, 25);
    dist.setBounds(380, 163, 100, 25);
    del.setBounds(50, 357, 100, 25);
    themelabel.setBounds(350, 90, 100, 25);

    //effect.setBounds(310, 90, 100, 30);

}

void AudioProcessor2AudioProcessorEditor::comboBoxChanged(juce::ComboBox* comboBoxThatWasChanged)
{

    audioProcessor.menuChoice = comboBoxThatWasChanged->getSelectedId();

    themechoice = comboBoxThatWasChanged->getSelectedId();

   
}

void AudioProcessor2AudioProcessorEditor::sliderValueChanged(juce::Slider* slider)
{
    if (slider == &mGainSlider)
    {
        audioProcessor.mGain = mGainSlider.getValue();
    }

    if (&Mix == slider)
    {
        audioProcessor.mix = Mix.getValue();
    }
    if (&Threshold == slider)
    {
        audioProcessor.thresh = Threshold.getValue();
    }   
}


// namespace state

//=============================================================================================================

/*StateComponent::StateComponent(StateAB& sab, StatePresets& sp)
    : procStateAB{ sab },
    procStatePresets{ sp },
    toggleABButton{ "A-B" },
    copyABButton{ "Copy" },
    savePresetButton{ "Save preset" },
    deletePresetButton{ "Delete preset" }
{
    addAndMakeVisible(toggleABButton);
    addAndMakeVisible(copyABButton);
    toggleABButton.addListener(this);
    copyABButton.addListener(this);

    addAndMakeVisible(presetBox);
    presetBox.setTextWhenNothingSelected("Load preset...");
    refreshPresetBox();
    ifPresetActiveShowInBox();
    presetBox.addListener(this);

    addAndMakeVisible(savePresetButton);
    savePresetButton.addListener(this);
    addAndMakeVisible(deletePresetButton);
    deletePresetButton.addListener(this);


    toggleABButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff373737));    // possibly set in custom Look instead???
    toggleABButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff808080));
    copyABButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff373737));
    copyABButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff808080));
    savePresetButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff373737));
    savePresetButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff808080));
    deletePresetButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff373737));
    deletePresetButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff808080));

    //setSize (400, 200); // remember to set before xtor finished
}

void StateComponent::paint(juce::Graphics& /)
{
    //g.fillAll (Colours::lightgrey);
}

void StateComponent::resized()
{
    juce::Rectangle<int> r(getLocalBounds());

    const int numComponents{ 6 };
    const int componentWidth{ getWidth() / numComponents };

    toggleABButton.setBounds(r.removeFromLeft(componentWidth).reduced(2));
    copyABButton.setBounds(r.removeFromLeft(componentWidth).reduced(2));
    presetBox.setBounds(r.removeFromLeft(componentWidth * 2).reduced(2));
    savePresetButton.setBounds(r.removeFromLeft(componentWidth).reduced(2));
    deletePresetButton.setBounds(r.removeFromLeft(componentWidth).reduced(2));
}

void StateComponent::buttonClicked(juce::Button* clickedButton)
{
    if (clickedButton == &toggleABButton)     procStateAB.toggleAB();
    if (clickedButton == &copyABButton)       procStateAB.copyAB();
    if (clickedButton == &savePresetButton)   savePresetAlertWindow();
    if (clickedButton == &deletePresetButton) deletePresetAndRefresh();
}

void StateComponent::comboBoxChanged(juce::ComboBox* changedComboBox)
{
    const int selectedId{ changedComboBox->getSelectedId() };
    procStatePresets.loadPreset(selectedId);
}

void populateComboBox(juce::ComboBox& comboBox, const juce::StringArray& listItems)
{
    for (int i = 0; i < listItems.size(); ++i)
        comboBox.addItem(listItems[i], i + 1); // 1-indexed ID for ComboBox
}

void StateComponent::refreshPresetBox()
{
    presetBox.clear();
    juce::StringArray presetNames{ procStatePresets.getPresetNames() };

    populateComboBox(presetBox, presetNames);
}

void StateComponent::ifPresetActiveShowInBox()
{
    const int currentPreset{ procStatePresets.getCurrentPresetId() };
    const int numPresets{ procStatePresets.getNumPresets() };
    if (1 <= currentPreset && currentPreset <= numPresets)
        presetBox.setSelectedId(currentPreset);
}

void StateComponent::deletePresetAndRefresh()
{
    procStatePresets.deletePreset();
    refreshPresetBox();
}

void StateComponent::savePresetAlertWindow()
{
    enum choice { ok, cancel };

    juce::AlertWindow alert{ "Save preset...", "", juce::AlertWindow::AlertIconType::NoIcon };
    alert.addTextEditor("presetEditorID", "Enter preset name");
    alert.addButton("OK", choice::ok, juce::KeyPress(juce::KeyPress::returnKey, 0, 0));
    alert.addButton("Cancel", choice::cancel, juce::KeyPress(juce::KeyPress::escapeKey, 0, 0));

    if (alert.runModalLoop() == choice::ok)                                     // LEAKS when quit while open !!!
    {
       juce::String presetName{ alert.getTextEditorContents("presetEditorID") };

        procStatePresets.savePreset(presetName);
        refreshPresetBox();
        presetBox.setSelectedId(procStatePresets.getNumPresets());
    }
}
 // namespace state
 
*/