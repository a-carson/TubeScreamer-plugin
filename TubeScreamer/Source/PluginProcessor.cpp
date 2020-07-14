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
    //std::make_unique < AudioParameterFloat >("gain", "Gain", -10.0f, 35.0f , 0.0f) ,
    std::make_unique < AudioParameterFloat >("dist", "Distortion", 0.0f, 10.0f, 5.0f),
    std::make_unique < AudioParameterFloat >("tone", "Tone", 0.0f, 10.0f, 5.0f),
    std::make_unique < AudioParameterFloat >("output", "Level", 0.0f, 10.0f, 5.0f),
        })
{
    gain = parameters.getRawParameterValue("gain");
    out = parameters.getRawParameterValue("output");
    distortion = parameters.getRawParameterValue("dist");
    tone = parameters.getRawParameterValue("tone");
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
    // Sine Osc - for testing only
    sineOsc.setSampleRate(sampleRate);
    sineOsc.setFrequency(220.0f);

    // Input High Pass
    highPass1.setCoefficients(IIRCoefficients::makeHighPass(sampleRate, 15.9));
    highPass2.setCoefficients(IIRCoefficients::makeHighPass(sampleRate, 15.6));

    // Clipping
    clippingStage.setSampleRate(sampleRate);
    clippingStage.setDistortion(0.5f);

    // Tone
    toneStage.setSampleRate(sampleRate);
    toneStage.setTone(1.0f);
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

    // Get Buffer ----------------------------------------------
    int numSamples = buffer.getNumSamples();
    float* left = buffer.getWritePointer(0);
    float* right = buffer.getWritePointer(1);

    // UI Params -----------------------------------------------
    //inGain = pow(10, *gain / 20.0f);
    // Distortion
    float dist = *distortion/10.0f;
    dist = pow(10, jmap(dist, 0.0f, 5.7f));
    clippingStage.setDistortion(dist);

    // Tone
    toneStage.setTone(*tone / 10.0f);

    // Level
    float outGain = *out / 10.0f;

    // Process Audio ---------------------------------------------
    for (int i = 0; i < numSamples; i++)
    {
        // Sine wave - for testing only
        //left[i] = 0.1f * sineOsc.process();

        // Process Audio
        float highPassOut = highPass1.processSingleSampleRaw(left[i]);
        highPassOut = highPass2.processSingleSampleRaw(highPassOut);
        float clipOut = clippingStage.process(inGain * highPassOut);
        float toneOut = 0.95 * outGain * toneStage.processSingleSample(clipOut);
        left[i] = toneOut;
        right[i] = toneOut;
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