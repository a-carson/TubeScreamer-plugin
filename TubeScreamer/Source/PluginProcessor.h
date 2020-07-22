/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "TSClippingStage.h"
#include "TSTone.h"
#include "Oscillator.h"
using namespace juce;

//==============================================================================
/**
*/
class TubeScreamerAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    TubeScreamerAudioProcessor();
    ~TubeScreamerAudioProcessor() override;

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

    float distortionVal;
    float level;
    float toneVal;
    bool isOn;

    std::atomic <float>* gain;
    std::atomic <float>* distortion;
    std::atomic <float>* tone;
    std::atomic <float>* out;
    std::atomic <float>* isAa;
private:
    // UI Params
    AudioProcessorValueTreeState parameters;

    double inGain = 1.0;
    SmoothedValue<float> distortionSmoothed;
    SmoothedValue<float> toneSmoothed;
    SmoothedValue<float> outputGainSmoothed;

    // Input high pass filters
    IIRFilter highPass1;
    IIRFilter highPass2;

    // Nonlinearities
    TSClippingStage<double> noAA;
    TSClippingStage<double> antiAliased;
    int os = 1;
    Oversampling<float> overSampling{ (size_t)2, (size_t)os,
                                    Oversampling<float>::filterHalfBandPolyphaseIIR , true, true };

    // Tone Stage
    TSTone<float> toneStage;
    // Sine input for testing
    SineOsc sineOsc;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TubeScreamerAudioProcessor)
};