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
    sineOsc.setFrequency(5000.0f);

    // Input High Pass
    highPass1.setCoefficients(IIRCoefficients::makeHighPass(fs, 15.9));
    highPass2.setCoefficients(IIRCoefficients::makeHighPass(fs, 15.6));

    // Clipping
    noAA.setSampleRate(fs);
    noAA.setDistortion(500.0e3);
    noAA.makeLookUpTable(8192, fs, 10.0f, 1.0);
    antiAliased.makeLookUpTable(1024, fs/1.5, 10.0f, 1.0);

    // Tone
    toneStage.setSampleRate(fs);
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
    float dist = *distortion / 10.0f;
    dist = pow(10, jmap(dist, 0.0f, 5.69897f))/pow(10, 5.69897f);
    noAA.setDistortion(dist);
    antiAliased.setDistortion(dist);

    // Tone
    toneStage.setTone(*tone / 10.0f);
    // Level
    float outGain = *out / 10.0f;


    // Upsample -------------------------------------------------
    AudioBlock<float> block{ buffer };
    AudioBlock<float> upsampledBlock = overSampling.processSamplesUp(block);

    // Get pointer to block -------------------------------------
    float* left = upsampledBlock.getChannelPointer(0);
    float* right = upsampledBlock.getChannelPointer(1);

    // Process Audio --------------------------------------------
    for (int i = 0; i < upsampledBlock.getNumSamples(); i++)
    {
        // Sine wave - for testing only
        //left[i] = 0.1f * sineOsc.process();

        // Process Audio
        float clipOut;
        float noneOut = noAA.process(inGain * left[i], 1);
        float aaOut = antiAliased.antiAliasedProcess(inGain * left[i]);

        if ((int)*isAa)
            clipOut = aaOut;
        else
            clipOut = noneOut;
        
        float toneOut = 0.95 * outGain * toneStage.processSingleSample(clipOut);
        left[i] = toneOut;
        right[i] = left[i];
    }

    // Downsample -----------------------------
    overSampling.processSamplesDown(block);
}

//==============================================================================
bool TubeScreamerAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* TubeScreamerAudioProcessor::createEditor()
{
    return new GenericAudioProcessorEditor(*this);
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