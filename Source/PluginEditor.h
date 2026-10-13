#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <vector>

class RajvikDynamicsProAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                                      private juce::Timer
{
public:
    explicit RajvikDynamicsProAudioProcessorEditor (RajvikDynamicsProAudioProcessor&);
    ~RajvikDynamicsProAudioProcessorEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    struct Knob
    {
        juce::String id;
        std::unique_ptr<juce::Slider> slider;
        std::unique_ptr<juce::Label> label;
        std::unique_ptr<SliderAttachment> attachment;
    };

    RajvikDynamicsProAudioProcessor& audioProcessor;
    std::vector<std::unique_ptr<Knob>> knobs;

    juce::Label brandLabel, subtitleLabel, styleLabel, presetLabel, characterLabel;
    juce::ComboBox styleCombo, presetCombo, characterCombo, detectorCombo, sidechainSourceCombo;
    std::unique_ptr<ComboAttachment> styleAttachment, characterAttachment, detectorAttachment, sidechainSourceAttachment;

    juce::ToggleButton autoThresholdButton { "AUTO THRESHOLD" };
    juce::ToggleButton autoReleaseButton { "AUTO RELEASE" };
    juce::ToggleButton autoGainButton { "AUTO GAIN" };
    juce::ToggleButton sidechainEnabledButton { "SIDECHAIN EQ" };
    juce::ToggleButton sidechainListenButton { "LISTEN" };
    juce::ToggleButton characterPreButton { "PRE" };
    std::unique_ptr<ButtonAttachment> autoThresholdAttachment, autoReleaseAttachment,
        autoGainAttachment, sidechainEnabledAttachment, sidechainListenAttachment, characterPreAttachment;

    juce::TextButton saveAButton { "Save A" }, saveBButton { "Save B" }, compareButton { "A/B" }, resetButton { "Reset" }, helpButton { "?" };
    juce::ValueTree snapshotA, snapshotB;
    bool showingA = false;

    void addKnob (const juce::String& id, const juce::String& label);
    void configureCombo (juce::ComboBox&, const juce::StringArray& items);
    void paintKnobPanel (juce::Graphics&, juce::Rectangle<int>, const juce::String& title);
    void paintCompressorDisplay (juce::Graphics&, juce::Rectangle<int>);
    void paintSidechainEQ (juce::Graphics&, juce::Rectangle<int>);
    void applyPreset (int index);
    void resetParameters();
    void timerCallback() override;
    juce::Rectangle<int> getSidechainGraphBounds() const;
    void setParameterFromUI (const juce::String& id, float actualValue);
    int selectedEQBand = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RajvikDynamicsProAudioProcessorEditor)
};
