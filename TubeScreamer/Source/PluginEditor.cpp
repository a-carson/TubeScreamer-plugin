/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
TubeScreamerAudioProcessorEditor::TubeScreamerAudioProcessorEditor (TubeScreamerAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (300, 400);

    addAndMakeVisible(distortionKnob);
    distortionKnob.setSliderStyle(juce::Slider::Rotary);
    distortionKnob.setRange(0.0f, 1.0f, 0.01f);
    distortionKnob.setValue(0.5f);
    distortionKnob.setRotaryParameters(MathConstants<float>::pi *(9.0f/8.0f), (23.0f / 8.0f)*MathConstants<float>::pi, true);
    distortionKnob.setTextBoxStyle(Slider::TextEntryBoxPosition::NoTextBox, false, 50, 50);
    distortionKnob.addListener(this);
    sliderValueChanged(&distortionKnob);

    addAndMakeVisible(toneKnob);
    toneKnob.setSliderStyle(juce::Slider::Rotary);
    toneKnob.setRange(0.0f, 1.0f, 0.01f);
    toneKnob.setValue(0.5f);
    toneKnob.setRotaryParameters(MathConstants<float>::pi * (9.0f / 8.0f), (23.0f / 8.0f) * MathConstants<float>::pi, true);
    toneKnob.setTextBoxStyle(Slider::TextEntryBoxPosition::NoTextBox, false, 50, 50);
    toneKnob.addListener(this);
    sliderValueChanged(&toneKnob);

    addAndMakeVisible(levelKnob);
    levelKnob.setSliderStyle(juce::Slider::Rotary);
    levelKnob.setRange(0.0f, 1.0f, 0.01f);
    levelKnob.setValue(0.5f);
    levelKnob.setRotaryParameters(MathConstants<float>::pi * (9.0f / 8.0f), (23.0f / 8.0f) * MathConstants<float>::pi, true);
    levelKnob.setTextBoxStyle(Slider::TextEntryBoxPosition::NoTextBox, false, 50, 50);
    levelKnob.addListener(this);
    sliderValueChanged(&levelKnob);

    addAndMakeVisible(distortionLabel);
    distortionLabel.setText("OVERDRIVE", NotificationType::dontSendNotification);
    distortionLabel.setJustificationType(Justification::centred);

    addAndMakeVisible(toneLabel);
    toneLabel.setText("TONE", NotificationType::dontSendNotification);
    toneLabel.setJustificationType(Justification::centred);

    addAndMakeVisible(levelLabel);
    levelLabel.setText("LEVEL", NotificationType::dontSendNotification);
    levelLabel.setJustificationType(Justification::centred);


}

TubeScreamerAudioProcessorEditor::~TubeScreamerAudioProcessorEditor()
{
}

//==============================================================================
void TubeScreamerAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll(Colours::green);
}

void TubeScreamerAudioProcessorEditor::resized()
{
    const int border = 30;
    const int knobSize = 80;
    const int textHeight = 15;
    const int gap = 10;

    distortionKnob.setBounds(border, border, knobSize, knobSize);
    levelKnob.setBounds(getWidth() - border - knobSize, border, knobSize, knobSize);
    toneKnob.setBounds(0.5 * (getWidth() - knobSize), border + 0.5 * knobSize + textHeight, knobSize, knobSize);


    distortionLabel.setBounds(border, border + knobSize - gap, knobSize, textHeight);
    levelLabel.setBounds(getWidth() - border - knobSize, border + knobSize - gap, knobSize, textHeight);
    toneLabel.setBounds(0.5 * (getWidth() - knobSize), border + 1.5 * knobSize + textHeight - gap, knobSize, textHeight);

}

void TubeScreamerAudioProcessorEditor::sliderValueChanged(juce::Slider* slider)
{
    audioProcessor.distortionVal = distortionKnob.getValue();
    audioProcessor.level = levelKnob.getValue();
    audioProcessor.toneVal = toneKnob.getValue();
}