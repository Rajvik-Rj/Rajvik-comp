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
    void showSettings(); void showGuiMenu(); void closeOverlays(); static juce::File sizeFile(); static int loadDefaultW();
    void showPresets(); void promptSave(); void stepPreset(int dir); void setTrigger(int t); int triggerNow() const; void toggleEnlarge();
    void registerHints(); void setScalePercent(int pct);
    float scH() const { return scOpen ? 210.f : 28.f; }
    float canvasH() const { return 674.f + scH(); }
    float k() const { return (float) getWidth() / 1160.f; }
    juce::Rectangle<int> Ri(float x, float y, float w, float h) const { return juce::Rectangle<float>(x * k(), y * k(), w * k(), h * k()).toNearestInt(); }

    RajvikProcessor& proc; RajvikLnF lnf;
    int selBand = 0, eqSel = 1, presetIdx = -1; bool mbShown = false, scOpen = false, helpOn = false, enlarged = false, learnOn = false; int normalW = 1000;
    juce::String presetName{ "Default Preset" }, hint;

    // header
    pro::BoxButton undoBtn{ "Undo" }, redoBtn{ "Redo" }, aBtn{ "A" }, bBtn{ "B" }, copyBtn{ "Copy" }, detBtn{ "Peak" }, prevBtn{ "<" }, nextBtn{ ">" }, presetBtn{ "Default Preset" }, saveBtn{ "Save" }, toolsBtn{ "Tools" };
    ui::IconButton guiBtn{ "gear" };
    // main view
    pro::ProDisplay display; MultibandDisplay mbDisplay; pro::ProMeter meter;
    pro::BoxButton extBtn{ "Ext SC" }; std::unique_ptr<SA::ButtonAttachment> extA;
    juce::TextButton bandBtn[4]; juce::TextButton soloBtn{ "S" }, muteBtn{ "M" }, bypBtn{ "Byp" }; juce::ComboBox bandsBox; juce::Slider bandGain{ juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight };
    // control cards
    pro::BoxButton stylePill{ "Clean" }, autoThrBtn{ "AUTO THR" }, autoRelBtn{ "AUTO REL" }, autoGainBtn{ "AUTO GAIN" }, charBtn{ "Tube" };
    pro::ProKnob thr, ratio, atk, rel, gain, mixK; pro::CurveSlider knee, range, look, hold, dryG, wetMS, dryMS;
    pro::CharacterPanel charPanel;
    // side chain (top)
    std::unique_ptr<juce::Button> scToggle;
    pro::BoxButton trig[4] = { pro::BoxButton("Internal"), pro::BoxButton("External"), pro::BoxButton("MIDI"), pro::BoxButton("Tempo") };
    pro::BoxButton eqOnBtn{ "EQ" }, listenBtn{ "Listen" }, scOnBtn{ "SC On" }, invertBtn{ "Invert" }, midBtn{ "Mid" }, sideBtn{ "Side" };
    pro::CurveSlider scLevelSl, linkSl; juce::ComboBox routeBox, eqBandsBox; EqGraph eqGraph;
    juce::ComboBox bTypeBox, bSlopeBox, bChBox; pro::DragValue bFreq, bGain, bQ; pro::BoxButton bOn{ "ON" };
    // status bar
    pro::BoxButton osBtn{ "Oversampling  Off" }, midiBtn{ "MIDI Learn" }, modeBtn{ "Mode  L / R" }, mbBtn{ "Multiband" }; pro::DragValue inV; juce::String latencyText;

    std::unique_ptr<SA::ComboBoxAttachment> bandsA, routeA, eqBandsA, bTypeA, bSlopeA, bChA;
    std::unique_ptr<SA::ButtonAttachment> listenA, invertA, midA, sideA, soloA, muteA, bypA, scOnA, bOnA, autoThrA, autoRelA, autoGainA;
    std::unique_ptr<SA::SliderAttachment> bandGainA;
    std::vector<std::pair<juce::Component*, juce::String>> hints;
    float scRing = -100.f, extSm = 0.f; juce::Component::SafePointer<juce::Component> openBox, openAlert;
};
