#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>
#include <vector>

namespace
{
    constexpr float tiny = 1.0e-9f;

    float readParam (const juce::AudioProcessorValueTreeState& state, const char* id) noexcept
    {
        return state.getRawParameterValue (id)->load (std::memory_order_relaxed);
    }

    float levelToDb (float level) noexcept
    {
        return juce::Decibels::gainToDecibels (juce::jmax (level, tiny), -120.0f);
    }
}

float RajvikDynamicsProAudioProcessor::Biquad::process (float x) noexcept
{
    const float y = b0 * x + z1;
    z1 = b1 * x - a1 * y + z2;
    z2 = b2 * x - a2 * y;
    return y;
}

void RajvikDynamicsProAudioProcessor::Biquad::setNormalized (double nb0, double nb1, double nb2,
                                                              double a0, double a1In, double a2In) noexcept
{
    if (std::abs (a0) < 1.0e-12)
        return;
    const double invA0 = 1.0 / a0;
    b0 = static_cast<float> (nb0 * invA0);
    b1 = static_cast<float> (nb1 * invA0);
    b2 = static_cast<float> (nb2 * invA0);
    a1 = static_cast<float> (a1In * invA0);
    a2 = static_cast<float> (a2In * invA0);
}

void RajvikDynamicsProAudioProcessor::Biquad::setHighPass (double sampleRate, float frequency, float q) noexcept
{
    const double w = 2.0 * juce::MathConstants<double>::pi * frequency / sampleRate;
    const double c = std::cos (w), s = std::sin (w);
    const double alpha = s / (2.0 * juce::jmax (q, 0.1f));
    setNormalized ((1.0 + c) * 0.5, -(1.0 + c), (1.0 + c) * 0.5,
                   1.0 + alpha, -2.0 * c, 1.0 - alpha);
}

void RajvikDynamicsProAudioProcessor::Biquad::setLowPass (double sampleRate, float frequency, float q) noexcept
{
    const double w = 2.0 * juce::MathConstants<double>::pi * frequency / sampleRate;
    const double c = std::cos (w), s = std::sin (w);
    const double alpha = s / (2.0 * juce::jmax (q, 0.1f));
    setNormalized ((1.0 - c) * 0.5, 1.0 - c, (1.0 - c) * 0.5,
                   1.0 + alpha, -2.0 * c, 1.0 - alpha);
}

void RajvikDynamicsProAudioProcessor::Biquad::setPeak (double sampleRate, float frequency, float q, float gainDb) noexcept
{
    const double w = 2.0 * juce::MathConstants<double>::pi * frequency / sampleRate;
    const double c = std::cos (w), s = std::sin (w);
    const double A = std::pow (10.0, static_cast<double> (gainDb) / 40.0);
    const double alpha = s / (2.0 * juce::jmax (q, 0.1f));
    setNormalized (1.0 + alpha * A, -2.0 * c, 1.0 - alpha * A,
                   1.0 + alpha / A, -2.0 * c, 1.0 - alpha / A);
}

