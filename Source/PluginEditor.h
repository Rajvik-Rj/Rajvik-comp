#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class RajvikDynamicsProAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit RajvikDynamicsProAudioProcessorEditor (RajvikDynamicsProAudioProcessor&);
    ~RajvikDynamicsProAudioProcessorEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    RajvikDynamicsProAudioProcessor& audioProcessor;

    juce::Label titleLabel, subtitleLabel;
    juce::Slider threshold, ratio, attack, release, mix, output;
    juce::Label thresholdLabel, ratioLabel, attackLabel, releaseLabel, mixLabel, outputLabel;
    std::unique_ptr<SliderAttachment> thresholdAttachment, ratioAttachment,
        attackAttachment, releaseAttachment, mixAttachment, outputAttachment;

    void configureSlider (juce::Slider&, juce::Label&, const juce::String&);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RajvikDynamicsProAudioProcessorEditor)
};
