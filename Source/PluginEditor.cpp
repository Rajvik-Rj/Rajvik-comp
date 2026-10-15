#include "PluginEditor.h"
using namespace pro;

namespace {
// "SIDE CHAIN" label with a chevron (opens / closes the side chain panel)
class LabelButton : public juce::Button {
public:
    LabelButton(const juce::String& t) : juce::Button(t), text(t) {}
    void paintButton(juce::Graphics& g, bool hl, bool) override {
        const float k = K(*this); auto r = getLocalBounds().toFloat(); const bool on = getToggleState();
        const auto f = F(10.5f * k, true, 0.1f); const float tw = textW(text, f), iw = 12.f * k, x0 = (r.getWidth() - (tw + iw + 4.f * k)) * 0.5f;
        const auto col = on ? cYellow : (hl ? juce::Colours::white : cDim);
        ui::drawIcon(g, on ? "chev" : "chevUp", juce::Rectangle<float>(x0, r.getY(), iw, r.getHeight()), col, 1.5f * k);
        g.setFont(f); g.setColour(col); g.drawText(text, juce::Rectangle<float>(x0 + iw + 4.f * k, r.getY(), tw + 4.f, r.getHeight()), juce::Justification::centredLeft);
    }
    juce::String text;
};
// small red "character" icon under the style pill
class CharIcon : public juce::Button {
public:
    CharIcon() : juce::Button("character") {}
    bool lit = false;
    void paintButton(juce::Graphics& g, bool hl, bool) override {
        const float k = K(*this); auto r = getLocalBounds().toFloat().withSizeKeepingCentre(20.f * k, 12.f * k); const auto c = juce::Colour(0xffe2452e).withAlpha(lit || hl ? 1.f : 0.75f);
        if (lit) { g.setColour(c.withAlpha(0.25f)); g.fillRoundedRectangle(r.expanded(3.f * k), 5.f * k); }
        g.setColour(c); g.drawRoundedRectangle(r, 3.5f * k, 1.4f * k);
        g.fillEllipse(r.getX() + r.getWidth() * 0.26f - 2.2f * k, r.getCentreY() - 2.2f * k, 4.4f * k, 4.4f * k); g.fillEllipse(r.getRight() - r.getWidth() * 0.26f - 2.2f * k, r.getCentreY() - 2.2f * k, 4.4f * k, 4.4f * k);
    }
};
// status bar item:  Prefix: value
class StatusText : public juce::Button {
public:
    StatusText(const juce::String& p) : juce::Button(p), prefix(p) {}
    void setValue(const juce::String& v) { if (v != value) { value = v; repaint(); } }
    void paintButton(juce::Graphics& g, bool hl, bool) override {
        const float k = K(*this); auto r = getLocalBounds().toFloat(); g.setFont(F(11.f * k));
        const float pw = textW(prefix, F(11.f * k)) + 5.f * k; g.setColour(cFaint.brighter(0.25f)); g.drawText(prefix, r.withWidth(pw), juce::Justification::centredLeft);
        g.setColour(hl ? cYellow : cText.withAlpha(0.88f)); g.drawText(value, r.withTrimmedLeft(pw), juce::Justification::centredLeft);
    }
    juce::String prefix, value;
};

// popup with tempo-sync settings and preset / state actions
class SettingsPanel : public juce::Component {
public:
    using SA = juce::AudioProcessorValueTreeState;
    SettingsPanel(SA& a, std::function<void(int)> act) : action(std::move(act)) {
        setLookAndFeel(&lnf);
        ui::fillCombo(rateBox, a, "syncRate");
        rateA = std::make_unique<SA::ComboBoxAttachment>(a, "syncRate", rateBox); syncA = std::make_unique<SA::ButtonAttachment>(a, "syncOn", syncBtn);
        offA = std::make_unique<SA::SliderAttachment>(a, "syncOffset", offset); pulA = std::make_unique<SA::SliderAttachment>(a, "syncPulse", pulse);
        lookA = std::make_unique<SA::ButtonAttachment>(a, "lookOn", lookBtn);
        for (auto* s : { &offset, &pulse }) s->setTextBoxStyle(juce::Slider::TextBoxRight, false, 64, 20);
        const char* names[] = { "Host tempo trigger", "Rate", "Offset", "Pulse width", "Lookahead" };
        for (int i = 0; i < 5; ++i) { lbl[i].setText(names[i], juce::dontSendNotification); lbl[i].setColour(juce::Label::textColourId, ui::dim); addAndMakeVisible(lbl[i]); }
        const char* acts[] = { "Style defaults", "Save preset...", "Copy settings", "Paste settings", "Clear MIDI map", "Reset all" };
        for (int i = 0; i < 6; ++i) { btn[i].setButtonText(acts[i]); btn[i].onClick = [this, i] { action(i); }; addAndMakeVisible(btn[i]); }
        for (auto* c : std::initializer_list<juce::Component*>{ &syncBtn, &rateBox, &offset, &pulse, &lookBtn }) addAndMakeVisible(c);
        setSize(340, 5 * 34 + 3 * 34 + 24);
    }
    ~SettingsPanel() override { setLookAndFeel(nullptr); }
    void paint(juce::Graphics& g) override { g.fillAll(juce::Colour(0xff1e1e22)); }
    void resized() override {
        auto r = getLocalBounds().reduced(12); juce::Component* rows[] = { &syncBtn, &rateBox, &offset, &pulse, &lookBtn };
        for (int i = 0; i < 5; ++i) { auto row = r.removeFromTop(34); lbl[i].setBounds(row.removeFromLeft(130)); rows[i]->setBounds(row.reduced(0, 3)); }
        r.removeFromTop(6);
        for (int i = 0; i < 6; i += 2) { auto row = r.removeFromTop(34); btn[i].setBounds(row.removeFromLeft(row.getWidth() / 2).reduced(2)); btn[i + 1].setBounds(row.reduced(2)); }
    }
private:
    RajvikLnF lnf; std::function<void(int)> action; juce::ComboBox rateBox; juce::ToggleButton syncBtn, lookBtn; juce::Label lbl[5]; juce::TextButton btn[6];
    juce::Slider offset{ juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight }, pulse{ juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight };
    std::unique_ptr<SA::ComboBoxAttachment> rateA; std::unique_ptr<SA::ButtonAttachment> syncA, lookA; std::unique_ptr<SA::SliderAttachment> offA, pulA;
};
} // namespace

