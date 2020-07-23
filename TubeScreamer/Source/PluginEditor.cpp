/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
TubeScreamerAudioProcessorEditor::TubeScreamerAudioProcessorEditor(TubeScreamerAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setSize(300, 400);

    addAndMakeVisible(distortionKnob);
    distortionKnob.setSliderStyle(juce::Slider::Rotary);
    distortionKnob.setRange(0.0f, 1.0f, 0.01f);
    distortionKnob.setValue(*audioProcessor.distortion);
    distortionKnob.setRotaryParameters(MathConstants<float>::pi * (9.0f / 8.0f), (23.0f / 8.0f) * MathConstants<float>::pi, true);
    distortionKnob.setTextBoxStyle(Slider::TextEntryBoxPosition::NoTextBox, false, 50, 50);
    distortionKnob.addListener(this);
    //sliderValueChanged(&distortionKnob);



    addAndMakeVisible(toneKnob);
    toneKnob.setSliderStyle(juce::Slider::Rotary);
    toneKnob.setRange(0.0f, 1.0f, 0.01f);
    toneKnob.setValue(*audioProcessor.tone);
    toneKnob.setRotaryParameters(MathConstants<float>::pi * (9.0f / 8.0f), (23.0f / 8.0f) * MathConstants<float>::pi, true);
    toneKnob.setTextBoxStyle(Slider::TextEntryBoxPosition::NoTextBox, false, 50, 50);
    toneKnob.addListener(this);
    //sliderValueChanged(&toneKnob);

    addAndMakeVisible(levelKnob);
    levelKnob.setSliderStyle(juce::Slider::Rotary);
    levelKnob.setRange(0.0f, 1.0f, 0.01f);
    levelKnob.setValue(*audioProcessor.out);
    levelKnob.setRotaryParameters(MathConstants<float>::pi * (9.0f / 8.0f), (23.0f / 8.0f) * MathConstants<float>::pi, true);
    levelKnob.setTextBoxStyle(Slider::TextEntryBoxPosition::NoTextBox, false, 50, 50);
    levelKnob.addListener(this);
    //sliderValueChanged(&levelKnob);

    addAndMakeVisible(distortionLabel);
    distortionLabel.setText("OVERDRIVE", NotificationType::dontSendNotification);
    distortionLabel.setJustificationType(Justification::centred);
    distortionLabel.setColour(Label::textColourId, Colours::whitesmoke);

    addAndMakeVisible(toneLabel);
    toneLabel.setText("TONE", NotificationType::dontSendNotification);
    toneLabel.setJustificationType(Justification::centred);
    toneLabel.setColour(Label::textColourId, Colours::whitesmoke);


    addAndMakeVisible(levelLabel);
    levelLabel.setText("LEVEL", NotificationType::dontSendNotification);
    levelLabel.setJustificationType(Justification::centred);
    levelLabel.setColour(Label::textColourId, Colours::whitesmoke);


    font.setBold(true);
    font.setHeight(30.0f);
    addAndMakeVisible(title);
    title.setText("TUBE SCREAMER", NotificationType::dontSendNotification);
    title.setJustificationType(Justification::centred);
    title.setFont(font);
    title.setColour(Label::textColourId, Colours::whitesmoke);


    addAndMakeVisible(textButton);
    textButton.setColour(TextButton::buttonColourId, Colours::dimgrey);
    textButton.addListener(this);
    textButton.setToggleState(true, NotificationType::dontSendNotification);
}

TubeScreamerAudioProcessorEditor::~TubeScreamerAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

//==============================================================================
void TubeScreamerAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll(Colours::forestgreen);
    g.setColour(Colours::red);
    float diameter = 20.0f;
    g.fillEllipse(0.5 * (getWidth() - diameter), 30.0f, diameter, diameter);
    setLookAndFeel(&myLookAndFeel);
}

void TubeScreamerAudioProcessorEditor::resized()
{
    const int border = 30;
    const int knobSize = 80;
    const int textHeight = 15;
    const int gap = 10;
    const int buttonSize = 125;

    // Knobs
    distortionKnob.setBounds(border, border, knobSize, knobSize);
    levelKnob.setBounds(getWidth() - border - knobSize, border, knobSize, knobSize);
    toneKnob.setBounds(0.5 * (getWidth() - knobSize), border + 0.5 * knobSize + textHeight, knobSize, knobSize);

    // Labels
    distortionLabel.setBounds(border, border + knobSize - gap, knobSize, textHeight);
    levelLabel.setBounds(getWidth() - border - knobSize, border + knobSize - gap, knobSize, textHeight);
    toneLabel.setBounds(0.5 * (getWidth() - knobSize), border + 1.5 * knobSize + textHeight - gap, knobSize, textHeight);
    
    // Main title 
    title.setBounds(0.5 * (getWidth() - 200), 0.5 * (getHeight() - gap - font.getHeight()), 200, gap + font.getHeight());

    // Button
    textButton.setBounds(0.5*(getWidth() - buttonSize), 0.75 * getHeight() - 0.5*buttonSize, buttonSize, buttonSize);

}

void TubeScreamerAudioProcessorEditor::sliderValueChanged(juce::Slider* slider)
{
    *audioProcessor.distortion = distortionKnob.getValue();
    *audioProcessor.out = levelKnob.getValue();
    *audioProcessor.tone = toneKnob.getValue();
}

void TubeScreamerAudioProcessorEditor::buttonClicked(Button* button)
{
    if (isOn == false)
    {
        textButton.onClick = [this]() {pedalOn(); };
    }
    else if (isOn)
    {
        textButton.onClick = [this]() {pedalOff(); };
    }
}

void TubeScreamerAudioProcessorEditor::pedalOn()
{
    textButton.setToggleState(true, NotificationType::dontSendNotification);
    isOn = true;
    audioProcessor.isOn = true;
}

void TubeScreamerAudioProcessorEditor::pedalOff()
{
    textButton.setToggleState(false, NotificationType::dontSendNotification);
    isOn = false;
    audioProcessor.isOn = false;
}