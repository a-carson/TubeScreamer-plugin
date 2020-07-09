/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
TubeScreamerAudioProcessor::TubeScreamerAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
#endif
    parameters(*this, nullptr, "ParamTreeIdentifier", {
    std::make_unique < AudioParameterFloat >("gain", "Gain", -10.0f, 35.0f , 0.0f) ,
    std::make_unique < AudioParameterFloat >("output", "Level", -60.0f, 0.0f, -30.0f),
        })
{
    gain = parameters.getRawParameterValue("gain");
    out = parameters.getRawParameterValue("output");
}

TubeScreamerAudioProcessor::~TubeScreamerAudioProcessor()
{
}

//==============================================================================
const juce::String TubeScreamerAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool TubeScreamerAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool TubeScreamerAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool TubeScreamerAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double TubeScreamerAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int TubeScreamerAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int TubeScreamerAudioProcessor::getCurrentProgram()
{
    return 0;
}

void TubeScreamerAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String TubeScreamerAudioProcessor::getProgramName (int index)
{
    return {};
}

void TubeScreamerAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void TubeScreamerAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sineOsc.setSampleRate(sampleRate);
    sineOsc.setFrequency(220.0f);
    clippingStage.setSampleRate(sampleRate);
    clippingStage.setDistortion(0.5f);
}

void TubeScreamerAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool TubeScreamerAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void TubeScreamerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // get buffer info
    int numSamples = buffer.getNumSamples();
    float* left = buffer.getWritePointer(0);
    float* right = buffer.getWritePointer(1);

    // get UI params
    float inGain = pow(10, *gain / 20.0f);
    float outGain = pow(10, *out / 20.0f);

    // Clipping stage
    for (int i = 0; i < numSamples; i++)
    {
        // for testing
        //left[i] = 0.01f * sineOsc.process();

        // process audio
        left[i] = outGain * clippingStage.process(inGain * left[i]);
        right[i] = left[i];
    }


}

//==============================================================================
bool TubeScreamerAudioProcessor::hasEditor() const
{
    return false; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* TubeScreamerAudioProcessor::createEditor()
{
    return new TubeScreamerAudioProcessorEditor (*this);
}

//==============================================================================
void TubeScreamerAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void TubeScreamerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TubeScreamerAudioProcessor();
}