RajvikEditor::RajvikEditor(RajvikProcessor& p)
    : AudioProcessorEditor(&p), proc(p), display(p), mbDisplay(p, selBand, [this] { onBandSelected(); }), charPanel(p.apvts), eqGraph(p, eqSel, [this] { onEqSelected(); })
{
    setLookAndFeel(&lnf);
    auto& a = proc.apvts;
    charIcon = std::make_unique<CharIcon>(); scToggle = std::make_unique<LabelButton>("SIDE CHAIN");
    midiItem = std::make_unique<StatusText>("MIDI Learn"); osItem = std::make_unique<StatusText>("Oversampling:"); lookItem = std::make_unique<StatusText>("Lookahead:");
    for (auto* c : std::initializer_list<juce::Component*>{ &display, &mbDisplay, &meter, &guiBtn, &undoBtn, &redoBtn, &prevBtn, &nextBtn, &fullBtn, &abBtn, &copyBtn, &helpBtn, &presetBtn,
         &soloBtn, &muteBtn, &bypBtn, &bandsBox, &bandGain, &thr, &ratio, &atk, &rel, &gain, &dry, &knee, &range, &look, &hold, &stylePill, charIcon.get(), scToggle.get(),
         midiItem.get(), osItem.get(), lookItem.get(), &mbBtn, &mixV, &inV, &outV, &gearBtn,
         &listenBtn, &trig[0], &trig[1], &trig[2], &trig[3], &scLevelSl, &scLevelV, &wetPanV, &dryPanV, &linkKnob, &stereoPill, &midBtn, &sideBtn, &invertBtn, &peakBtn, &rmsBtn, &scOnBtn, &eqOnBtn, &routeBox, &eqBandsBox,
         &eqGraph, &bTypeBox, &bSlopeBox, &bChBox, &bFreq, &bGain, &bQ, &bOn })
        addAndMakeVisible(c);
    addChildComponent(charPanel);
    for (int i = 0; i < 4; ++i) { bandBtn[i].setButtonText("Band " + juce::String(i + 1)); bandBtn[i].getProperties().set("tc", (int) ui::bandCol[i].getARGB()); bandBtn[i].onClick = [this, i] { selBand = i; onBandSelected(); }; addAndMakeVisible(bandBtn[i]); }

    // ---- knobs ----
    thr.caption = "THRESHOLD"; thr.autoWord = "AUTO"; thr.big = true; thr.bindAuto(a, "autoThr");
    ratio.caption = "RATIO"; atk.caption = "ATTACK"; rel.caption = "RELEASE"; rel.autoWord = "AUTO"; rel.bindAuto(a, "autoRelease");
    gain.caption = "WET GAIN"; gain.autoWord = "AUTO"; gain.bipolar = true; gain.bind(a, "outGain"); gain.bindAuto(a, "autoGain");
    dry.caption = "DRY"; dry.bind(a, "dry");
    knee.caption = "KNEE"; knee.bind(a, "knee"); range.caption = "RANGE"; range.bind(a, "range"); look.caption = "LOOKAHEAD"; look.bind(a, "lookahead"); hold.caption = "HOLD"; hold.bind(a, "hold");
    bindMainKnobs();
    // MIDI learn: enable it in the status bar, touch a control, then move a hardware CC
    for (auto* kb : { &thr, &ratio, &atk, &rel, &gain, &dry, &linkKnob }) kb->onTouch = [this, kb] { if (learnOn) proc.beginLearn(kb->paramId); };
    knee.onTouch = [this] { if (learnOn) proc.beginLearn("knee"); }; range.onTouch = [this] { if (learnOn) proc.beginLearn("range"); };
    look.onTouch = [this] { if (learnOn) proc.beginLearn("lookahead"); }; hold.onTouch = [this] { if (learnOn) proc.beginLearn("hold"); };

    // ---- header ----
    guiBtn.onClick = [this] { showGuiMenu(); };
    undoBtn.onClick = [this] { proc.undo.undo(); }; redoBtn.onClick = [this] { proc.undo.redo(); };
    abBtn.onClick = [this] { proc.toggleAB(); abBtn.underline = proc.currentAB(); abBtn.repaint(); }; abBtn.underline = proc.currentAB();
    copyBtn.onClick = [this] { proc.copyToOtherAB(); };
    helpBtn.onClick = [this] { helpOn = !helpOn; hint = {}; repaint(); };
    fullBtn.onClick = [this] { if (!enlarged) { normalW = getWidth(); setSize(juce::roundToInt(normalW * 1.5f), juce::roundToInt(normalW * 1.5f * canvasH() / 1100.f)); } else setSize(normalW, juce::roundToInt(normalW * canvasH() / 1100.f)); enlarged = !enlarged; };
    prevBtn.onClick = [this] { stepPreset(-1); }; nextBtn.onClick = [this] { stepPreset(1); }; presetBtn.onClick = [this] { showPresets(); };

    // ---- style pill + character ----
    stylePill.onClick = [this] {
        juce::PopupMenu m; const int cur = (int) ui::getP(proc.apvts, "mode");
        for (int i = 0; i < rdp::kNumModes; ++i) m.addItem(i + 1, juce::String(rdp::kModes[i].name) + "   -   " + rdp::kModes[i].description, true, i == cur);
        m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&stylePill).withMinimumWidth(360), [this](int r) { if (r > 0) { ui::gesture(proc.apvts, "mode", true); ui::setP(proc.apvts, "mode", (float) (r - 1)); ui::gesture(proc.apvts, "mode", false); } });
    };
    charIcon->onClick = [this] { showCharacterPanel(!charPanel.isVisible()); };
    scToggle->onClick = [this] { setSidechainOpen(!scOpen); };

    // ---- status bar ----
    midiItem->onClick = [this] {
        juce::PopupMenu m; m.addItem(1, "MIDI Learn", true, learnOn); m.addItem(2, "Clear MIDI mappings");
        m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(midiItem.get()), [this](int r) {
            if (r == 1) { learnOn = !learnOn; if (!learnOn) proc.cancelLearn(); } else if (r == 2) proc.clearMidiMap(); });
    };
    osItem->onClick = [this] {
        juce::PopupMenu m; const int cur = (int) ui::getP(proc.apvts, "oversampling"); const char* n[] = { "Off", "2x", "4x", "8x", "16x", "32x" };
        for (int i = 0; i < 6; ++i) m.addItem(i + 1, n[i], true, i == cur);
        m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(osItem.get()), [this](int r) { if (r > 0) { ui::gesture(proc.apvts, "oversampling", true); ui::setP(proc.apvts, "oversampling", (float) (r - 1)); ui::gesture(proc.apvts, "oversampling", false); } });
    };
    lookItem->onClick = [this] {
        const bool on = ui::getP(proc.apvts, "lookOn") > 0.5f; ui::gesture(proc.apvts, "lookOn", true); ui::setP(proc.apvts, "lookOn", on ? 0.f : 1.f); ui::gesture(proc.apvts, "lookOn", false);
    };
    mbBtn.setClickingTogglesState(false);
    mbBtn.onClick = [this] { const bool on = ui::getP(proc.apvts, "mbOn") > 0.5f; ui::gesture(proc.apvts, "mbOn", true); ui::setP(proc.apvts, "mbOn", on ? 0.f : 1.f); ui::gesture(proc.apvts, "mbOn", false); };
    mixV.bind(a, "mix"); inV.bind(a, "inGain"); outV.bind(a, "outGain"); mixV.prefix = ""; inV.prefix = ""; outV.prefix = "";
    gearBtn.onClick = [this] { showSettings(); };
    meter.onReset = [this] { proc.meterReset = true; }; proc.metersWanted = true;

    // ---- band strip (multiband) ----
    ui::fillCombo(bandsBox, a, "mbBands"); bandsA = std::make_unique<SA::ComboBoxAttachment>(a, "mbBands", bandsBox);
    bandGain.setTextBoxStyle(juce::Slider::TextBoxRight, false, 70, 22);
    for (auto* b : { &soloBtn, &muteBtn, &bypBtn }) b->setClickingTogglesState(true);
    soloBtn.getProperties().set("tc", (int) ui::accent.getARGB()); muteBtn.getProperties().set("tc", (int) ui::red.getARGB()); bypBtn.getProperties().set("tc", (int) ui::dim.brighter(0.4f).getARGB());
    bindBandControls();

    // ---- side chain panel ----
    listenBtn.setClickingTogglesState(true); listenA = std::make_unique<SA::ButtonAttachment>(a, "scListen", listenBtn);
    for (int i = 0; i < 4; ++i) { trig[i].setClickingTogglesState(false); trig[i].onClick = [this, i] { if (i == 3 && triggerNow() == 3) showSettings(); else setTrigger(i); }; }
    scLevelA = std::make_unique<SA::SliderAttachment>(a, "scLevel", scLevelSl); 
    scLevelV.bind(a, "scLevel"); wetPanV.bind(a, "wetPan"); dryPanV.bind(a, "dryPan"); wetPanV.prefix = "WET M/S"; dryPanV.prefix = "DRY M/S";
    lastWet = gain.getValue();   // Alt + drag Wet gain = reverse-linked Dry gain
    gain.onValueChange = [this] { const double d = gain.getValue() - lastWet; lastWet = gain.getValue();
        if (d != 0.0 && gain.isMouseButtonDown() && juce::ModifierKeys::getCurrentModifiers().isAltDown()) dry.setValue(dry.getValue() - d, juce::sendNotificationSync); };
    linkKnob.caption = "STEREO LINK"; linkKnob.bind(a, "link");
    stereoPill.onClick = [this] { const bool ms = ui::getP(proc.apvts, "stereoMode") > 0.5f; ui::gesture(proc.apvts, "stereoMode", true); ui::setP(proc.apvts, "stereoMode", ms ? 0.f : 1.f); ui::gesture(proc.apvts, "stereoMode", false); };
    for (auto* b : { &midBtn, &sideBtn, &invertBtn, &scOnBtn, &eqOnBtn }) b->setClickingTogglesState(true);
    midA = std::make_unique<SA::ButtonAttachment>(a, "procMid", midBtn); sideA = std::make_unique<SA::ButtonAttachment>(a, "procSide", sideBtn);
    invertA = std::make_unique<SA::ButtonAttachment>(a, "scInvert", invertBtn); scOnA = std::make_unique<SA::ButtonAttachment>(a, "scOn", scOnBtn);
    peakBtn.onClick = [this] { const bool on = ui::getP(proc.apvts, "detector") == 1.f; ui::gesture(proc.apvts, "detector", true); ui::setP(proc.apvts, "detector", on ? 0.f : 1.f); ui::gesture(proc.apvts, "detector", false); };
    rmsBtn.onClick = [this] { const bool on = ui::getP(proc.apvts, "detector") == 2.f; ui::gesture(proc.apvts, "detector", true); ui::setP(proc.apvts, "detector", on ? 0.f : 2.f); ui::gesture(proc.apvts, "detector", false); };
    eqOnBtn.setClickingTogglesState(false);
    eqOnBtn.onClick = [this] { const float on = ui::getP(proc.apvts, "scEqOn") > 0.5f ? 0.f : 1.f; ui::gesture(proc.apvts, "scEqOn", true); ui::setP(proc.apvts, "scEqOn", on); ui::gesture(proc.apvts, "scEqOn", false); };
    ui::fillCombo(routeBox, a, "scRoute"); routeA = std::make_unique<SA::ComboBoxAttachment>(a, "scRoute", routeBox);
    ui::fillCombo(eqBandsBox, a, "eqBands"); eqBandsA = std::make_unique<SA::ComboBoxAttachment>(a, "eqBands", eqBandsBox);
    ui::fillCombo(bTypeBox, a, "eqType1"); ui::fillCombo(bSlopeBox, a, "eqSlope1"); ui::fillCombo(bChBox, a, "eqMs1");
    bFreq.prefix = "FREQ"; bGain.prefix = "GAIN"; bQ.prefix = "Q"; bOn.text = "ON"; bOn.setClickingTogglesState(true);
    bindEqBand();

    setResizable(true, true); setResizeLimits(220, 132, 4400, 2640); getConstrainer()->setFixedAspectRatio(1100.0 / canvasH());
    { const int dw = loadDefaultW(); setSize(dw, juce::roundToInt((float) dw * canvasH() / 1100.f)); }
    mbShown = ui::getP(a, "mbOn") > 0.5f; applyModeLayout(); onEqSelected(); registerHints();
    addMouseListener(this, true);
    startTimerHz(30);
}

