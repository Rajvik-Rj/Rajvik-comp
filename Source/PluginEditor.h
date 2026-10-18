#pragma once
#include "ProUI.h"

class RajvikEditor : public juce::AudioProcessorEditor, private juce::Timer {
public:
    explicit RajvikEditor(RajvikProcessor&);
    ~RajvikEditor() override;
    void paint(juce::Graphics&) override;
    void paintOverChildren(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseMove(const juce::MouseEvent&) override;
    void setSidechainOpen(bool open);      // also used by the screenshot tool
    void setHelpOn(bool on) { helpOn = on; repaint(); }
    void showCharacterPanel(bool show);

private:
    using SA = juce::AudioProcessorValueTreeState;
    void timerCallback() override;
    void bindMainKnobs(); void bindBandControls(); void bindEqBand(); void onBandSelected(); void onEqSelected(); void applyModeLayout(); void applyLayoutVisibility();
    void showSettings(); void closeOverlays(); void showGuiMenu(); static juce::File sizeFile(); static int loadDefaultW(); void showPresets(); void promptSave(); void stepPreset(int dir); void setTrigger(int t); int triggerNow() const;
    void registerHints(); void setScalePercent(int pct); float canvasH() const { return scOpen ? 920.f : 660.f; }
    float k() const { return (float) getWidth() / 1100.f; }
    juce::Rectangle<int> Ri(float x, float y, float w, float h) const { return juce::Rectangle<float>(x * k(), y * k(), w * k(), h * k()).toNearestInt(); }

    RajvikProcessor& proc; RajvikLnF lnf;
    int selBand = 0, eqSel = 1, presetIdx = -1; bool mbShown = false, scOpen = false, helpOn = false, enlarged = false, learnOn = false; int normalW = 1000;
    juce::String presetName{ "Default Preset" }, hint;

    // header
    ui::IconButton guiBtn{ "gear" }, undoBtn{ "undo" }, redoBtn{ "redo" }, prevBtn{ "prev" }, nextBtn{ "next" }, fullBtn{ "fullscreen" };
    ui::TextLinkButton abBtn{ "A / B" }, copyBtn{ "Copy" }, helpBtn{ "Help" }, presetBtn{ "Default Preset" };
    // display
    pro::ProDisplay display; pro::KneePanel kneePanel; MultibandDisplay mbDisplay; pro::ProMeter meter;
    juce::TextButton bandBtn[4]; juce::TextButton soloBtn{ "S" }, muteBtn{ "M" }, bypBtn{ "Byp" }; juce::ComboBox bandsBox; juce::Slider bandGain{ juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight };
    // knobs
    pro::ProKnob thr, ratio, atk, rel, gain, dry; pro::CurveSlider knee, range, look, hold;
    pro::PillButton stylePill; std::unique_ptr<juce::Button> charIcon, scToggle; pro::CharacterPanel charPanel;
    // status bar
    std::unique_ptr<juce::Button> midiItem, osItem, lookItem, detItem; pro::ProKnob mbMix; juce::TextButton mbBtn{ "Multiband" }; pro::DragValue mixV, inV, outV; ui::IconButton gearBtn{ "gear" };
    // side chain panel
    ui::IconButton listenBtn{ "phones" }; pro::PillButton trig[4] = { pro::PillButton("INTERNAL"), pro::PillButton("EXTERNAL"), pro::PillButton("MIDI"), pro::PillButton("TEMPO") };
    pro::DragValue scLevelV; pro::ProKnob wetPan, dryPan; pro::Segmented stereoSeg; double lastWet = 0; pro::VSlider scLevelSl; pro::ProKnob linkKnob;
    pro::PillButton stereoPill, midBtn{ "M" }, sideBtn{ "S" }, invertBtn{ "INVERT" }, peakBtn{ "PEAK" }, rmsBtn{ "RMS" }, scOnBtn{ "SC ON" }, eqOnBtn{ "EQ" };
    juce::ComboBox routeBox, eqBandsBox;
    EqGraph eqGraph;
    juce::ComboBox bTypeBox, bSlopeBox, bChBox; pro::DragValue bFreq, bGain, bQ; pro::PillButton bOn;

    std::unique_ptr<SA::ComboBoxAttachment> bandsA, routeA, eqBandsA, bTypeA, bSlopeA, bChA;
    std::unique_ptr<SA::ButtonAttachment> listenA, invertA, midA, sideA, soloA, muteA, bypA, scOnA, eqOnA, bOnA;
    std::unique_ptr<SA::SliderAttachment> bandGainA, scLevelA;
    std::vector<std::pair<juce::Component*, juce::String>> hints;
    float scRing = -100.f, extSm = 0.f; juce::Component::SafePointer<juce::Component> openBox, openAlert;
};
