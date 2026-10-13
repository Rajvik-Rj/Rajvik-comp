#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "DSP/CompressionAlgorithms.h"

namespace rdp {
inline juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    using F = juce::AudioParameterFloat; using NR = juce::NormalisableRange<float>;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    auto addF = [&](const char* id, const char* name, NR r, float def, const char* unit = "") {
        p.push_back(std::make_unique<F>(juce::ParameterID{ id, 1 }, name, r, def, juce::AudioParameterFloatAttributes().withLabel(unit))); };
    auto skew = [](float lo, float hi, float centre) { NR r(lo, hi); r.setSkewForCentre(centre); return r; };
    auto addB = [&](const char* id, const char* name, bool def) {
        p.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{ id, 1 }, name, def)); };
    auto addC = [&](const char* id, const char* name, juce::StringArray items, int def) {
        p.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{ id, 1 }, name, items, def)); };

    juce::StringArray modes; for (auto& m : kModes) modes.add(m.name);
    addC("mode", "Style", modes, 0);
    addF("threshold", "Threshold", NR(-60.f, 0.f, 0.1f), -18.f, " dB");
    addF("ratio", "Ratio", skew(1.f, 100.f, 6.f), 4.f, ":1");
    addF("attack", "Attack", skew(0.01f, 500.f, 10.f), 10.f, " ms");
    addF("release", "Release", skew(1.f, 5000.f, 150.f), 120.f, " ms");
    addF("mix", "Mix", NR(0.f, 100.f, 0.1f), 100.f, " %");
    addF("knee", "Knee", NR(0.f, 72.f, 0.1f), 6.f, " dB");
    addF("range", "Range", NR(0.f, 60.f, 0.1f), 40.f, " dB");
    addF("hold", "Hold", NR(0.f, 500.f, 0.1f), 0.f, " ms");
    addF("lookahead", "Lookahead", NR(0.f, 20.f, 0.1f), 0.f, " ms");
    addF("link", "Link", NR(0.f, 100.f, 1.f), 100.f, " %");
    addF("inGain", "Input", NR(-24.f, 24.f, 0.1f), 0.f, " dB");
    addF("outGain", "Output", NR(-24.f, 24.f, 0.1f), 0.f, " dB");
    addB("autoGain", "Auto Gain", false);
    addB("autoRelease", "Auto Release", false);
    addC("detector", "Detector", { "Style", "Peak", "RMS" }, 0);
    addC("character", "Character", { "Clean", "Tube", "Diode", "Bright" }, 0);
    addF("drive", "Drive", NR(0.f, 24.f, 0.1f), 0.f, " dB");
    addC("oversampling", "Oversampling", { "Off", "2x", "4x", "8x", "16x", "32x" }, 0);
    addB("scExternal", "External SC", false);
    addB("scListen", "SC Listen", false);
    addF("scHpf", "SC HPF", skew(20.f, 1000.f, 120.f), 20.f, " Hz");
    addC("scHpfSlope", "SC HPF Slope", { "12 dB", "24 dB", "48 dB" }, 0);
    addF("scLpf", "SC LPF", skew(1000.f, 20000.f, 6000.f), 20000.f, " Hz");
    addF("scLowFreq", "SC Low Freq", skew(20.f, 1000.f, 120.f), 100.f, " Hz");
    addF("scLowGain", "SC Low Gain", NR(-18.f, 18.f, 0.1f), 0.f, " dB");
    addF("scBellFreq", "SC Bell Freq", skew(100.f, 10000.f, 1000.f), 1000.f, " Hz");
    addF("scBellGain", "SC Bell Gain", NR(-18.f, 18.f, 0.1f), 0.f, " dB");
    addF("scBellQ", "SC Bell Q", skew(0.2f, 10.f, 1.f), 1.f, "");
    addF("scHighFreq", "SC High Freq", skew(1000.f, 20000.f, 6000.f), 6000.f, " Hz");
    addF("scHighGain", "SC High Gain", NR(-18.f, 18.f, 0.1f), 0.f, " dB");
    addC("stereoMode", "Stereo Mode", { "Stereo", "Mid/Side" }, 0);
    addB("syncOn", "Tempo Sync", false);
    addC("syncRate", "Sync Rate", { "1/1", "1/2", "1/4", "1/8", "1/16", "1/32" }, 2);
    addF("syncOffset", "Sync Offset", NR(0.f, 100.f, 0.1f), 0.f, " %");
    addF("syncPulse", "Sync Pulse", NR(1.f, 100.f, 0.1f), 25.f, " %");
    return { p.begin(), p.end() };
}
} // namespace rdp
