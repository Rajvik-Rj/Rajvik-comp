#pragma once
#include "EditorComponents.h"

class RajvikEditor : public juce::AudioProcessorEditor, private juce::Timer {
public:
    explicit RajvikEditor(RajvikProcessor&);
    ~RajvikEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    using SA = juce::AudioProcessorValueTreeState;
    void timerCallback() override;
    void rebuildPresetMenu(); void promptSave(); void stepPreset(int dir);
    void applyModeLayout(); void bindMainKnobs(); void bindBandControls(); void onBandSelected(); void onEqSelected();
    void setEqType(int kind); void showSettings(); void showHelp(); void setScalePercent(int pct);
    float k() const { return (float) getWidth() / 1536.f; }
    juce::Rectangle<int> Ri(float x, float y, float w, float h) const { return juce::Rectangle<float>(x * k(), y * k(), w * k(), h * k()).toNearestInt(); }
    void place(KnobBox& kb, juce::Rectangle<float> cell, const KnobGeom& g);

    RajvikProcessor& proc; RajvikLnF lnf;
    int selBand = 0, eqSel = 1; bool mbShown = false, scOpen = true, enlarged = false; int normalW = 1152; bool learnOn = false;

    // header
    ui::IconButton undoBtn{ "undo" }, redoBtn{ "redo" }, prevBtn{ "prev" }, nextBtn{ "next" }, fullBtn{ "fullscreen" };
    ui::TextLinkButton abBtn{ "A / B" }, copyBtn{ "Copy" }, helpBtn{ "Help" };
    juce::ComboBox presetBox;
    // display panel
    juce::ComboBox modeBox; juce::Label modeDesc; juce::TextButton singleBtn{ "Single" }, multiBtn{ "Multiband" };
    DisplayComponent display; MultibandDisplay mbDisplay; MeterStrip meter;
    juce::TextButton bandBtn[4]; juce::TextButton soloBtn{ "S" }, muteBtn{ "M" }, bypBtn{ "Byp" };
    juce::ComboBox bandsBox; juce::Slider bandGain{ juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight };
    // knobs
    KnobBox inG, thr, ratio, atk, rel, mix, outG, knee, range, hold, look, scLevel;
    juce::ToggleButton autoGainBtn{ "Auto Gain" }, autoRelBtn{ "Auto Release" };
    juce::ComboBox stereoBox; juce::Slider linkSl{ juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight };
    juce::TextButton peakBtn{ "Peak" }, rmsBtn{ "RMS" };
    // sidechain & EQ
    juce::ToggleButton scOnBtn{ "Sidechain & EQ" }; ui::IconButton chevBtn{ "chevUp" }, eqPowerBtn{ "power" };
    juce::ComboBox eqBandsBox, srcBox, routeBox, eqModeBox, osBox;
    ui::IconButton toolBell{ "tBell" }, toolShelf{ "tShelf" }, toolCut{ "tCut" };
    juce::TextButton listenBtn{ "Listen" }, invertBtn{ "Invert" }, midBtn{ "M" }, sideBtn{ "S" };
    EqGraph eqGraph; EqTable eqTable;
    // status bar
    juce::ComboBox midiBox; StatusButton osItem{ "upload", "" }, lookItem{ "clock", "" }, mixItem{ "link", "" }, outItem{ "speaker", "" }, scaleItem{ "eye", "" };
    ui::IconButton gearBtn{ "gear" };

    std::unique_ptr<SA::ComboBoxAttachment> modeA, stereoA, bandsA, eqBandsA, srcA, routeA, eqModeA, osA;
    std::unique_ptr<SA::ButtonAttachment> agA, arA, scOnA, listenA, invertA, midA, sideA, soloA, muteA, bypA;
    std::unique_ptr<SA::SliderAttachment> linkA, bandGainA;
    juce::String lastDesc;
};
