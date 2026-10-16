#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "DSP/CompressionAlgorithms.h"
#include "DSP/SidechainEQ.h"
#include "DSP/MultibandEngine.h"

namespace rdp {
inline juce::String freqText(float v) { return v >= 1000.f ? juce::String(v / 1000.f, 1) + " kHz" : juce::String(juce::roundToInt(v)) + " Hz"; }
inline float freqFromText(const juce::String& t) { auto s = t.trim().toLowerCase(); float v = s.getFloatValue(); if (s.contains("k")) v *= 1000.f; return v; }

inline juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    using F = juce::AudioParameterFloat; using NR = juce::NormalisableRange<float>;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    // unit is appended to the displayed text ("-18.0 dB", "4.0:1"); dec < 0 = adaptive (2 decimals below 1.0)
    auto addF = [&](const juce::String& id, const juce::String& name, NR r, float def, const juce::String& unit = {}, int dec = 1) {
        auto attr = juce::AudioParameterFloatAttributes()
            .withStringFromValueFunction([unit, dec](float v, int) { return juce::String(v, dec < 0 ? (std::abs(v) < 0.995f ? 2 : 1) : dec) + unit; })
            .withValueFromStringFunction([](const juce::String& t) { return t.getFloatValue(); });
        p.push_back(std::make_unique<F>(juce::ParameterID{ id, 1 }, name, r, def, attr)); };
    auto addFreq = [&](const juce::String& id, const juce::String& name, float def) {
        NR r(20.f, 20000.f, 0.f); r.setSkewForCentre(1000.f);
        auto attr = juce::AudioParameterFloatAttributes().withStringFromValueFunction([](float v, int) { return freqText(v); })
            .withValueFromStringFunction([](const juce::String& t) { return freqFromText(t); });
        p.push_back(std::make_unique<F>(juce::ParameterID{ id, 1 }, name, r, def, attr)); };
    auto skew = [](float lo, float hi, float centre) { NR r(lo, hi); r.setSkewForCentre(centre); return r; };
    auto addB = [&](const juce::String& id, const juce::String& name, bool def) {
        p.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{ id, 1 }, name, def)); };
    auto addC = [&](const juce::String& id, const juce::String& name, juce::StringArray items, int def) {
        p.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{ id, 1 }, name, items, def)); };

    juce::StringArray modes; for (auto& m : kModes) modes.add(m.name);
    addC("mode", "Style", modes, 0);
    addF("threshold", "Threshold", NR(-60.f, 0.f, 0.1f), -18.f, " dB");
    addF("ratio", "Ratio", skew(1.f, 100.f, 6.f), 4.f, ":1");
    addF("attack", "Attack", skew(0.01f, 500.f, 10.f), 10.f, " ms", -1);
    addF("release", "Release", skew(1.f, 5000.f, 150.f), 120.f, " ms");
    addF("mix", "Mix", NR(0.f, 100.f, 1.f), 100.f, " %", 0);        // overall dry/wet after everything: 0 % = unprocessed, 100 % = fully processed
    addF("dry", "Dry Gain", NR(-60.f, 12.f, 0.1f), -60.f, " dB");  // dry level for parallel compression (-60 = off)
    addF("wetPan", "Wet Pan", NR(-100.f, 100.f, 1.f), 0.f, " %", 0);   // Mid/Side mode: -100 = Mid only, +100 = Side only
    addF("dryPan", "Dry Pan", NR(-100.f, 100.f, 1.f), 0.f, " %", 0);
    addB("autoThr", "Auto Threshold", false);                      // threshold follows the signal level
    addB("charPost", "Character Post", true);                      // true = saturation after compression, false = before
    addB("lookOn", "Lookahead On", true);
    addF("knee", "Knee", NR(0.f, 72.f, 0.1f), 6.f, " dB");
    addF("range", "Range", NR(0.f, 60.f, 0.1f), 40.f, " dB");
    addF("hold", "Hold", NR(0.f, 500.f, 0.1f), 0.f, " ms", 0);
    addF("lookahead", "Lookahead", NR(0.f, 20.f, 0.1f), 0.f, " ms");
    addF("link", "Link", NR(0.f, 100.f, 1.f), 100.f, " %", 0);
    addF("inGain", "Input", NR(-24.f, 24.f, 0.1f), 0.f, " dB");
    addF("outGain", "Output", NR(-24.f, 24.f, 0.1f), 0.f, " dB");
    addB("autoGain", "Auto Gain", false);
    addB("autoRelease", "Auto Release", false);
    addC("detector", "Detector", { "Style", "Peak", "RMS" }, 0);
    addC("character", "Character", { "Clean", "Tube", "Diode", "Bright" }, 0);
    addF("drive", "Drive", NR(0.f, 24.f, 0.1f), 0.f, " dB");
    addC("oversampling", "Oversampling", { "Off", "2x", "4x", "8x", "16x", "32x" }, 0);
    addC("stereoMode", "Stereo Mode", { "Stereo", "Mid/Side" }, 0);
    addB("procMid", "Process Mid", true);
    addB("procSide", "Process Side", true);
    addB("syncOn", "Tempo Sync", false);
    addC("syncRate", "Sync Rate", { "1/1", "1/2", "1/4", "1/8", "1/16", "1/32" }, 2);
    addF("syncOffset", "Sync Offset", NR(0.f, 100.f, 0.1f), 0.f, " %");
    addF("syncPulse", "Sync Pulse", NR(1.f, 100.f, 0.1f), 25.f, " %");

    // ---- sidechain section ----
    addB("scOn", "Sidechain Processing", true);
    addC("scExternal", "Sidechain Source", { "Internal", "External", "MIDI" }, 0);
    addC("scRoute", "Sidechain Routing", { "Stereo", "Mid", "Side" }, 0);
    addF("scLevel", "Sidechain Level", NR(-24.f, 24.f, 0.1f), 0.f, " dB");
    addB("scListen", "SC Listen", false);
    addB("scInvert", "SC Invert", false);
    addC("scEqOn", "Sidechain EQ", { "Off", "Post Filter" }, 1);
    addC("eqBands", "EQ Bands", { "2 Bands", "3 Bands", "4 Bands", "5 Bands", "6 Bands" }, 4);
    static const int   eqTypeDef[kEqBands] = { EqHPF, EqLowShelf, EqBell, EqBell, EqBell, EqHighShelf };
    static const float eqFreqDef[kEqBands] = { 20.f, 120.f, 500.f, 2000.f, 5000.f, 12000.f };
    static const float eqQDef[kEqBands]    = { 0.71f, 0.71f, 1.f, 1.f, 1.f, 0.71f };
    for (int i = 0; i < kEqBands; ++i) {
        const auto n = juce::String(i + 1);
        addB("eqOn" + n, "EQ " + n + " On", true);
        addC("eqType" + n, "EQ " + n + " Type", { "HPF", "Low Shelf", "Bell", "High Shelf", "LPF", "All-Pass" }, eqTypeDef[i]);
        addFreq("eqFreq" + n, "EQ " + n + " Freq", eqFreqDef[i]);
        addF("eqGain" + n, "EQ " + n + " Gain", NR(-18.f, 18.f, 0.1f), 0.f, " dB");
        addF("eqQ" + n, "EQ " + n + " Q", skew(0.1f, 10.f, 1.f), eqQDef[i], "", 2);
        addC("eqSlope" + n, "EQ " + n + " Slope", { "12 dB", "24 dB", "48 dB", "96 dB" }, 1);
        addC("eqMs" + n, "EQ " + n + " Channel", { "Stereo", "Mid", "Side" }, 0);
    }

    // ---- multiband (up to 4 bands) ----
    addB("mbOn", "Multiband", false);
    addC("mbBands", "MB Bands", { "2 Bands", "3 Bands", "4 Bands" }, 2);
    static const float xDef[3] = { 150.f, 1500.f, 6000.f };
    for (int j = 0; j < 3; ++j) addFreq("mbX" + juce::String(j + 1), "MB Crossover " + juce::String(j + 1), xDef[j]);
    static const float atkDef[4] = { 30.f, 15.f, 8.f, 4.f }, relDef[4] = { 200.f, 150.f, 110.f, 80.f };
    for (int b = 0; b < 4; ++b) {
        const auto n = juce::String(b + 1), nm = "Band " + n + " ";
        addF("mbThr" + n, nm + "Threshold", NR(-60.f, 0.f, 0.1f), -18.f, " dB");
        addF("mbRatio" + n, nm + "Ratio", skew(1.f, 100.f, 6.f), 3.f, ":1");
        addF("mbAtk" + n, nm + "Attack", skew(0.01f, 500.f, 10.f), atkDef[b], " ms", -1);
        addF("mbRel" + n, nm + "Release", skew(1.f, 5000.f, 150.f), relDef[b], " ms");
        addF("mbGain" + n, nm + "Gain", NR(-18.f, 18.f, 0.1f), 0.f, " dB");
        { juce::StringArray sl; sl.add("Main style"); sl.addArray(modes); addC("mbMode" + n, nm + "Style", sl, 0); }   // each band can use its own compressor style
        addB("mbSolo" + n, nm + "Solo", false);
        addB("mbMute" + n, nm + "Mute", false);
        addB("mbByp" + n, nm + "Bypass", false);
    }
    return { p.begin(), p.end() };
}
} // namespace rdp
