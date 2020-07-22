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
        setColour(juce::Slider::thumbColourId, juce::Colours::red);
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
                                          public juce::Button::Listener,
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
    void sliderValueChanged(Slider* slider) override;
    void buttonClicked(Button* button) override;
    void pedalOn();
    void pedalOff();

    TubeScreamerAudioProcessor& audioProcessor;
    Slider toneKnob;
    Slider levelKnob;

    Label distortionLabel;
    Label toneLabel;
    Label levelLabel;
    Label title;
    MyLookAndFeel myLookAndFeel;
    Font font;

    bool isOn = true;


    TextButton textButton{ "BYPASS" };
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TubeScreamerAudioProcessorEditor)
};