RajvikEditor::~RajvikEditor() { stopTimer(); proc.metersWanted = false; proc.cancelLearn(); setLookAndFeel(nullptr); }

void RajvikEditor::bindMainKnobs()
{
    auto& a = proc.apvts; const bool mb = ui::getP(a, "mbOn") > 0.5f; const auto n = juce::String(selBand + 1);
    thr.bind(a, mb ? "mbThr" + n : juce::String("threshold")); ratio.bind(a, mb ? "mbRatio" + n : juce::String("ratio"));
    atk.bind(a, mb ? "mbAtk" + n : juce::String("attack")); rel.bind(a, mb ? "mbRel" + n : juce::String("release"));
}
void RajvikEditor::bindBandControls()
{
    auto& a = proc.apvts; const auto n = juce::String(selBand + 1);
    soloA.reset(); muteA.reset(); bypA.reset(); bandGainA.reset();
    soloA = std::make_unique<SA::ButtonAttachment>(a, "mbSolo" + n, soloBtn); muteA = std::make_unique<SA::ButtonAttachment>(a, "mbMute" + n, muteBtn);
    bypA = std::make_unique<SA::ButtonAttachment>(a, "mbByp" + n, bypBtn); bandGainA = std::make_unique<SA::SliderAttachment>(a, "mbGain" + n, bandGain);
    for (int i = 0; i < 4; ++i) bandBtn[i].setToggleState(i == selBand, juce::dontSendNotification);
}
void RajvikEditor::onBandSelected() { selBand = std::clamp(selBand, 0, 3); bindBandControls(); bindMainKnobs(); mbDisplay.repaint(); }

