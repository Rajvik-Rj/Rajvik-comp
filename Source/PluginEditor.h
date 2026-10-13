#pragma once
#include "EditorComponents.h"

class RajvikEditor : public juce::AudioProcessorEditor, private juce::Timer {
public:
    explicit RajvikEditor(RajvikProcessor&);
    ~RajvikEditor() override { proc.metersWanted = false; proc.cancelLearn(); setLookAndFeel(nullptr); }
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    void timerCallback() override;
    void rebuildPresetMenu();
    void promptSave();
    using SA = juce::AudioProcessorValueTreeState;

    RajvikProcessor& proc;
    RajvikLnF lnf;
    DisplayComponent display; MeterStrip meter; EqGraph eqGraph;
    juce::TextButton undoBtn{ "Undo" }, redoBtn{ "Redo" }, abBtn{ "A" }, copyBtn{ "Copy" }, pasteBtn{ "Paste" }, saveBtn{ "Save" },
                     scBtn{ "Sidechain" }, clearMidiBtn{ "Clear MIDI" }, defaultsBtn{ "Mode defaults" }, resetBtn{ "Reset" };
    juce::ToggleButton learnBtn{ "MIDI Learn" }, syncBtn{ "Tempo Sync" };
    juce::ComboBox stereoBox, rateBox, presetBox, modeBox, charBox, detBox, osBox, slopeBox;
    juce::ToggleButton autoGainBtn{ "Auto Gain" }, autoRelBtn{ "Auto Release" }, extScBtn{ "External SC" }, listenBtn{ "SC Listen" };
    Knob thr, ratio, atk, rel, mix, knee, range, hold, look, inG, outG, drive, link;
    std::unique_ptr<SA::ComboBoxAttachment> modeA, charA, detA, osA, slopeA;
    std::unique_ptr<SA::ButtonAttachment> agA, arA, exA, liA;
    juce::Slider offsetSl{ juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight }, pulseSl{ juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight };
    juce::Label offsetLbl{ {}, "Offset" }, pulseLbl{ {}, "Pulse" };
    std::unique_ptr<SA::ComboBoxAttachment> stereoA, rateA; std::unique_ptr<SA::ButtonAttachment> syncA;
    std::unique_ptr<SA::SliderAttachment> offA, pulA;
    juce::Label modeDesc; bool scOpen = false;
};
