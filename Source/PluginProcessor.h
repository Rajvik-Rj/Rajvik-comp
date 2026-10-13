#pragma once

#include <JuceHeader.h>
#include <array>
#include <atomic>

class RajvikDynamicsProAudioProcessor final : public juce::AudioProcessor
{
public:
    static constexpr int scopeSize = 256;
    static constexpr int delayCapacity = 4096; // supports 20 ms lookahead through 192 kHz

    RajvikDynamicsProAudioProcessor();
    ~RajvikDynamicsProAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.05; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    using APVTS = juce::AudioProcessorValueTreeState;
    APVTS parameters;

    float getInputLevelDb() const noexcept { return inputLevelDb.load(); }
    float getOutputLevelDb() const noexcept { return outputLevelDb.load(); }
    float getGainReductionDb() const noexcept { return gainReductionDb.load(); }
    int getScopeWriteIndex() const noexcept { return scopeWriteIndex.load(); }
    float getInputScopeSample (int i) const noexcept { return inputScope[static_cast<size_t>(i % scopeSize)].load(); }
    float getOutputScopeSample (int i) const noexcept { return outputScope[static_cast<size_t>(i % scopeSize)].load(); }

private:
    struct Biquad
    {
        float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f;
        float z1 = 0.0f, z2 = 0.0f;
        void reset() noexcept { z1 = z2 = 0.0f; }
        float process (float x) noexcept;
        void setHighPass (double sampleRate, float frequency, float q = 0.7071f) noexcept;
        void setLowPass (double sampleRate, float frequency, float q = 0.7071f) noexcept;
        void setPeak (double sampleRate, float frequency, float q, float gainDb) noexcept;
        void setNormalized (double nb0, double nb1, double nb2, double a0, double a1, double a2) noexcept;
    };

    static APVTS::ParameterLayout createParameterLayout();
    void updateSidechainFilters();
    static float saturate (float x, int character, float driveDb) noexcept;
    static float calculateGainDb (float detectorDb, float thresholdDb, float ratio, float kneeDb,
                                  float rangeDb, int style) noexcept;

    double currentSampleRate = 44100.0;
    std::array<float, 2> envelope { 0.0f, 0.0f };
    std::array<float, 2> rmsState { 0.0f, 0.0f };
    std::array<int, 2> holdRemaining { 0, 0 };
    float longTermLevel = 0.1f;
    std::array<std::array<float, delayCapacity>, 2> delayLine {};
    int delayWriteIndex = 0;

    Biquad sidechainHPF[2];
    Biquad sidechainLPF[2];
    Biquad sidechainBands[2][6];
    float lastHPFFrequency = -1.0f;
    float lastLPFFrequency = -1.0f;
    std::array<float, 6> lastBandGains { 999.0f, 999.0f, 999.0f, 999.0f, 999.0f, 999.0f };
    std::array<float, 6> lastBandFrequencies { -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f };
    std::array<float, 6> lastBandQs { -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f };
    int lastReportedLatency = -1;
    std::atomic<float>* sidechainHPFParam = nullptr;
    std::atomic<float>* sidechainLPFParam = nullptr;
    std::array<std::atomic<float>*, 6> sidechainGainParams {};
    std::array<std::atomic<float>*, 6> sidechainFrequencyParams {};
    std::array<std::atomic<float>*, 6> sidechainQParams {};

    std::array<std::atomic<float>, scopeSize> inputScope {};
    std::array<std::atomic<float>, scopeSize> outputScope {};
    std::atomic<int> scopeWriteIndex { 0 };
    std::atomic<float> inputLevelDb { -100.0f };
    std::atomic<float> outputLevelDb { -100.0f };
    std::atomic<float> gainReductionDb { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RajvikDynamicsProAudioProcessor)
};