void RajvikEditor::bindEqBand()
{
    auto& a = proc.apvts; const auto n = juce::String(eqSel + 1);
    bTypeA.reset(); bSlopeA.reset(); bChA.reset(); bOnA.reset(); bFreq.unbind(); bGain.unbind(); bQ.unbind();
    bTypeA = std::make_unique<SA::ComboBoxAttachment>(a, "eqType" + n, bTypeBox); bSlopeA = std::make_unique<SA::ComboBoxAttachment>(a, "eqSlope" + n, bSlopeBox);
    bChA = std::make_unique<SA::ComboBoxAttachment>(a, "eqMs" + n, bChBox); bOnA = std::make_unique<SA::ButtonAttachment>(a, "eqOn" + n, bOn);
    bFreq.bind(a, "eqFreq" + n); bGain.bind(a, "eqGain" + n); bQ.bind(a, "eqQ" + n);
}
void RajvikEditor::onEqSelected() { eqSel = std::clamp(eqSel, 0, rdp::kEqBands - 1); bindEqBand(); applyLayoutVisibility(); repaint(); }

int RajvikEditor::triggerNow() const
{
    if (ui::getP(proc.apvts, "syncOn") > 0.5f) return 3;
    return juce::jlimit(0, 2, (int) ui::getP(proc.apvts, "scExternal"));
}
void RajvikEditor::setTrigger(int t)
{
    auto& a = proc.apvts; ui::gesture(a, "syncOn", true); ui::gesture(a, "scExternal", true);
    ui::setP(a, "syncOn", t == 3 ? 1.f : 0.f); ui::setP(a, "scExternal", t == 3 ? 0.f : (float) t);
    ui::gesture(a, "syncOn", false); ui::gesture(a, "scExternal", false);
}