juce::AudioProcessorValueTreeState::ParameterLayout RajvikDynamicsProAudioProcessor::createParameterLayout()
{
    using Float = juce::AudioParameterFloat;
    using Choice = juce::AudioParameterChoice;
    using Bool = juce::AudioParameterBool;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    const juce::StringArray styles {
        "Clean", "Classic", "Opto", "Vocal", "Mastering", "Bus", "Punch", "Pumping",
        "Versatile", "Smooth", "Upward", "TTM", "Op-El", "Vari-Mu"
    };
    p.push_back (std::make_unique<Choice> ("style", "Style", styles, 0));
    p.push_back (std::make_unique<Choice> ("character", "Character",
        juce::StringArray { "Clean", "Tube", "Diode", "Bright" }, 0));
    p.push_back (std::make_unique<Choice> ("detector", "Detector",
        juce::StringArray { "Peak", "RMS" }, 0));
    p.push_back (std::make_unique<Choice> ("sidechainSource", "Sidechain Source",
        juce::StringArray { "Internal", "External" }, 0));

    p.push_back (std::make_unique<Float> ("inputGain", "Input Gain", juce::NormalisableRange<float> (-24.0f, 24.0f, 0.1f), 0.0f, "dB"));
    p.push_back (std::make_unique<Float> ("threshold", "Threshold", juce::NormalisableRange<float> (-60.0f, 0.0f, 0.1f), -18.0f, "dB"));
    p.push_back (std::make_unique<Float> ("ratio", "Ratio", juce::NormalisableRange<float> (1.0f, 100.0f, 0.1f, 0.45f), 4.0f, ":1"));
    p.push_back (std::make_unique<Float> ("attack", "Attack", juce::NormalisableRange<float> (0.01f, 500.0f, 0.01f, 0.35f), 10.0f, "ms"));
    p.push_back (std::make_unique<Float> ("release", "Release", juce::NormalisableRange<float> (1.0f, 5000.0f, 1.0f, 0.35f), 120.0f, "ms"));
    p.push_back (std::make_unique<Float> ("knee", "Knee", juce::NormalisableRange<float> (0.0f, 72.0f, 0.1f), 6.0f, "dB"));
    p.push_back (std::make_unique<Float> ("range", "Range", juce::NormalisableRange<float> (0.0f, 60.0f, 0.1f), 24.0f, "dB"));
    p.push_back (std::make_unique<Float> ("hold", "Hold", juce::NormalisableRange<float> (0.0f, 500.0f, 1.0f), 0.0f, "ms"));
    p.push_back (std::make_unique<Float> ("lookahead", "Lookahead", juce::NormalisableRange<float> (0.0f, 20.0f, 0.1f), 0.0f, "ms"));
    p.push_back (std::make_unique<Float> ("mix", "Mix", juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f, "%"));
    p.push_back (std::make_unique<Float> ("outputGain", "Output Gain", juce::NormalisableRange<float> (-24.0f, 24.0f, 0.1f), 0.0f, "dB"));
    p.push_back (std::make_unique<Float> ("sidechainLevel", "Sidechain Level", juce::NormalisableRange<float> (-24.0f, 24.0f, 0.1f), 0.0f, "dB"));
    p.push_back (std::make_unique<Float> ("sidechainHPF", "Sidechain HPF", juce::NormalisableRange<float> (20.0f, 1000.0f, 1.0f, 0.35f), 25.0f, "Hz"));
    p.push_back (std::make_unique<Float> ("sidechainLPF", "Sidechain LPF", juce::NormalisableRange<float> (1000.0f, 20000.0f, 1.0f, 0.4f), 20000.0f, "Hz"));
    p.push_back (std::make_unique<Float> ("drive", "Drive", juce::NormalisableRange<float> (0.0f, 24.0f, 0.1f), 0.0f, "dB"));
    p.push_back (std::make_unique<Float> ("stereoLink", "Stereo Link", juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f, "%"));

    const float defaultFreqs[6] = { 90.0f, 180.0f, 400.0f, 1000.0f, 3000.0f, 8000.0f };
    for (int i = 0; i < 6; ++i)
    {
        const auto suffix = juce::String (i + 1);
        p.push_back (std::make_unique<Float> ("scBand" + suffix, "Sidechain EQ Gain " + suffix,
                                               juce::NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f, "dB"));
        p.push_back (std::make_unique<Float> ("scFreq" + suffix, "Sidechain EQ Frequency " + suffix,
                                               juce::NormalisableRange<float> (20.0f, 20000.0f, 1.0f, 0.35f), defaultFreqs[i], "Hz"));
        p.push_back (std::make_unique<Float> ("scQ" + suffix, "Sidechain EQ Q " + suffix,
                                               juce::NormalisableRange<float> (0.2f, 8.0f, 0.01f, 0.4f), 1.0f));
    }

    p.push_back (std::make_unique<Bool> ("autoThreshold", "Auto Threshold", false));
    p.push_back (std::make_unique<Bool> ("autoRelease", "Auto Release", true));
    p.push_back (std::make_unique<Bool> ("autoGain", "Auto Gain", false));
    p.push_back (std::make_unique<Bool> ("sidechainEnabled", "Sidechain EQ", true));
    p.push_back (std::make_unique<Bool> ("sidechainListen", "Sidechain Listen", false));
    p.push_back (std::make_unique<Bool> ("characterPre", "Character Pre", false));

    return { p.begin(), p.end() };
}

