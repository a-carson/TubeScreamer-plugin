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
    : AudioProcessor(BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
    ),
#endif
    parameters(*this, nullptr, "ParamTreeIdentifier", {
    //std::make_unique < AudioParameterFloat >("gain", "Gain", -10.0f, 35.0f , 0.0f) ,
    std::make_unique < AudioParameterFloat >("dist", "Distortion", 0.0f, 10.0f, 5.0f),
    std::make_unique < AudioParameterFloat >("tone", "Tone", 0.0f, 10.0f, 5.0f),
    std::make_unique < AudioParameterFloat >("output", "Level", 0.0f, 10.0f, 5.0f),
    std::make_unique < AudioParameterBool >("aa", "Anti-aliasing", 1),
        })

{
    gain = parameters.getRawParameterValue("gain");
    out = parameters.getRawParameterValue("output");
    distortion = parameters.getRawParameterValue("dist");
    tone = parameters.getRawParameterValue("tone");
    isAa = parameters.getRawParameterValue("aa");
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
    float fs = sampleRate * overSampling.getOversamplingFactor();
    sineOsc.setSampleRate(fs);
    sineOsc.setFrequency(1000.0);

    // Input High Pass
    highPass1.setCoefficients(IIRCoefficients::makeHighPass(sampleRate, 15.9));
    highPass2.setCoefficients(IIRCoefficients::makeHighPass(sampleRate, 15.6));

    // Clipping
    noAA.setSampleRate(fs);
    noAA.setDistortion(500.0e3);
    noAA.makeLookUpTable(8192, fs, 10.0f, 500.0e3);
    antiAliased.makeLookUpTable(1024, fs/1.5, 10.0f, 500.0e3);

    // Tone
    toneStage.setSampleRate(sampleRate);
    toneStage.setTone(1.0f);

    overSampling.initProcessing(samplesPerBlock);

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

    // UI Params -----------------------------------------------

    // Distortion
    float dist = pow(10, jmap(distortionVal, 0.0f, 5.69897f));
    noAA.setDistortion(dist);
    antiAliased.setDistortion(dist);

    // Tone
    toneStage.setTone(toneVal);

    // Level
    float outGain = level;

    // Input High pass filters ---------------------------------
    float* samples = buffer.getWritePointer(0);
    highPass1.processSamples(samples, buffer.getNumSamples());
    highPass2.processSamples(samples, buffer.getNumSamples());
    
    // Non-linearity -------------------------------------------
    
    // Upsample
    AudioBlock<float> block{ buffer };
    AudioBlock<float> upsampledBlock = overSampling.processSamplesUp(block);
    float* newSamples = upsampledBlock.getChannelPointer(0);

    // Loop
    for (int i = 0; i < upsampledBlock.getNumSamples(); i++)
    {
        // Sine wave - for testing only
        //newSamples[i] = 0.01f * sineOsc.process();

        float regularOut = noAA.process(inGain * newSamples[i], 1);
        float aaOut = antiAliased.antiAliasedProcess(inGain * newSamples[i]);

        if ((int)*isAa)
            newSamples[i] = outGain * aaOut;
        else
            newSamples[i] = outGain * regularOut;
    }

    // Downsample
    overSampling.processSamplesDown(block);

    // Tone Stage -------------------------------------------
    float* downSamples = buffer.getWritePointer(0);
    toneStage.processBlock(downSamples, buffer.getNumSamples());



    // Copy to all output channels
    for (int channel = 0; channel < totalNumInputChannels; channel++)
    {
        auto* channelData = buffer.getWritePointer(channel);

        for (int i = 0; i < buffer.getNumSamples(); i++)
            channelData[i] = downSamples[i];
    }

}

//==============================================================================
bool TubeScreamerAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* TubeScreamerAudioProcessor::createEditor()
{
    return new TubeScreamerAudioProcessorEditor(*this);
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