void RajvikEditor::applyModeLayout()
{
    mbShown = ui::getP(proc.apvts, "mbOn") > 0.5f;
    display.setVisible(!mbShown); mbDisplay.setVisible(mbShown);
    bindMainKnobs(); applyLayoutVisibility(); resized(); repaint();
}
void RajvikEditor::applyLayoutVisibility()
{
    const bool mb = mbShown, sc = scOpen;
    for (auto* c : std::initializer_list<juce::Component*>{ &soloBtn, &muteBtn, &bypBtn, &bandsBox, &bandGain, &bandBtn[0], &bandBtn[1], &bandBtn[2], &bandBtn[3] }) c->setVisible(mb);
    for (auto* c : std::initializer_list<juce::Component*>{ &listenBtn, &trig[0], &trig[1], &trig[2], &trig[3], &scLevelSl, &scLevelV, &linkKnob, &stereoPill, &invertBtn, &peakBtn, &rmsBtn, &scOnBtn, &eqOnBtn,
         &routeBox, &eqBandsBox, &eqGraph, &bTypeBox, &bOn, &bChBox }) c->setVisible(sc);
    const int t = (int) ui::getP(proc.apvts, "eqType" + juce::String(eqSel + 1));
    bFreq.setVisible(sc); bGain.setVisible(sc && rdp::eqHasGain(t)); bQ.setVisible(sc && !rdp::eqIsCut(t)); bSlopeBox.setVisible(sc && rdp::eqIsCut(t));
    for (auto* c : std::initializer_list<juce::Component*>{ &eqGraph, &bTypeBox, &bOn, &bChBox, &bFreq, &bGain, &bQ, &bSlopeBox, &eqOnBtn, &eqBandsBox }) { c->setEnabled(!mb); c->setAlpha(mb ? 0.35f : 1.f); }   // no sidechain EQ in multiband
    const bool ms = ui::getP(proc.apvts, "stereoMode") > 0.5f; midBtn.setVisible(sc && ms); sideBtn.setVisible(sc && ms); wetPanV.setVisible(sc && ms); dryPanV.setVisible(sc && ms);
}

void RajvikEditor::setSidechainOpen(bool open)
{
    scOpen = open; applyLayoutVisibility(); getConstrainer()->setFixedAspectRatio(1100.0 / canvasH());
    setSize(getWidth(), juce::roundToInt((float) getWidth() * canvasH() / 1100.f)); repaint();
}
void RajvikEditor::showCharacterPanel(bool show) { charPanel.setVisible(show); if (show) { charPanel.refresh(); charPanel.toFront(false); } }

void RajvikEditor::mouseDown(const juce::MouseEvent& e)
{
    if (charPanel.isVisible() && e.eventComponent != &charPanel && !charPanel.isParentOf(e.eventComponent) && e.eventComponent != charIcon.get()) showCharacterPanel(false);
}
void RajvikEditor::mouseMove(const juce::MouseEvent& e)
{
    if (!helpOn) return;
    juce::String h; for (auto* c = e.eventComponent; c != nullptr && c != this; c = c->getParentComponent()) {
        for (auto& [comp, txt] : hints) if (comp == c) { h = txt; break; } if (h.isNotEmpty()) break; }
    if (h != hint) { hint = h; repaint(); }
}
void RajvikEditor::registerHints()
{
    hints = {
        { &thr, "Threshold: the level above which the compressor starts working. The green ring shows the level arriving at the detector. AUTO makes the threshold follow the signal level." },
        { &ratio, "Ratio: how strongly the signal above the threshold is reduced. 1:1 does nothing, 100:1 is limiting." },
        { &atk, "Attack: how fast the compressor reacts to a level above the threshold. Program-dependent in most styles." },
        { &rel, "Release: how fast the gain recovers. AUTO adapts the release to the material." },
        { &gain, "Gain: output gain. AUTO matches the loudness of the compressed signal to the input." },
        { &dry, "Dry gain: level of the uncompressed signal mixed in (parallel compression). Alt + drag Wet gain moves Dry in the opposite direction." },
        { &knee, "Knee: width of the soft transition around the threshold." }, { &range, "Range: the maximum amount of gain change in dB." },
        { &look, "Lookahead: delays the audio so the compressor can react before a peak arrives. Reports latency to the host." },
        { &hold, "Hold: how long the gain reduction is held before the release starts." },
        { &stylePill, "Style: one of 14 program-dependent compression styles. Click to choose." },
        { charIcon.get(), "Character: analog saturation (Tube, Diode, Bright) with drive and Pre / Post compression routing." },
        { scToggle.get(), "Side chain: open the trigger section with the side chain EQ, trigger source and stereo link." },
        { &meter, "Meters: input and output peak levels, gain reduction on the left. Click the output read-out to cycle peak, true peak, RMS and LUFS." },
        { &guiBtn, "GUI settings: window size from tiny to huge, default size, custom size." }, { &presetBtn, "Preset browser: search by name or tag." }, { &mixV, "Mix: scales the gain change from 0 % to 200 %." },
        { &inV, "Input gain." }, { &outV, "Output gain." },
        { &eqGraph, "Side chain EQ: drag nodes to shape what the compressor hears. Right-click a node for the filter type, double-click to switch a band on / off, wheel changes Q." },
        { &linkKnob, "Stereo link: 0 % compresses each channel independently, 100 % uses a single common gain." },
        { &stereoPill, "Switch between Left/Right and Mid/Side processing." },
        { &listenBtn, "Side chain listen: hear the filtered trigger signal." },
        { &trig[0], "Trigger: internal signal." }, { &trig[1], "Trigger: external side chain input." }, { &trig[2], "Trigger: MIDI notes (compression is active while a note is held)." }, { &trig[3], "Trigger: host-tempo pulse. Click again for rate, offset and pulse width." },
    };
}

