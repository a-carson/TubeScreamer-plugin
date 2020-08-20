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
    std::make_unique < AudioParameterFloat >("dist", "Distortion", 0.0f, 1.0f, 0.5f),
    std::make_unique < AudioParameterFloat >("tone", "Tone", 0.0001f, 0.9999f, 0.5f),
    std::make_unique < AudioParameterFloat >("output", "Level", 0.0f, 1.0f, 0.5f),
    std::make_unique < AudioParameterBool >("aa", "Anti-aliasing", 1),
    std::make_unique < AudioParameterChoice >("clip_type", "Clipping Type", StringArray{"Symmetric", "Asymmetric"}, 1),
        })

{
    gain = parameters.getRawParameterValue("gain");
    out = parameters.getRawParameterValue("output");
    distortion = parameters.getRawParameterValue("dist");
    tone = parameters.getRawParameterValue("tone");
    isAa = parameters.getRawParameterValue("aa");
    isSymm = parameters.getRawParameterValue("clip_type");

    parameters.state.addListener(this);
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
    // Oversampled sampling frequency
    float fs = sampleRate * overSampling.getOversamplingFactor();

    // Sine Osc - for testing only
    sineOsc.setSampleRate(fs);
    sineOsc.setFrequency(220.0);

    // Input and Output High Pass (DC Block)
    highPassIn.setCoefficients(IIRCoefficients::makeHighPass(sampleRate, 15.6));
    highPassOut.setCoefficients(IIRCoefficients::makeHighPass(sampleRate, 3.0));

    // Clipping
    overSampling.initProcessing(samplesPerBlock);
    regSymm.makeLookUpTable(32768, fs, 50.0, 1.0);
    regAsymm.makeLookUpTable(32768, fs, 50.0, 1.0);
    aaSymm.makeLookUpTable(32768, fs / 1.5, 50.0, 1.0);
    aaAsymm.makeLookUpTable(32768, fs / 1.5, 50.0, 1.0);

    // Tone
    toneStage.setSampleRate(sampleRate);
    toneStage.setTone(1.0f);

    // UI Parameters
    levelSmoothed.reset(sampleRate, 0.01);
    levelSmoothed.setCurrentAndTargetValue(0.0);
    toneSmoothed.reset(sampleRate, 0.01);
    toneSmoothed.setCurrentAndTargetValue(0.0);
    updatePluginParameters();

}

void TubeScreamerAudioProcessor::releaseResources()
{

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
    if (shouldUpdate)
        updatePluginParameters();


    if (isOn)
    {
        // Input High pass filters ---------------------------------
        float* samples = buffer.getWritePointer(0);
        //highPassIn.processSamples(samples, buffer.getNumSamples());

        // Non-linearity -------------------------------------------
        // Upsample
        AudioBlock<float> block{ buffer };
        AudioBlock<float> upsampledBlock = overSampling.processSamplesUp(block);
        float* newSamples = upsampledBlock.getChannelPointer(0);

        // Loop
        for (int i = 0; i < upsampledBlock.getNumSamples(); i++)
        {
            JUCE_SNAP_TO_ZERO(newSamples[i]);
            // Sine wave - for testing only
            //newSamples[i] = 0.1f * sineOsc.process();
            newSamples[i] *= 0.95F;
            if ((int)*isAa)
            { 
                if ((int)*isSymm < 1)
                    newSamples[i] = aaSymm.antiAliasedProcess(newSamples[i]);
                else
                    newSamples[i] = aaAsymm.antiAliasedProcess(newSamples[i]);
            }
            else
            {
                if ((int)*isSymm < 1)
                    newSamples[i] = regSymm.process(newSamples[i], false);
                else
                    newSamples[i] = regAsymm.process(newSamples[i], false);
            }

            float outputLevel = levelSmoothed.getNextValue();
            newSamples[i] *= outputLevel;
        }

        // Downsample
        overSampling.processSamplesDown(block);

        // Tone Stage -------------------------------------------
        float* downSamples = buffer.getWritePointer(0);
        toneStage.processBlock(downSamples, buffer.getNumSamples());
        highPassOut.processSamples(downSamples, buffer.getNumSamples());

        // Copy to all output channels
        for (int channel = 0; channel < totalNumInputChannels; channel++)
        {
            auto* channelData = buffer.getWritePointer(channel);

            for (int i = 0; i < buffer.getNumSamples(); i++)
            {
                channelData[i] = downSamples[i];
            }
        }
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
    auto state = parameters.copyState();
    std::unique_ptr<XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void TubeScreamerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr)
    {
        if (xmlState->hasTagName(parameters.state.getType()))
        {
            parameters.replaceState(ValueTree::fromXml(*xmlState));
        }
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TubeScreamerAudioProcessor();
}

void TubeScreamerAudioProcessor::updatePluginParameters()
{ 
    regSymm.setDistortion(*distortion);
    regAsymm.setDistortion(*distortion);
    aaSymm.setDistortion(*distortion);
    aaAsymm.setDistortion(*distortion);
    float toneLog = powf(*tone, 0.5);
    toneStage.setTone(toneLog);
    levelSmoothed.setTargetValue(*out);
}

void TubeScreamerAudioProcessor::valueTreePropertyChanged(ValueTree& treeWhosePropertyHasChanged, const Identifier& property)
{
    shouldUpdate = true;
}