RajvikDynamicsProAudioProcessor::RajvikDynamicsProAudioProcessor()
    : AudioProcessor (BusesProperties()
        .withInput ("Input", juce::AudioChannelSet::stereo(), true)
        .withInput ("Sidechain", juce::AudioChannelSet::stereo(), false)
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "RAJVIK_PARAMETERS", createParameterLayout())
{
    for (int i = 0; i < scopeSize; ++i)
    {
        inputScope[static_cast<size_t> (i)].store (0.0f, std::memory_order_relaxed);
        outputScope[static_cast<size_t> (i)].store (0.0f, std::memory_order_relaxed);
    }
    sidechainHPFParam = parameters.getRawParameterValue ("sidechainHPF");
    sidechainLPFParam = parameters.getRawParameterValue ("sidechainLPF");
    for (int i = 0; i < 6; ++i)
    {
        const auto suffix = juce::String (i + 1);
        sidechainGainParams[static_cast<size_t> (i)] = parameters.getRawParameterValue ("scBand" + suffix);
        sidechainFrequencyParams[static_cast<size_t> (i)] = parameters.getRawParameterValue ("scFreq" + suffix);
        sidechainQParams[static_cast<size_t> (i)] = parameters.getRawParameterValue ("scQ" + suffix);
    }
}

void RajvikDynamicsProAudioProcessor::prepareToPlay (double sampleRate, int)
{
    currentSampleRate = juce::jmax (sampleRate, 8000.0);
    envelope = { 0.0f, 0.0f };
    rmsState = { 0.0f, 0.0f };
    holdRemaining = { 0, 0 };
    longTermLevel = 0.1f;
    delayWriteIndex = 0;
    for (auto& channel : delayLine)
        channel.fill (0.0f);

    for (int ch = 0; ch < 2; ++ch)
    {
        sidechainHPF[ch].reset();
        sidechainLPF[ch].reset();
        for (int band = 0; band < 6; ++band)
            sidechainBands[ch][band].reset();
    }
    lastHPFFrequency = -1.0f;
    lastLPFFrequency = -1.0f;
    lastBandGains.fill (999.0f);
    lastBandFrequencies.fill (-1.0f);
    lastBandQs.fill (-1.0f);
    lastReportedLatency = -1;
    updateSidechainFilters();
    const int initialLookahead = juce::roundToInt (readParam (parameters, "lookahead") * 0.001f * static_cast<float> (currentSampleRate));
    setLatencySamples (initialLookahead);
    lastReportedLatency = initialLookahead;
}

void RajvikDynamicsProAudioProcessor::releaseResources()
{
    for (int ch = 0; ch < 2; ++ch)
    {
        sidechainHPF[ch].reset();
        sidechainLPF[ch].reset();
        for (int band = 0; band < 6; ++band)
            sidechainBands[ch][band].reset();
    }
}

bool RajvikDynamicsProAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();
    if (in != out)
        return false;
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return true;
}