void RajvikEditor::showPresets()
{
    auto box = std::make_unique<PresetBrowser>(proc, [this](const juce::String& n) { presetName = n; presetBtn.setButtonText(n); presetBtn.repaint(); },
                                               [this] { promptSave(); });
    juce::CallOutBox::launchAsynchronously(std::move(box), presetBtn.getScreenBounds(), nullptr);
}
void RajvikEditor::stepPreset(int dir)
{
    auto& f = proc.presets.factory(); if (f.empty()) return;
    presetIdx = presetIdx < 0 ? (dir > 0 ? 0 : (int) f.size() - 1) : (presetIdx + dir + (int) f.size()) % (int) f.size();
    proc.presets.loadFactory(presetIdx); presetName = f[(size_t) presetIdx].name; presetBtn.setButtonText(presetName);
}
void RajvikEditor::promptSave()
{
    auto* aw = new juce::AlertWindow("Save preset", "Preset name:", juce::MessageBoxIconType::NoIcon);
    aw->addTextEditor("name", presetName); aw->addButton("Save", 1); aw->addButton("Cancel", 0);
    juce::Component::SafePointer<RajvikEditor> self(this);
    aw->enterModalState(true, juce::ModalCallbackFunction::create([self, aw](int r) {
        if (r == 1 && self != nullptr) { self->proc.presets.saveUser(aw->getTextEditorContents("name")); self->presetName = aw->getTextEditorContents("name"); self->presetBtn.setButtonText(self->presetName); }
    }), true);
}
void RajvikEditor::showSettings()
{
    auto panel = std::make_unique<SettingsPanel>(proc.apvts, [this](int act) {
        switch (act) {
            case 0: proc.applyModeDefaults(); break;
            case 1: promptSave(); break;
            case 2: proc.copySettings(); break;
            case 3: proc.pasteSettings(); break;
            case 4: proc.clearMidiMap(); break;
            default: proc.undo.beginNewTransaction("Reset"); proc.presets.resetToDefaults(); break;
        }
    });
    juce::CallOutBox::launchAsynchronously(std::move(panel), gearBtn.getScreenBounds(), nullptr);
}
juce::File RajvikEditor::sizeFile() { return rdp::PresetManager::userDir().getParentDirectory().getChildFile("gui_default_width.txt"); }
int RajvikEditor::loadDefaultW() { auto f = sizeFile(); const int w = f.existsAsFile() ? f.loadFileAsString().getIntValue() : 0; return juce::jlimit(220, 4400, w > 0 ? w : 1000); }
void RajvikEditor::showGuiMenu()
{
    juce::PopupMenu m; m.addSectionHeader("Window size");
    const int pcts[] = { 25, 33, 50, 75, 100, 125, 150, 200, 250, 300, 400 };
    const int cur = juce::roundToInt((float) getWidth() / 10.f);
    for (int p : pcts) m.addItem(100 + p, juce::String(p) + " %", true, cur == p);
    m.addItem(10, "Custom width...");
    m.addSeparator(); m.addItem(1, "Default size (reset window)"); m.addItem(2, "Save current size as default");
    m.addSeparator(); m.addItem(3, "Enlarge / shrink (x1.5)"); m.addItem(4, "Show help hints", true, helpOn);
    m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&guiBtn), [this](int r) {
        if (r >= 125 && r <= 500) setScalePercent(r - 100);
        else if (r == 1) setScalePercent(juce::roundToInt((float) loadDefaultW() / 10.f));
        else if (r == 2) sizeFile().replaceWithText(juce::String(getWidth()));
        else if (r == 3) fullBtn.onClick();
        else if (r == 4) { helpOn = !helpOn; hint = {}; repaint(); }
        else if (r == 10) {
            auto* aw = new juce::AlertWindow("Custom size", "Window width in pixels (220 - 4400):", juce::MessageBoxIconType::NoIcon);
            aw->addTextEditor("w", juce::String(getWidth())); aw->addButton("OK", 1); aw->addButton("Cancel", 0);
            juce::Component::SafePointer<RajvikEditor> self(this);
            aw->enterModalState(true, juce::ModalCallbackFunction::create([self, aw](int res) {
                if (res == 1 && self != nullptr) { const int w = juce::jlimit(220, 4400, aw->getTextEditorContents("w").getIntValue()); self->setSize(w, juce::roundToInt((float) w * self->canvasH() / 1100.f)); }
            }), true);
        }
    });
}
void RajvikEditor::setScalePercent(int pct) { const int w = juce::jlimit(220, 4400, juce::roundToInt(1000.f * (float) pct / 100.f)); setSize(w, juce::roundToInt((float) w * canvasH() / 1100.f)); enlarged = false; }

