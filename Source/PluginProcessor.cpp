#include "PluginProcessor.h"
#include "PluginEditor.h"

static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    using FloatParam = juce::AudioParameterFloat;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    p.push_back (std::make_unique<FloatParam> ("threshold", "Threshold",
        juce::NormalisableRange<float> (-60.0f, 0.0f, 0.1f), -18.0f, "dB"));
    p.push_back (std::make_unique<FloatParam> ("ratio", "Ratio",
        juce::NormalisableRange<float> (1.0f, 20.0f, 0.1f), 4.0f, ":1"));
    p.push_back (std::make_unique<FloatParam> ("attack", "Attack",
        juce::NormalisableRange<float> (0.1f, 200.0f, 0.1f, 0.5f), 10.0f, "ms"));
    p.push_back (std::make_unique<FloatParam> ("release", "Release",
        juce::NormalisableRange<float> (5.0f, 1000.0f, 1.0f, 0.5f), 120.0f, "ms"));
    p.push_back (std::make_unique<FloatParam> ("mix", "Mix",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f, "%"));
    p.push_back (std::make_unique<FloatParam> ("output", "Output",
        juce::NormalisableRange<float> (-24.0f, 24.0f, 0.1f), 0.0f, "dB"));

    return { p.begin(), p.end() };
}

RajvikDynamicsProAudioProcessor::RajvikDynamicsProAudioProcessor()
    : AudioProcessor (BusesProperties()
        .withInput ("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

void RajvikDynamicsProAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec { sampleRate, static_cast<juce::uint32> (samplesPerBlock),
                                 static_cast<juce::uint32> (getTotalNumOutputChannels()) };
    compressor.prepare (spec);
    outputGain.prepare (spec);
    dryBuffer.setSize (getTotalNumOutputChannels(), samplesPerBlock);
    compressor.reset();
    outputGain.reset();
}

void RajvikDynamicsProAudioProcessor::releaseResources()
{
    dryBuffer.setSize (0, 0);
}

bool RajvikDynamicsProAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();
    if (in != out)
        return false;
    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

void RajvikDynamicsProAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const auto numSamples = buffer.getNumSamples();
    const auto channels = buffer.getNumChannels();

    if (dryBuffer.getNumChannels() != channels || dryBuffer.getNumSamples() < numSamples)
        dryBuffer.setSize (channels, numSamples, false, false, true);
    dryBuffer.makeCopyOf (buffer, true);

    compressor.setThreshold (parameters.getRawParameterValue ("threshold")->load());
    compressor.setRatio (parameters.getRawParameterValue ("ratio")->load());
    compressor.setAttack (parameters.getRawParameterValue ("attack")->load());
    compressor.setRelease (parameters.getRawParameterValue ("release")->load());

    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> context (block);
    compressor.process (context);

    const auto mix = parameters.getRawParameterValue ("mix")->load() * 0.01f;
    for (int ch = 0; ch < channels; ++ch)
    {
        auto* wet = buffer.getWritePointer (ch);
        const auto* dry = dryBuffer.getReadPointer (ch);
        for (int i = 0; i < numSamples; ++i)
            wet[i] = dry[i] * (1.0f - mix) + wet[i] * mix;
    }

    outputGain.setGainDecibels (parameters.getRawParameterValue ("output")->load());
    outputGain.process (context);
}

juce::AudioProcessorEditor* RajvikDynamicsProAudioProcessor::createEditor()
{
    return new RajvikDynamicsProAudioProcessorEditor (*this);
}

void RajvikDynamicsProAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void RajvikDynamicsProAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (parameters.state.getType()))
        parameters.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new RajvikDynamicsProAudioProcessor();
}
