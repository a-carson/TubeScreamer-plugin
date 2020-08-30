/*-----------------------------------------------------------------------
 ALIASING REDUCTION IN VIRTUAL ANALOGUE MODELLING
 Alistair Carson 2020
 MSc Acoustics & Music Technology
 University of Edinburgh
--------------------------------------------------------------------*/

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
    distortionKnob.setValue(*audioProcessor.distortion);
    distortionKnob.setRotaryParameters(MathConstants<float>::pi * (9.0f / 8.0f), (23.0f / 8.0f) * MathConstants<float>::pi, true);
    distortionKnob.setTextBoxStyle(Slider::TextEntryBoxPosition::NoTextBox, false, 50, 50);
    distortionAttachment = std::make_unique<SliderAttachment>(audioProcessor.getAPVTS(), "dist", distortionKnob);


    addAndMakeVisible(toneKnob);
    toneKnob.setSliderStyle(juce::Slider::Rotary);
    toneKnob.setValue(*audioProcessor.tone);
    toneKnob.setRotaryParameters(MathConstants<float>::pi * (9.0f / 8.0f), (23.0f / 8.0f) * MathConstants<float>::pi, true);
    //toneKnob.setSkewFactorFromMidPoint(0.9);
    toneKnob.setSkewFactor(10.0, false);
    toneKnob.setTextBoxStyle(Slider::TextEntryBoxPosition::NoTextBox, false, 50, 50);
    toneAttachment = std::make_unique<SliderAttachment>(audioProcessor.getAPVTS(), "tone", toneKnob);

    addAndMakeVisible(levelKnob);
    levelKnob.setSliderStyle(juce::Slider::Rotary);
    levelKnob.setRange(0.0f, 1.0f, 0.01f);
    levelKnob.setValue(*audioProcessor.out);
    levelKnob.setRotaryParameters(MathConstants<float>::pi * (9.0f / 8.0f), (23.0f / 8.0f) * MathConstants<float>::pi, true);
    levelKnob.setTextBoxStyle(Slider::TextEntryBoxPosition::NoTextBox, false, 50, 50);
    levelAttachment = std::make_unique<SliderAttachment>(audioProcessor.getAPVTS(), "output", levelKnob);

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
    title.setText("TS OVERDRIVE", NotificationType::dontSendNotification);
    title.setJustificationType(Justification::centred);
    title.setFont(font);
    title.setColour(Label::textColourId, Colours::whitesmoke);


    addAndMakeVisible(textButton);
    textButton.setColour(TextButton::buttonColourId, Colours::darkgrey);
    textButton.setColour(TextButton::buttonOnColourId, Colours::silver);
    textButton.addListener(this);
    textButton.setToggleState(true, NotificationType::dontSendNotification);

    addAndMakeVisible(led);

    addAndMakeVisible(dropDown);
    dropDown.addItemList({ "SYMMETRIC", "ASYMMETRIC" }, 1);
    dropDown.setColour(ComboBox::backgroundColourId,Colours::silver);
    dropDown.setJustificationType(Justification::centred);
    dropDown.setSelectedId(0, NotificationType::dontSendNotification);
    dropDownAttachment = std::make_unique<ComboBoxAttachment>(audioProcessor.getAPVTS(), "clip_type", dropDown);
    dropDownLabel.attachToComponent(&dropDown, true);
    dropDownLabel.setText("CLIPPING TYPE", NotificationType::dontSendNotification);
    dropDownLabel.setColour(Label::textColourId, Colours::whitesmoke);
    dropDown.setColour(ComboBox::textColourId, Colours::whitesmoke);
}

TubeScreamerAudioProcessorEditor::~TubeScreamerAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

//==============================================================================
void TubeScreamerAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll(tsColour);
    g.setColour(ledColour);
    float diameter = 20.0f;
    g.fillEllipse(0.5 * (getWidth() - diameter), 30.0f, diameter, diameter);
    setLookAndFeel(&myLookAndFeel);
}

void TubeScreamerAudioProcessorEditor::resized()
{
    const int border = 30;
    const int yborder = 25;
    const int knobSize = 80;
    const int textHeight = 15;
    const int gap = 10;
    const int buttonSize = 125;

    // Knobs
    distortionKnob.setBounds(border, yborder, knobSize, knobSize);
    levelKnob.setBounds(getWidth() - border - knobSize, yborder, knobSize, knobSize);
    toneKnob.setBounds(0.5 * (getWidth() - knobSize), yborder + 0.5 * knobSize + textHeight, knobSize, knobSize);

    // Labels
    distortionLabel.setBounds(border, yborder + knobSize - gap, knobSize, textHeight);
    levelLabel.setBounds(getWidth() - border - knobSize, yborder + knobSize - gap, knobSize, textHeight);
    toneLabel.setBounds(0.5 * (getWidth() - knobSize), yborder + 1.5 * knobSize + textHeight - gap, knobSize, textHeight);
    
    // Main title 
    title.setBounds(0.5 * (getWidth() - 200), 0.47 * (getHeight() - font.getHeight()), 200, gap + font.getHeight());

    // Button
    textButton.setBounds(0.5*(getWidth() - buttonSize), 0.7 * getHeight() - 0.5*buttonSize, buttonSize, buttonSize);

    dropDown.setBounds(0.5*getWidth(), 0.9 * getHeight(), buttonSize, 20);
    dropDownLabel.setBoundsRelative(1.0f, 1.0f, 1.0f, 1.0f);
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
    dropDown.setColour(ComboBox::backgroundColourId, Colours::silver);
    ledColour = Colours::red;
    textButton.setButtonText("BYPASS");
    repaint();
}

void TubeScreamerAudioProcessorEditor::pedalOff()
{
    textButton.setToggleState(false, NotificationType::dontSendNotification);
    isOn = false;
    audioProcessor.isOn = false;
    ledColour = Colours::black;
    dropDown.setColour(ComboBox::backgroundColourId, Colours::darkgrey);
    textButton.setButtonText("BYPASSED");
    repaint();
}