void RajvikEditor::timerCallback()
{
    display.pull(); mbDisplay.pull();
    meter.update(display.curIn, display.curOut, display.curGr);
    meter.mom = proc.mMom; meter.sho = proc.mShort; meter.integ = proc.mInt; meter.tp = proc.mTp; meter.rmsIn = proc.mRmsIn; meter.rmsOut = proc.mRmsOut;
    proc.syncMidiMap();
    auto& a = proc.apvts;
    const bool mb = ui::getP(a, "mbOn") > 0.5f; if (mb != mbShown) applyModeLayout();
    const float scDb = proc.scMeter.load(); scRing = scDb > scRing ? scDb : scRing - 1.0f; thr.ring = juce::jlimit(0.f, 1.f, (scRing + 60.f) / 60.f);
    stylePill.text = rdp::kModes[(int) ui::getP(a, "mode")].name; stylePill.repaint();
    static_cast<CharIcon*>(charIcon.get())->lit = ui::getP(a, "character") > 0.5f; charIcon->repaint();
    if (charPanel.isVisible()) charPanel.refresh();
    const char* osn[] = { "Off", "2x", "4x", "8x", "16x", "32x" };
    static_cast<StatusText*>(osItem.get())->setValue(osn[(int) ui::getP(a, "oversampling")]);
    static_cast<StatusText*>(lookItem.get())->setValue(ui::getP(a, "lookOn") > 0.5f ? "On (" + juce::String(ui::getP(a, "lookahead"), ui::getP(a, "lookahead") < 10.f ? 1 : 0) + " ms)" : juce::String("Off"));
    static_cast<StatusText*>(midiItem.get())->prefix = learnOn ? "MIDI Learn: ON" : "MIDI Learn"; midiItem->repaint();
    mbBtn.setToggleState(mb, juce::dontSendNotification);
    const int det = (int) ui::getP(a, "detector"); peakBtn.setToggleState(det == 1, juce::dontSendNotification); rmsBtn.setToggleState(det == 2, juce::dontSendNotification);
    eqOnBtn.setToggleState(ui::getP(a, "scEqOn") > 0.5f, juce::dontSendNotification);
    const bool ms = ui::getP(a, "stereoMode") > 0.5f; stereoPill.text = ms ? "MID / SIDE" : "STEREO"; stereoPill.repaint();
    if (midBtn.isVisible() != (scOpen && ms)) applyLayoutVisibility();
    const int tg = triggerNow(); for (int i = 0; i < 4; ++i) trig[i].setToggleState(i == tg, juce::dontSendNotification);
    if (scOpen) { eqGraph.pull(); applyLayoutVisibility(); }
}

void RajvikEditor::paint(juce::Graphics& g)
{
    const float s = k(), W = (float) getWidth(), H = (float) getHeight();
    g.setGradientFill(juce::ColourGradient(cTop, 0.f, 0.f, cBot, 0.f, H, false)); g.fillAll();
    // header
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff2a2a2f), 0.f, 0.f, cHdr, 0.f, 40.f * s, false)); g.fillRect(0.f, 0.f, W, 40.f * s);
    g.setColour(cLine); g.fillRect(0.f, 40.f * s - 1.f, W, 1.f);
    ui::drawIcon(g, "logo", juce::Rectangle<float>(16.f * s, 11.f * s, 38.f * s, 18.f * s), cYellow, 1.8f * s);
    { const auto fa = F(17.f * s, true, 0.12f); g.setFont(fa); g.setColour(juce::Colour(0xffe4e4ea)); const float wa = textW("RAJVIK", fa); g.drawText("RAJVIK", juce::Rectangle<float>(64.f * s, 6.f * s, wa + 6.f, 28.f * s), juce::Justification::centredLeft);
      const auto fb = F(15.f * s, false, 0.06f); g.setFont(fb); g.setColour(juce::Colour(0xff9b9ba6)); const float wb = textW("Dynamics", fb); g.drawText("Dynamics", juce::Rectangle<float>(64.f * s + wa + 10.f * s, 6.f * s, wb + 6.f, 28.f * s), juce::Justification::centredLeft);
      g.setColour(cYellow); g.setFont(F(15.f * s, true, 0.06f)); g.drawText("Pro", juce::Rectangle<float>(64.f * s + wa + wb + 20.f * s, 6.f * s, 40.f * s, 28.f * s), juce::Justification::centredLeft); }
    // side chain panel
    if (scOpen) {
        const float y0 = 620.f * s; g.setGradientFill(juce::ColourGradient(juce::Colour(0xff222227), 0.f, y0, juce::Colour(0xff151518), 0.f, y0 + 260.f * s, false)); g.fillRect(0.f, y0, W, 260.f * s);
        g.setColour(cLine); g.fillRect(0.f, y0, W, 1.f);
        g.setColour(juce::Colour(0x12ffffff)); g.fillRect(0.f, y0 + 1.f, W, 1.f);
        g.setColour(cDim); g.setFont(F(9.5f * s, true, 0.1f)); g.drawText("SC LEVEL", Ri(8, 826, 70, 14).toFloat(), juce::Justification::centredLeft);
        g.drawText(juce::String(scRing <= -99.f ? juce::String("--") : juce::String(scRing, 1)) + " dB", Ri(1008, 850, 88, 14).toFloat(), juce::Justification::centred);   // detector level
        g.setColour(cFaint); g.setFont(F(9.f * s, true, 0.08f)); g.drawText("TRIGGER", Ri(46, 628, 100, 10).toFloat(), juce::Justification::centredLeft);
        const char* src[] = { "Internal", "External", "MIDI", "Host tempo" }; g.setColour(cText.withAlpha(0.9f)); g.setFont(F(11.f * s)); g.drawText(src[triggerNow()], Ri(46, 664, 110, 16).toFloat(), juce::Justification::centredLeft);
    }
    // status bar
    const float sy = (canvasH() - 40.f) * s; g.setGradientFill(juce::ColourGradient(juce::Colour(0xff232328), 0.f, sy, juce::Colour(0xff17171a), 0.f, H, false)); g.fillRect(0.f, sy, W, H - sy);
    g.setColour(cLine); g.fillRect(0.f, sy, W, 1.f);
    // the right-hand meter column: also paint the scale region while the side chain panel covers it
    g.setColour(cDim); g.setFont(F(9.5f * s, true, 0.1f));
}

