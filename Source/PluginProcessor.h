#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>
#include "ParameterDefinitions.h"
#include "PresetManager.h"
#include "DSP/CompressorEngine.h"
#include "DSP/SidechainEQ.h"
#include "DSP/MultibandEngine.h"
#include "DSP/SaturationProcessor.h"
#include "DSP/MeteringEngine.h"

// Lock-free ring of per-block meter frames (audio thread writes, GUI reads)
struct MeterRing {
    static constexpr int N = 1024;
    std::array<std::atomic<float>, N> in, out, gr;
    std::atomic<unsigned> w{ 0 };
    void push(float i, float o, float g) noexcept {
        const unsigned idx = w.load(std::memory_order_relaxed) % N;
        in[idx].store(i, std::memory_order_relaxed); out[idx].store(o, std::memory_order_relaxed); gr[idx].store(g, std::memory_order_relaxed);
        w.store(w.load(std::memory_order_relaxed) + 1, std::memory_order_release);
    }
};

// Mono sample ring read by the GUI's FFT analyzers (audio thread only writes; only filled while an editor is open)
struct AnalyzerFeed {
    static constexpr int N = 4096;
    std::array<std::atomic<float>, N> buf{}; std::atomic<unsigned> w{ 0 };
};

class RajvikProcessor : public juce::AudioProcessor {
public:
    RajvikProcessor();
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "RAJVIK DYNAMICS PRO"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    void toggleAB();
    int currentAB() const { return abCurrent; }
    void copyToOtherAB() { abState[abCurrent ^ 1] = apvts.copyState(); }   // A/B "Copy": current slot -> the other slot
    void copySettings() { clipboard = apvts.copyState(); }
    void pasteSettings() { if (clipboard.isValid()) apvts.replaceState(clipboard.createCopy()); }
    void applyModeDefaults();

    // MIDI learn (CC -> parameter). GUI thread calls these; audio thread only reads atomics.
    void beginLearn(const juce::String& paramId) { learnParam.store(apvts.getParameter(paramId)); }
    void cancelLearn() { learnParam.store(nullptr); }
    void clearMidiMap() { for (auto& c : ccMap) c.store(nullptr); mapDirty = true; }
    void syncMidiMap();   // called from GUI timer: persists/restores the map in plugin state

    juce::UndoManager undo;
    juce::AudioProcessorValueTreeState apvts;
    rdp::PresetManager presets;
    MeterRing meters;
    AnalyzerFeed anIn, anOut, anDet;   // input, final output, and the detector signal after sidechain EQ
    // loudness / peak meters (only computed while an editor is open)
    std::atomic<float> mMom{ -100.f }, mShort{ -100.f }, mInt{ -100.f }, mTp{ -100.f }, mRmsIn{ -100.f }, mRmsOut{ -100.f };
    std::atomic<bool> metersWanted{ false }, meterReset{ false };
    // multiband display feed (per band gain change in dB, band input level in dB) - written by the audio thread, read by the GUI
    std::array<std::atomic<float>, 4> mbGr{}, mbLvl{};
    // detector level after the sidechain EQ (dB, peak) and the threshold actually in use (differs from the knob when Auto Threshold is on)
    std::atomic<float> scMeter{ -100.f }, effThr{ -18.f }, extPk{ 0.f };   // extPk: peak of the external side chain input bus
private:
    struct Prm {
        std::atomic<float> *mode{}, *thr{}, *ratio{}, *atk{}, *rel{}, *mix{}, *knee{}, *range{}, *hold{}, *look{}, *link{}, *inG{}, *outG{}, *autoGain{}, *autoRel{},
            *det{}, *chr{}, *drive{}, *os{}, *stMode{}, *procMid{}, *procSide{}, *syncOn{}, *syncRate{}, *syncOff{}, *syncPulse{},
            *dry{}, *wetPan{}, *dryPan{}, *autoThr{}, *charPost{}, *lookOn{}, *scOn{}, *scExt{}, *scRoute{}, *scLevel{}, *scListen{}, *scInvert{}, *scEqOn{}, *eqBands{}, *mbOn{}, *mbBands{}, *mbX[3]{};
    } prm;
    struct EqP { std::atomic<float> *on{}, *type{}, *freq{}, *gain{}, *q{}, *slope{}, *ms{}; } eqP[rdp::kEqBands];
    struct MbP { std::atomic<float> *thr{}, *ratio{}, *atk{}, *rel{}, *gain{}, *solo{}, *mute{}, *byp{}, *mode{}; } mbP[4];

    void processChunk(juce::AudioBuffer<float>& main, int start, int n, int nCh, const float* const* sc, int scCh);

    rdp::CompressorEngine engine; rdp::MultibandEngine mb; rdp::SidechainEQ eq; rdp::SaturationProcessor sat; rdp::MeteringEngine meter;
    rdp::DelayLine lookDelay[2], rawLook[2], dryDelay[2], wetDelay[2];
    juce::AudioBuffer<float> wet, dry, inBuf, pre;
    // auto threshold / MIDI trigger / character routing / gain-change scale
    double autoPow = 0.0158; float gcScale = 1.f; bool preMode = false, trigMidi = false, gate = false; int heldNotes = 0;
    struct GateEv { int pos; bool on; }; GateEv gateEv[128]; int nGateEv = 0, gateIdx = 0;
    std::array<std::unique_ptr<juce::dsp::Oversampling<float>>, 5> os;
    std::array<int, 5> osLat{};
    int lastOs = -2, maxBlock = 512;
    double sr = 44100;
    juce::SmoothedValue<float> inSm, outSm, mixSm, mkSm, mixAllSm;
    int lookS = 0, osIdx = -1, osLatNow = 0, chrIdx = 0, routeMode = 0; bool listen = false, extSc = false, msOn = false, autoOn = false;
    unsigned anWr = 0, anWrOut = 0; bool mbActive = false, mbShared = false, scOnNow = true, eqRun = false, procCh[2] = { true, true }; float detGain = 1.f;
    // tempo sync
    bool syncActive = false; double syncPpq0 = 0, ppqPerSample = 0, syncBeats = 1, syncOff = 0, syncPulse = 0.25;
    double pIn = 0, pOut = 0;   // slow power averages for loudness-matched auto gain
    // MIDI
    std::array<std::atomic<juce::RangedAudioParameter*>, 128> ccMap;
    std::atomic<juce::RangedAudioParameter*> learnParam{ nullptr };
    std::atomic<bool> mapDirty{ false }; juce::String midiMapStr;
    juce::ValueTree abState[2], clipboard; int abCurrent = 0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RajvikProcessor)
};
