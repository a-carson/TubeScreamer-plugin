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
class TubeScreamerAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                          private juce::Slider::Listener
{
public:
    TubeScreamerAudioProcessorEditor (TubeScreamerAudioProcessor&);
    ~TubeScreamerAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

    Slider distortionKnob;

private:
    void sliderValueChanged(juce::Slider* slider) override;

    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    TubeScreamerAudioProcessor& audioProcessor;
    Slider toneKnob;
    Slider levelKnob;

    Label distortionLabel;
    Label toneLabel;
    Label levelLabel;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TubeScreamerAudioProcessorEditor)
};