void RajvikEditor::paintOverChildren(juce::Graphics& g)
{
    const float s = k();
    if (helpOn) {   // hint bubble (like the context help of the reference plug-in)
        const juce::String text = hint.isEmpty() ? "Move the mouse over a control to see what it does." : hint;
        auto r = Ri(560, 52, 360, 100).toFloat(); const auto f = F(12.f * s); juce::AttributedString as; as.setJustification(juce::Justification::topLeft);
        as.append(text, f, juce::Colour(0xff111114)); juce::TextLayout tl; tl.createLayout(as, r.getWidth() - 20.f * s); r.setHeight(tl.getHeight() + 22.f * s);
        g.setColour(juce::Colours::black.withAlpha(0.3f)); g.fillRoundedRectangle(r.translated(0.f, 3.f * s), 8.f * s);
        g.setColour(juce::Colour(0xffd6d6da)); g.fillRoundedRectangle(r, 8.f * s); tl.draw(g, r.reduced(10.f * s, 10.f * s));
    }
}

void RajvikEditor::resized()
{
    const float H0 = canvasH(); const float sy = H0 - 40.f;   // status bar top
    // header
    undoBtn.setBounds(Ri(440, 6, 28, 28)); redoBtn.setBounds(Ri(470, 6, 28, 28)); abBtn.setBounds(Ri(520, 4, 64, 32)); copyBtn.setBounds(Ri(588, 4, 54, 32));
    prevBtn.setBounds(Ri(660, 6, 26, 28)); presetBtn.setBounds(Ri(688, 4, 210, 32)); nextBtn.setBounds(Ri(900, 6, 26, 28));
    helpBtn.setBounds(Ri(950, 4, 50, 32)); guiBtn.setBounds(Ri(1010, 6, 28, 28)); fullBtn.setBounds(Ri(1054, 6, 28, 28));
    // display + meter
    display.setBounds(Ri(0, 40, 1040, 580)); mbDisplay.setBounds(Ri(0, 40, 1040, 580)); meter.setBounds(Ri(1040, 40, 60, 580));
    // multiband band strip
    for (int i = 0; i < 4; ++i) bandBtn[i].setBounds(Ri(14.f + (float) i * 70.f, 46, 66, 24));
    soloBtn.setBounds(Ri(300, 46, 36, 24)); muteBtn.setBounds(Ri(340, 46, 36, 24)); bypBtn.setBounds(Ri(380, 46, 46, 24)); bandsBox.setBounds(Ri(436, 46, 96, 24)); bandGain.setBounds(Ri(548, 46, 220, 24));
    // knob row (overlaid on the lower part of the display)
    thr.setBounds(Ri(112, 400, 156, 124)); ratio.setBounds(Ri(280, 426, 110, 100)); atk.setBounds(Ri(552, 426, 110, 100)); rel.setBounds(Ri(690, 426, 130, 100));
    stylePill.setBounds(Ri(398, 438, 144, 32)); charIcon->setBounds(Ri(445, 476, 50, 22)); charPanel.setBounds(Ri(344, 296, 252, 124));
    gain.setBounds(Ri(826, 410, 100, 80)); dry.setBounds(Ri(826, 496, 100, 72));
    knee.setBounds(Ri(158, 556, 120, 38)); range.setBounds(Ri(290, 556, 120, 38)); scToggle->setBounds(Ri(412, 568, 116, 26)); look.setBounds(Ri(566, 556, 120, 38)); hold.setBounds(Ri(698, 556, 120, 38));
    // status bar
    midiItem->setBounds(Ri(14, sy + 8, 120, 24)); osItem->setBounds(Ri(250, sy + 8, 170, 24)); lookItem->setBounds(Ri(440, sy + 8, 190, 24)); mbBtn.setBounds(Ri(668, sy + 8, 90, 24));
    mixV.setBounds(Ri(846, sy + 8, 56, 24)); inV.setBounds(Ri(910, sy + 8, 64, 24)); outV.setBounds(Ri(978, sy + 8, 64, 24)); gearBtn.setBounds(Ri(1056, sy + 6, 30, 28));
    // side chain panel
    listenBtn.setBounds(Ri(10, 628, 26, 26)); for (int i = 0; i < 4; ++i) trig[i].setBounds(Ri(46.f + (float) i * 25.f, 640, 24, 22));
    scLevelSl.setBounds(Ri(14, 664, 20, 150)); scLevelV.setBounds(Ri(2, 842, 54, 20));
    linkKnob.setBounds(Ri(66, 694, 90, 88)); stereoPill.setBounds(Ri(58, 790, 100, 22)); midBtn.setBounds(Ri(58, 818, 48, 20)); sideBtn.setBounds(Ri(110, 818, 48, 20)); wetPanV.setBounds(Ri(58, 842, 100, 18)); dryPanV.setBounds(Ri(58, 861, 100, 18));
    eqGraph.setBounds(Ri(168, 660, 832, 214));
    bTypeBox.setBounds(Ri(168, 628, 104, 26)); bFreq.setBounds(Ri(280, 628, 108, 26)); bGain.setBounds(Ri(392, 628, 96, 26)); bQ.setBounds(Ri(492, 628, 80, 26)); bSlopeBox.setBounds(Ri(492, 628, 80, 26));
    bChBox.setBounds(Ri(580, 628, 84, 26)); bOn.setBounds(Ri(672, 628, 44, 26));
    scOnBtn.setBounds(Ri(1008, 630, 88, 22)); eqOnBtn.setBounds(Ri(1008, 658, 88, 22)); eqBandsBox.setBounds(Ri(1008, 688, 88, 24)); routeBox.setBounds(Ri(1008, 718, 88, 24));
    invertBtn.setBounds(Ri(1008, 750, 88, 22)); peakBtn.setBounds(Ri(1008, 778, 42, 22)); rmsBtn.setBounds(Ri(1054, 778, 42, 22));
    // knob geometry the ProKnob uses depends on its own bounds only
    (void) sy;
}
