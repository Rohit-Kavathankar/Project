/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class AudioProcessor2AudioProcessorEditor  : public juce::AudioProcessorEditor, private juce::Timer, public juce::Slider::Listener, 
                                            private juce::ComboBox::Listener
{
public:
    AudioProcessor2AudioProcessorEditor (AudioProcessor2AudioProcessor&);
    ~AudioProcessor2AudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

    void timerCallback();

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> buttonState;

    void sliderValueChanged(juce::Slider* slider);

    


private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.

    juce::TextButton playButton{ "Play" };
    juce::TextButton openButton{ "Open" };

    juce::Slider mGainSlider;
    juce::Label  GainLabel;

    juce::Slider filterCutoffDial;
    juce::Slider filterResDial;
    juce::Label Cutofflabel;
    juce::Label Reslabel;


    juce::ScopedPointer<juce::AudioProcessorValueTreeState::SliderAttachment> filterCutoffValue;
    juce::ScopedPointer<juce::AudioProcessorValueTreeState::SliderAttachment> filterResValue;

    juce::Slider rateSlider;
    juce::Slider feedbackSlider;
    juce::Slider mixSlider;
    juce::Label rateLabel{ "Rate", "Rate" };
    juce::Label feedbackLabel{ "Feedback", "Feedback" };
    juce::Label mixLabel{ "Mix", "Mix" };
    juce::Label pluginTitle{ "Plug-in Title", "Delay" };

    using Attachment = std::unique_ptr<juce::SliderParameterAttachment>;

    Attachment rateSliderAttachment;
    Attachment feedbackSliderAttachment;
    Attachment mixSliderAttachment;

    juce::Slider gSlider;

    void comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged) override;
    


    juce::ComboBox disChoice;
    juce::Slider Threshold;
    juce::Slider Mix;

    juce::ComboBox theme;
    int themechoice;

    juce::Label filt;
    juce::Label dist;
    juce::Label del;
    juce::Label themelabel;

    AudioProcessor2AudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioProcessor2AudioProcessorEditor)
};

//================================================================================================
/*class StateAB
{
public:
    explicit StateAB(juce::AudioProcessor& p);

    void toggleAB();
    void copyAB();

private:
    juce::AudioProcessor& pluginProcessor;
    juce::XmlElement ab{ "AB" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StateAB);
};

class StateComponent : public juce::Component,
    public juce::Button::Listener,
    public juce::ComboBox::Listener
{
public:
    StateComponent(StateAB& sab, StatePresets& sp);

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    StateAB& procStateAB;
    StatePresets& procStatePresets;

    juce::TextButton toggleABButton;
    juce::TextButton copyABButton;
    juce::ComboBox   presetBox;
    juce::TextButton savePresetButton;
    juce::TextButton deletePresetButton;

    void buttonClicked(juce::Button* clickedButton) override;
    void comboBoxChanged(juce::ComboBox* changedComboBox) override;

    void refreshPresetBox();
    void ifPresetActiveShowInBox();
    void deletePresetAndRefresh();
    void savePresetAlertWindow();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StateComponent);
};*/