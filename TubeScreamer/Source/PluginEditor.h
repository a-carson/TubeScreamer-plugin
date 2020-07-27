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
class MyLookAndFeel : public juce::LookAndFeel_V4
{
public:
    MyLookAndFeel()
    {
        
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
        const float rotaryStartAngle, const float rotaryEndAngle, juce::Slider&) override
    {
        auto radius = (float)juce::jmin(width / 2, height / 2) - 10.0f;
        auto centreX = (float)x + (float)width * 0.5f;
        auto centreY = (float)y + (float)height * 0.5f;
        auto rx = centreX - radius;
        auto ry = centreY - radius;
        auto rw = radius * 2.0f;
        auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        // fill
        g.setColour(juce::Colours::black);
        g.fillEllipse(rx, ry, rw, rw);

        // outline
        g.setColour(juce::Colours::black);
        g.drawEllipse(rx, ry, rw, rw, 1.0f);

        juce::Path p;
        auto pointerLength = radius * 0.33f;
        auto pointerThickness = 2.0f;
        p.addRectangle(-pointerThickness * 0.5f, -radius, pointerThickness, pointerLength);
        p.applyTransform(juce::AffineTransform::rotation(angle).translated(centreX, centreY));

        // pointer
        g.setColour(juce::Colours::whitesmoke);
        g.fillPath(p);
    }
};

class TubeScreamerAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                          public juce::Button::Listener
                                          //private juce::Slider::Listener
                                           
{
public:
    TubeScreamerAudioProcessorEditor (TubeScreamerAudioProcessor&);
    ~TubeScreamerAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;


private:

    void buttonClicked(Button* button) override;
    void pedalOn();
    void pedalOff();

    TubeScreamerAudioProcessor& audioProcessor;
    Slider distortionKnob, toneKnob, levelKnob;
    Label distortionLabel, toneLabel, levelLabel, title;
    MyLookAndFeel myLookAndFeel;
    Font font;

    typedef juce::AudioProcessorValueTreeState::SliderAttachment SliderAttachment;
    typedef juce::AudioProcessorValueTreeState::ButtonAttachment ButtonAttachment;
    typedef juce::AudioProcessorValueTreeState::ComboBoxAttachment ComboBoxAttachment;
    std::unique_ptr<SliderAttachment> distortionAttachment, toneAttachment, levelAttachment;
    std::unique_ptr<ButtonAttachment> bypassAttachment;
    std::unique_ptr<ComboBoxAttachment> dropDownAttachment;

    bool isOn = true;
    TextButton textButton{ "BYPASS" };
    
    Component led;
    Colour ledColour = Colours::red;
    Colour tsColour{ 72, 191, 93 };

    ComboBox dropDown;
    Label dropDownLabel;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TubeScreamerAudioProcessorEditor)
};