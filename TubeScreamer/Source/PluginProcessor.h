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
class TubeScreamerAudioProcessor  : public juce::AudioProcessor,
                                    public juce::ValueTree::Listener
                                    
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
    // Audio parameters
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    

    AudioProcessorValueTreeState& getAPVTS() { return parameters; };
    bool isOn;
    std::atomic <float>* gain = nullptr;
    std::atomic <float>* distortion = nullptr;
    std::atomic <float>* tone = nullptr;
    std::atomic <float>* out = nullptr;
    std::atomic <float>* isAa = nullptr;
    std::atomic <float>* isSymm = nullptr;

private:
    AudioProcessorValueTreeState parameters;
    void updatePluginParameters();
    void valueTreePropertyChanged(ValueTree& treeWhosePropertyHasChanged, const Identifier& property) override;
    std::atomic<bool> shouldUpdate{ false };
    SmoothedValue<float> distortionSmoothed;
    SmoothedValue<float> toneSmoothed;
    SmoothedValue<float> levelSmoothed;

    // High pass filter
    IIRFilter highPassOut;

    // Nonlinearities
    TSClippingStage<double> regSymm{TSClippingStage<double>::ClippingType::symmetric};
    TSClippingStage<double> regAsymm{ TSClippingStage<double>::ClippingType::asymmetric };
    TSClippingStage<double> aaSymm{ TSClippingStage<double>::ClippingType::symmetric };
    TSClippingStage<double> aaAsymm{ TSClippingStage<double>::ClippingType::asymmetric };

    // Oversampling
    int os = 1;
    Oversampling<float> overSampling{ (size_t)2, (size_t)os,
                                    Oversampling<float>::filterHalfBandFIREquiripple , true, false };

    // Tone Stage
    TSTone<float> toneStage;

    // Sine input for testing
    SineOsc sineOsc;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TubeScreamerAudioProcessor)
};