void RajvikDynamicsProAudioProcessor::updateSidechainFilters()
{
    const auto sr = currentSampleRate;
    const float hp = juce::jlimit (20.0f, static_cast<float> (sr * 0.40), sidechainHPFParam->load (std::memory_order_relaxed));
    const float lp = juce::jlimit (1000.0f, static_cast<float> (sr * 0.45), sidechainLPFParam->load (std::memory_order_relaxed));
    const bool updateHPF = std::abs (hp - lastHPFFrequency) > 0.01f;
    const bool updateLPF = std::abs (lp - lastLPFFrequency) > 0.01f;

    if (updateHPF || updateLPF)
    {
        for (int ch = 0; ch < 2; ++ch)
        {
            if (updateHPF)
                sidechainHPF[ch].setHighPass (sr, hp);
            if (updateLPF)
                sidechainLPF[ch].setLowPass (sr, lp);
        }
        lastHPFFrequency = hp;
        lastLPFFrequency = lp;
    }

    for (int band = 0; band < 6; ++band)
    {
        const auto ix = static_cast<size_t> (band);
        const float gainDb = sidechainGainParams[ix]->load (std::memory_order_relaxed);
        const float freq = juce::jlimit (20.0f, static_cast<float> (sr * 0.42), sidechainFrequencyParams[ix]->load (std::memory_order_relaxed));
        const float q = sidechainQParams[ix]->load (std::memory_order_relaxed);
        if (std::abs (gainDb - lastBandGains[ix]) > 0.01f || std::abs (freq - lastBandFrequencies[ix]) > 0.5f
            || std::abs (q - lastBandQs[ix]) > 0.01f)
        {
            for (int ch = 0; ch < 2; ++ch)
                sidechainBands[ch][band].setPeak (sr, freq, q, gainDb);
            lastBandGains[ix] = gainDb;
            lastBandFrequencies[ix] = freq;
            lastBandQs[ix] = q;
        }
    }
}

float RajvikDynamicsProAudioProcessor::saturate (float x, int character, float driveDb) noexcept
{
    if (character == 0 || driveDb <= 0.001f)
        return x;

    // Parallel, level-aware waveshaping: zero drive is an exact bypass and mild settings stay subtle.
    const float amount = juce::jlimit (0.0f, 1.0f, driveDb / 24.0f);
    const float drive = 1.0f + driveDb / 3.0f;
    float shaped = x;

    if (character == 1) // Tube-like asymmetric curve with even-harmonic character
    {
        constexpr float bias = 0.035f;
        shaped = std::tanh (drive * (x + bias)) - std::tanh (drive * bias);
    }
    else if (character == 2) // Diode-like, firmer asymmetrical clipping
    {
        const float bias = x >= 0.0f ? 0.025f : -0.055f;
        shaped = std::tanh (drive * (x + bias));
    }
    else // Bright: stronger upper harmonics with a soft safety curve
    {
        const float driven = x * drive;
        shaped = std::tanh (driven + 0.10f * std::sin (3.0f * driven));
    }

    return x + (shaped - x) * amount;
}

float RajvikDynamicsProAudioProcessor::calculateGainDb (float detectorDb, float thresholdDb, float ratio,
                                                         float kneeDb, float rangeDb, int style) noexcept
{
    ratio = juce::jmax (ratio, 1.0f);
    // Style-specific transfer/time signatures: intentionally distinct DSP behaviors, not renamed presets.
    if (style == 2) ratio = 1.0f + (ratio - 1.0f) * 0.72f; // Opto
    if (style == 4) ratio = 1.0f + (ratio - 1.0f) * 0.65f; // Mastering
    if (style == 9) ratio = 1.0f + (ratio - 1.0f) * 0.80f; // Smooth
    if (style == 12) ratio = 1.0f + (ratio - 1.0f) * 0.82f; // Op-El
    if (style == 13) ratio = 1.0f + (ratio - 1.0f) * 0.75f; // Vari-Mu

    const float over = detectorDb - thresholdDb;
    float gainDb = 0.0f;

    if (style == 10) // Upward compression only: lift material below threshold, leave louder material alone.
    {
        if (over < 0.0f)
            return juce::jmin (rangeDb, (-over) * (1.0f - 1.0f / ratio));
        return 0.0f;
    }
    if (style == 11 && over < 0.0f) // TTM-style: upward below threshold, downward above
        return juce::jmin (rangeDb, (-over) * (1.0f - 1.0f / ratio));

    if (kneeDb > 0.0001f && std::abs (over) < kneeDb * 0.5f)
    {
        const float shifted = over + kneeDb * 0.5f;
        gainDb = (1.0f / ratio - 1.0f) * shifted * shifted / (2.0f * kneeDb);
    }
    else if (over >= 0.0f)
    {
        gainDb = over * (1.0f / ratio - 1.0f);
    }

    if (style == 11 && over >= 0.0f)
        gainDb = over * (1.0f / ratio - 1.0f);

    return juce::jmax (gainDb, -rangeDb);
}

void RajvikDynamicsProAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    auto mainInput = getBusBuffer (buffer, true, 0);
    auto mainOutput = getBusBuffer (buffer, false, 0);
    auto sidechainInput = getBusBuffer (buffer, true, 1);

    const int channels = juce::jmin (mainOutput.getNumChannels(), 2);
    const int samples = mainOutput.getNumSamples();
    if (channels == 0 || samples == 0)
        return;

    const int style = juce::roundToInt (readParam (parameters, "style"));
    const int character = juce::roundToInt (readParam (parameters, "character"));
    const int detectorType = juce::roundToInt (readParam (parameters, "detector"));
    const bool externalChosen = readParam (parameters, "sidechainSource") > 0.5f;
    const bool useExternal = externalChosen && sidechainInput.getNumChannels() > 0;
    const bool useScEq = readParam (parameters, "sidechainEnabled") > 0.5f;
    const bool sidechainListen = readParam (parameters, "sidechainListen") > 0.5f;
    const bool autoThreshold = readParam (parameters, "autoThreshold") > 0.5f;
    const bool autoRelease = readParam (parameters, "autoRelease") > 0.5f;
    const bool autoGain = readParam (parameters, "autoGain") > 0.5f;
    const bool characterPre = readParam (parameters, "characterPre") > 0.5f;

    const float inputGain = juce::Decibels::decibelsToGain (readParam (parameters, "inputGain"));
    const float outputGain = juce::Decibels::decibelsToGain (readParam (parameters, "outputGain"));
    const float threshold = readParam (parameters, "threshold");
    const float ratio = readParam (parameters, "ratio");
    const float baseAttack = readParam (parameters, "attack");
    const float baseRelease = readParam (parameters, "release");
    const float knee = readParam (parameters, "knee");
    const float range = readParam (parameters, "range");
    const float holdMs = readParam (parameters, "hold");
    const int lookaheadSamples = juce::jlimit (0, delayCapacity - 2,
        juce::roundToInt (readParam (parameters, "lookahead") * 0.001f * static_cast<float> (currentSampleRate)));
    if (lookaheadSamples != lastReportedLatency)
    {
        setLatencySamples (lookaheadSamples);
        lastReportedLatency = lookaheadSamples;
    }
    const float mix = readParam (parameters, "mix") * 0.01f;
    const float stereoLink = readParam (parameters, "stereoLink") * 0.01f;
    const float scGain = juce::Decibels::decibelsToGain (readParam (parameters, "sidechainLevel"));
    const float drive = readParam (parameters, "drive");

    float styleAttack = 1.0f, styleRelease = 1.0f;
    switch (style)
    {
        case 0:  styleAttack = 0.85f; styleRelease = 0.95f; break; // Clean
        case 1:  styleAttack = 1.0f; styleRelease = 1.0f; break; // Classic
        case 2:  styleAttack = 2.0f; styleRelease = 2.2f; break; // Opto
        case 3:  styleAttack = 0.75f; styleRelease = 1.1f; break; // Vocal
        case 4:  styleAttack = 1.5f; styleRelease = 2.4f; break; // Mastering
        case 5:  styleAttack = 1.15f; styleRelease = 1.35f; break; // Bus
        case 6:  styleAttack = 2.8f; styleRelease = 0.65f; break; // Punch
        case 7:  styleAttack = 0.15f; styleRelease = 0.55f; break; // Pumping
        case 8:  styleAttack = 1.0f; styleRelease = 1.2f; break; // Versatile
        case 9:  styleAttack = 1.8f; styleRelease = 2.8f; break; // Smooth
        case 12: styleAttack = 2.2f; styleRelease = 1.8f; break; // Op-El
        case 13: styleAttack = 1.7f; styleRelease = 2.2f; break; // Vari-Mu
        default: break;
    }
    if (autoRelease)
    {
        const float activity = juce::jlimit (0.65f, 2.2f, 1.15f + (levelToDb (longTermLevel) + 24.0f) * 0.015f);
        styleRelease *= activity;
    }
    const float attackMs = juce::jlimit (0.01f, 500.0f, baseAttack * styleAttack);
    const float releaseMs = juce::jlimit (1.0f, 5000.0f, baseRelease * styleRelease);
    const float attackCoef = std::exp (-1.0f / (static_cast<float> (currentSampleRate) * attackMs * 0.001f));
    const float releaseCoef = std::exp (-1.0f / (static_cast<float> (currentSampleRate) * releaseMs * 0.001f));
    const float rmsCoef = std::exp (-1.0f / (static_cast<float> (currentSampleRate) * 0.010f));
    const int holdSamples = juce::roundToInt (holdMs * 0.001f * static_cast<float> (currentSampleRate));
    const float autoCoef = std::exp (-1.0f / (static_cast<float> (currentSampleRate) * 1.5f));
    const float autoMakeup = autoGain ? juce::jlimit (0.0f, 12.0f, -threshold * (1.0f - 1.0f / juce::jmax (ratio, 1.0f)) * 0.25f) : 0.0f;
    const float autoMakeupGain = juce::Decibels::decibelsToGain (autoMakeup);

    updateSidechainFilters();

    float peakIn = 0.0f, peakOut = 0.0f, maxGr = 0.0f;
    int scopeIndex = scopeWriteIndex.load (std::memory_order_relaxed);
    const int scopeStride = juce::jmax (1, samples / 48);
    const bool mono = channels == 1;

    for (int i = 0; i < samples; ++i)
    {
        float input[2] { 0.0f, 0.0f };
        float main[2] { 0.0f, 0.0f };
        float side[2] { 0.0f, 0.0f };
        float detector[2] { 0.0f, 0.0f };
        float detectorLevel[2] { 0.0f, 0.0f };

        for (int ch = 0; ch < channels; ++ch)
        {
            input[ch] = mainInput.getSample (ch, i) * inputGain;
            main[ch] = characterPre ? saturate (input[ch], character, drive) : input[ch];
            side[ch] = useExternal
                ? sidechainInput.getSample (juce::jmin (ch, sidechainInput.getNumChannels() - 1), i) * scGain
                : main[ch];
            if (useScEq)
            {
                side[ch] = sidechainHPF[ch].process (side[ch]);
                for (int band = 0; band < 6; ++band)
                    side[ch] = sidechainBands[ch][band].process (side[ch]);
                side[ch] = sidechainLPF[ch].process (side[ch]);
            }
            detector[ch] = side[ch];
            const float detectMagnitude = std::abs (detector[ch]);
            rmsState[static_cast<size_t> (ch)] = rmsCoef * rmsState[static_cast<size_t> (ch)]
                + (1.0f - rmsCoef) * detectMagnitude * detectMagnitude;
            const float chosen = detectorType == 1 ? std::sqrt (juce::jmax (rmsState[static_cast<size_t> (ch)], 0.0f)) : detectMagnitude;

            if (chosen > envelope[static_cast<size_t> (ch)])
            {
                envelope[static_cast<size_t> (ch)] = attackCoef * envelope[static_cast<size_t> (ch)] + (1.0f - attackCoef) * chosen;
                holdRemaining[static_cast<size_t> (ch)] = holdSamples;
            }
            else if (holdRemaining[static_cast<size_t> (ch)] > 0)
            {
                --holdRemaining[static_cast<size_t> (ch)];
            }
            else
            {
                envelope[static_cast<size_t> (ch)] = releaseCoef * envelope[static_cast<size_t> (ch)] + (1.0f - releaseCoef) * chosen;
            }
            detectorLevel[ch] = envelope[static_cast<size_t> (ch)];
            peakIn = juce::jmax (peakIn, std::abs (input[ch]));
        }

        if (mono)
            detectorLevel[1] = detectorLevel[0];

        float longLevel = 0.5f * (detectorLevel[0] + detectorLevel[1]);
        longTermLevel = autoCoef * longTermLevel + (1.0f - autoCoef) * longLevel;
        const float longLevelDb = levelToDb (longTermLevel);
        const float adaptiveThreshold = autoThreshold
            ? threshold + juce::jlimit (-12.0f, 12.0f, (longLevelDb + 18.0f) * 0.65f)
            : threshold;

        const float dbL = levelToDb (detectorLevel[0]);
        const float dbR = levelToDb (detectorLevel[1]);
        const float linkedDb = juce::jmax (dbL, dbR);
        const float effectiveDbL = dbL + (linkedDb - dbL) * stereoLink;
        const float effectiveDbR = dbR + (linkedDb - dbR) * stereoLink;
        float grDb[2] {
            calculateGainDb (effectiveDbL, adaptiveThreshold, ratio, knee, range, style),
            calculateGainDb (effectiveDbR, adaptiveThreshold, ratio, knee, range, style)
        };
        if (mono)
            grDb[1] = grDb[0];
        maxGr = juce::jmax (maxGr, juce::jmax (std::abs (grDb[0]), std::abs (grDb[1])));

        for (int ch = 0; ch < channels; ++ch)
        {
            delayLine[static_cast<size_t> (ch)][static_cast<size_t> (delayWriteIndex)] = main[ch];
            const int readIndex = (delayWriteIndex - lookaheadSamples + delayCapacity) % delayCapacity;
            const float delayed = delayLine[static_cast<size_t> (ch)][static_cast<size_t> (readIndex)];

            if (sidechainListen)
            {
                mainOutput.setSample (ch, i, side[ch]);
                peakOut = juce::jmax (peakOut, std::abs (side[ch]));
                continue;
            }

            const float compressorGain = juce::Decibels::decibelsToGain (grDb[ch]) * autoMakeupGain;
            const float wet = delayed * compressorGain;
            float processed = delayed * (1.0f - mix) + wet * mix;
            if (! characterPre)
                processed = saturate (processed, character, drive);
            processed *= outputGain;
            mainOutput.setSample (ch, i, processed);
            peakOut = juce::jmax (peakOut, std::abs (processed));
        }

        delayWriteIndex = (delayWriteIndex + 1) % delayCapacity;
        if (i % scopeStride == 0)
        {
            inputScope[static_cast<size_t> (scopeIndex % scopeSize)].store (juce::jlimit (-1.0f, 1.0f, input[0]), std::memory_order_relaxed);
            outputScope[static_cast<size_t> (scopeIndex % scopeSize)].store (
                juce::jlimit (-1.0f, 1.0f, mainOutput.getSample (0, i)), std::memory_order_relaxed);
            ++scopeIndex;
        }
    }

    scopeWriteIndex.store (scopeIndex % scopeSize, std::memory_order_relaxed);
    inputLevelDb.store (levelToDb (peakIn), std::memory_order_relaxed);
    outputLevelDb.store (levelToDb (peakOut), std::memory_order_relaxed);
    gainReductionDb.store (maxGr, std::memory_order_relaxed);
}

juce::AudioProcessorEditor* RajvikDynamicsProAudioProcessor::createEditor()
{
    return new RajvikDynamicsProAudioProcessorEditor (*this);
}

void RajvikDynamicsProAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    const auto state = parameters.copyState();
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
