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
        lookA = std::make_unique<SA::ButtonAttachment>(a, "lookOn", lookBtn); clampA = std::make_unique<SA::SliderAttachment>(a, "clamp", clamp); clamp.setTextBoxStyle(juce::Slider::TextBoxRight, false, 64, 20);
        for (auto* s : { &offset, &pulse }) s->setTextBoxStyle(juce::Slider::TextBoxRight, false, 64, 20);
        const char* names[] = { "Host tempo trigger", "Rate", "Offset", "Pulse width", "Lookahead", "Clamp (leaked peaks)" };
        for (int i = 0; i < 6; ++i) { lbl[i].setText(names[i], juce::dontSendNotification); lbl[i].setColour(juce::Label::textColourId, ui::dim); addAndMakeVisible(lbl[i]); }
        const char* acts[] = { "Style defaults", "Save preset...", "Copy settings", "Paste settings", "Clear MIDI map", "Reset all" };
        for (int i = 0; i < 6; ++i) { btn[i].setButtonText(acts[i]); btn[i].onClick = [this, i] { action(i); }; addAndMakeVisible(btn[i]); }
        for (auto* c : std::initializer_list<juce::Component*>{ &syncBtn, &rateBox, &offset, &pulse, &lookBtn, &clamp }) addAndMakeVisible(c);
        setSize(340, 6 * 34 + 3 * 34 + 24);
    }
    ~SettingsPanel() override { setLookAndFeel(nullptr); }
    void paint(juce::Graphics& g) override { g.fillAll(juce::Colour(0xff1e1e22)); }
    void resized() override {
        auto r = getLocalBounds().reduced(12); juce::Component* rows[] = { &syncBtn, &rateBox, &offset, &pulse, &lookBtn, &clamp };
        for (int i = 0; i < 6; ++i) { auto row = r.removeFromTop(34); lbl[i].setBounds(row.removeFromLeft(130)); rows[i]->setBounds(row.reduced(0, 3)); }
        r.removeFromTop(6);
        for (int i = 0; i < 6; i += 2) { auto row = r.removeFromTop(34); btn[i].setBounds(row.removeFromLeft(row.getWidth() / 2).reduced(2)); btn[i + 1].setBounds(row.reduced(2)); }
    }
private:
    RajvikLnF lnf; std::function<void(int)> action; juce::ComboBox rateBox; juce::ToggleButton syncBtn, lookBtn; juce::Label lbl[6]; juce::Slider clamp{ juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight }; std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> clampA; juce::TextButton btn[6];
    juce::Slider offset{ juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight }, pulse{ juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight };
    std::unique_ptr<SA::ComboBoxAttachment> rateA; std::unique_ptr<SA::ButtonAttachment> syncA, lookA; std::unique_ptr<SA::SliderAttachment> offA, pulA;
};
} // namespace

RajvikEditor::RajvikEditor(RajvikProcessor& p)
    : AudioProcessorEditor(&p), proc(p), display(p), mbDisplay(p, selBand, [this] { onBandSelected(); }), charPanel(p.apvts), eqGraph(p, eqSel, [this] { onEqSelected(); })
{
    setLookAndFeel(&lnf);
    auto& a = proc.apvts;
    scToggle = std::make_unique<LabelButton>("SIDE CHAIN");
    for (auto* c : std::initializer_list<juce::Component*>{ &display, &mbDisplay, &meter, &undoBtn, &redoBtn, &aBtn, &bBtn, &copyBtn, &detBtn, &prevBtn, &nextBtn, &presetBtn, &saveBtn, &toolsBtn, &guiBtn,
         &soloBtn, &muteBtn, &bypBtn, &extBtn, &bandsBox, &bandGain, &stylePill, &autoThrBtn, &autoRelBtn, &autoGainBtn, &charBtn, &thr, &ratio, &atk, &rel, &gain, &mixK,
         &knee, &range, &look, &hold, &dryG, &wetMS, &dryMS, scToggle.get(), &trig[0], &trig[1], &trig[2], &trig[3], &eqOnBtn, &listenBtn, &scOnBtn, &invertBtn, &midBtn, &sideBtn,
         &scLevelSl, &linkSl, &routeBox, &eqBandsBox, &eqGraph, &bTypeBox, &bSlopeBox, &bChBox, &bFreq, &bGain, &bQ, &bOn, &osBtn, &midiBtn, &modeBtn, &mbBtn, &inV })
        addAndMakeVisible(c);
    addChildComponent(charPanel);
    for (int i = 0; i < 4; ++i) { bandBtn[i].setButtonText("Band " + juce::String(i + 1)); bandBtn[i].getProperties().set("tc", (int) ui::bandCol[i].getARGB()); bandBtn[i].onClick = [this, i] { selBand = i; onBandSelected(); }; addAndMakeVisible(bandBtn[i]); }

    // ---- control cards ----
    for (auto* kb : { &thr, &ratio, &atk, &rel, &gain, &mixK }) kb->mini = true;
    thr.caption = "THRESHOLD"; ratio.caption = "RATIO"; atk.caption = "ATTACK"; rel.caption = "RELEASE"; gain.caption = "WET GAIN"; gain.bipolar = true; gain.bind(a, "outGain"); mixK.caption = "MIX"; mixK.bind(a, "mix");
    knee.caption = "KNEE"; knee.bind(a, "knee"); range.caption = "RANGE"; range.bind(a, "range"); look.caption = "LOOKAHEAD"; look.bind(a, "lookahead"); hold.caption = "HOLD"; hold.bind(a, "hold");
    dryG.caption = "DRY GAIN"; dryG.bind(a, "dry"); dryG.col = juce::Colour(0xffa78bfa);
    wetMS.caption = "WET  M / S"; wetMS.bal = true; wetMS.bind(a, "wetPan"); wetMS.col = juce::Colour(0xffa78bfa);
    dryMS.caption = "DRY  M / S"; dryMS.bal = true; dryMS.bind(a, "dryPan"); dryMS.col = juce::Colour(0xffa78bfa);
    scLevelSl.caption = "SC LEVEL"; scLevelSl.bind(a, "scLevel"); linkSl.caption = "STEREO LINK"; linkSl.bind(a, "link");
    bindMainKnobs();
    for (auto* b : { &autoThrBtn, &autoRelBtn, &autoGainBtn, &listenBtn, &scOnBtn, &invertBtn, &midBtn, &sideBtn, &bOn }) b->setClickingTogglesState(true);
    autoThrA = std::make_unique<SA::ButtonAttachment>(a, "autoThr", autoThrBtn); autoRelA = std::make_unique<SA::ButtonAttachment>(a, "autoRelease", autoRelBtn); autoGainA = std::make_unique<SA::ButtonAttachment>(a, "autoGain", autoGainBtn);
    listenA = std::make_unique<SA::ButtonAttachment>(a, "scListen", listenBtn); scOnA = std::make_unique<SA::ButtonAttachment>(a, "scOn", scOnBtn); invertA = std::make_unique<SA::ButtonAttachment>(a, "scInvert", invertBtn);
    midA = std::make_unique<SA::ButtonAttachment>(a, "procMid", midBtn); sideA = std::make_unique<SA::ButtonAttachment>(a, "procSide", sideBtn);
    // MIDI learn: enable it in the status bar, touch a control, then move a hardware CC
    for (auto* kb : { &thr, &ratio, &atk, &rel, &gain, &mixK }) kb->onTouch = [this, kb] { if (learnOn) proc.beginLearn(kb->paramId); };
    struct SL { pro::CurveSlider* s; const char* id; };
    for (auto e : { SL{ &knee, "knee" }, SL{ &range, "range" }, SL{ &look, "lookahead" }, SL{ &hold, "hold" }, SL{ &dryG, "dry" }, SL{ &wetMS, "wetPan" }, SL{ &dryMS, "dryPan" }, SL{ &scLevelSl, "scLevel" }, SL{ &linkSl, "link" } })
        e.s->onTouch = [this, id = juce::String(e.id)] { if (learnOn) proc.beginLearn(id); };

    // ---- header ----
    undoBtn.onClick = [this] { proc.undo.undo(); }; redoBtn.onClick = [this] { proc.undo.redo(); };
    aBtn.onClick = [this] { if (proc.currentAB() != 0) proc.toggleAB(); }; bBtn.onClick = [this] { if (proc.currentAB() != 1) proc.toggleAB(); }; copyBtn.onClick = [this] { proc.copyToOtherAB(); };
    detBtn.caret = true; detBtn.onClick = [this] {
        juce::PopupMenu m; const int cur = (int) ui::getP(proc.apvts, "detector"); const char* n[] = { "Style default", "Peak", "RMS" };
        for (int i = 0; i < 3; ++i) m.addItem(i + 1, n[i], true, i == cur);
        m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&detBtn), [this](int r) { if (r > 0) { ui::gesture(proc.apvts, "detector", true); ui::setP(proc.apvts, "detector", (float) (r - 1)); ui::gesture(proc.apvts, "detector", false); } });
    };
    prevBtn.onClick = [this] { stepPreset(-1); }; nextBtn.onClick = [this] { stepPreset(1); }; presetBtn.onClick = [this] { showPresets(); }; saveBtn.onClick = [this] { promptSave(); };
    toolsBtn.caret = true; toolsBtn.onClick = [this] { showSettings(); }; guiBtn.onClick = [this] { showGuiMenu(); };

    // ---- style + character ----
    stylePill.caret = true; charBtn.caret = true;
    stylePill.onClick = [this] {
        juce::PopupMenu m; const bool mb = mbShown; const auto id = mb ? "mbMode" + juce::String(selBand + 1) : juce::String("mode"); const int cur = (int) ui::getP(proc.apvts, id);
        if (mb) { m.addSectionHeader("Style of band " + juce::String(selBand + 1)); m.addItem(1, "Follow main style", true, cur == 0); }
        for (int i = 0; i < rdp::kNumModes; ++i) { const int v = mb ? i + 1 : i; m.addItem(v + 1, rdp::kModes[i].name, true, v == cur); }
        m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&stylePill).withMinimumWidth(200), [this, id](int r) { if (r > 0) { ui::gesture(proc.apvts, id, true); ui::setP(proc.apvts, id, (float) (r - 1)); ui::gesture(proc.apvts, id, false); } });
    };
    charBtn.onClick = [this] { showCharacterPanel(!charPanel.isVisible()); };
    scToggle->onClick = [this] { setSidechainOpen(!scOpen); };

    // ---- status bar ----
    osBtn.onClick = [this] {
        juce::PopupMenu m; const int cur = (int) ui::getP(proc.apvts, "oversampling"); const char* n[] = { "Off", "2x", "4x", "8x", "16x", "32x" };
        for (int i = 0; i < 6; ++i) m.addItem(i + 1, n[i], true, i == cur);
        m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&osBtn), [this](int r) { if (r > 0) { ui::gesture(proc.apvts, "oversampling", true); ui::setP(proc.apvts, "oversampling", (float) (r - 1)); ui::gesture(proc.apvts, "oversampling", false); } });
    };
    midiBtn.onClick = [this] {
        juce::PopupMenu m; m.addItem(1, "MIDI Learn (touch a control, then move a CC)", true, learnOn); m.addItem(2, "Clear MIDI mappings");
        m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&midiBtn), [this](int r) { if (r == 1) { learnOn = !learnOn; if (!learnOn) proc.cancelLearn(); } else if (r == 2) proc.clearMidiMap(); });
    };
    modeBtn.caret = true; modeBtn.onClick = [this] {
        juce::PopupMenu m; const int cur = (int) ui::getP(proc.apvts, "stereoMode"); m.addItem(1, "Left / Right", true, cur == 0); m.addItem(2, "Mid / Side", true, cur == 1);
        m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&modeBtn), [this](int r) { if (r > 0) { ui::gesture(proc.apvts, "stereoMode", true); ui::setP(proc.apvts, "stereoMode", (float) (r - 1)); ui::gesture(proc.apvts, "stereoMode", false); } });
    };
    mbBtn.onClick = [this] { const bool on = ui::getP(proc.apvts, "mbOn") > 0.5f; ui::gesture(proc.apvts, "mbOn", true); ui::setP(proc.apvts, "mbOn", on ? 0.f : 1.f); ui::gesture(proc.apvts, "mbOn", false); };
    osBtn.caret = true; inV.bind(a, "inGain"); inV.prefix = "Input"; inV.alignLeft = false;
    meter.onReset = [this] { proc.meterReset = true; }; proc.metersWanted = true;

    // ---- band strip (multiband) ----
    ui::fillCombo(bandsBox, a, "mbBands"); bandsA = std::make_unique<SA::ComboBoxAttachment>(a, "mbBands", bandsBox);
    bandGain.setTextBoxStyle(juce::Slider::TextBoxRight, false, 70, 22);
    for (auto* b : { &soloBtn, &muteBtn, &bypBtn }) b->setClickingTogglesState(true); extBtn.setClickingTogglesState(true);
    soloBtn.getProperties().set("tc", (int) ui::accent.getARGB()); muteBtn.getProperties().set("tc", (int) ui::red.getARGB()); bypBtn.getProperties().set("tc", (int) ui::dim.brighter(0.4f).getARGB());
    bindBandControls();

    // ---- side chain panel ----
    for (int i = 0; i < 4; ++i) { trig[i].onClick = [this, i] { if (i == 3 && triggerNow() == 3) showSettings(); else setTrigger(i); }; }
    eqOnBtn.onClick = [this] { const float on = ui::getP(proc.apvts, "scEqOn") > 0.5f ? 0.f : 1.f; ui::gesture(proc.apvts, "scEqOn", true); ui::setP(proc.apvts, "scEqOn", on); ui::gesture(proc.apvts, "scEqOn", false); };
    ui::fillCombo(routeBox, a, "scRoute"); routeA = std::make_unique<SA::ComboBoxAttachment>(a, "scRoute", routeBox);
    ui::fillCombo(eqBandsBox, a, "eqBands"); eqBandsA = std::make_unique<SA::ComboBoxAttachment>(a, "eqBands", eqBandsBox);
    ui::fillCombo(bTypeBox, a, "eqType1"); ui::fillCombo(bSlopeBox, a, "eqSlope1"); ui::fillCombo(bChBox, a, "eqMs1");
    bFreq.prefix = "FREQ"; bGain.prefix = "GAIN"; bQ.prefix = "Q";
    bindEqBand();

    setResizable(true, true); setResizeLimits(232, 140, 4640, 2800); getConstrainer()->setFixedAspectRatio(1160.0 / canvasH());
    { const int dw = loadDefaultW(); setSize(dw, juce::roundToInt((float) dw * canvasH() / 1160.f)); }
    mbShown = ui::getP(a, "mbOn") > 0.5f; applyModeLayout(); onEqSelected(); registerHints();
    addMouseListener(this, true);
    startTimerHz(30);
}

RajvikEditor::~RajvikEditor() { stopTimer(); closeOverlays(); proc.metersWanted = false; proc.cancelLearn(); setLookAndFeel(nullptr); }
void RajvikEditor::closeOverlays()
{
    juce::PopupMenu::dismissAllActiveMenus();
    if (auto* b = dynamic_cast<juce::CallOutBox*>(openBox.getComponent())) b->dismiss();
    if (auto* a = dynamic_cast<juce::AlertWindow*>(openAlert.getComponent())) a->exitModalState(0);
    openBox = nullptr; openAlert = nullptr;
}


void RajvikEditor::bindMainKnobs()
{
    auto& a = proc.apvts; const bool mb = ui::getP(a, "mbOn") > 0.5f; const auto n = juce::String(selBand + 1);
    thr.bind(a, mb ? "mbThr" + n : juce::String("threshold")); ratio.bind(a, mb ? "mbRatio" + n : juce::String("ratio"));
    atk.bind(a, mb ? "mbAtk" + n : juce::String("attack")); rel.bind(a, mb ? "mbRel" + n : juce::String("release"));
}
void RajvikEditor::bindBandControls()
{
    auto& a = proc.apvts; const auto n = juce::String(selBand + 1);
    soloA.reset(); muteA.reset(); bypA.reset(); bandGainA.reset(); extA.reset(); extA = std::make_unique<SA::ButtonAttachment>(a, "mbExt" + n, extBtn);
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
    for (auto* c : std::initializer_list<juce::Component*>{ &soloBtn, &muteBtn, &bypBtn, &extBtn, &bandsBox, &bandGain, &bandBtn[0], &bandBtn[1], &bandBtn[2], &bandBtn[3] }) c->setVisible(mb);
    for (auto* c : std::initializer_list<juce::Component*>{ &scLevelSl, &linkSl, &scOnBtn, &invertBtn, &routeBox, &eqBandsBox, &eqGraph, &bTypeBox, &bOn, &bChBox, &bFreq }) c->setVisible(sc);
    const int t = (int) ui::getP(proc.apvts, "eqType" + juce::String(eqSel + 1));
    bGain.setVisible(sc && rdp::eqHasGain(t)); bQ.setVisible(sc && !rdp::eqIsCut(t)); bSlopeBox.setVisible(sc && rdp::eqIsCut(t));
    const bool ms = ui::getP(proc.apvts, "stereoMode") > 0.5f; midBtn.setVisible(sc && ms); sideBtn.setVisible(sc && ms);
    for (auto* c : std::initializer_list<juce::Component*>{ &eqGraph, &bTypeBox, &bOn, &bChBox, &bFreq, &bGain, &bQ, &bSlopeBox, &eqBandsBox }) { c->setEnabled(!mb); c->setAlpha(mb ? 0.35f : 1.f); }   // no sidechain EQ in multiband
    display.showCurve = !mb;
}
void RajvikEditor::toggleEnlarge()
{
    if (!enlarged) { normalW = getWidth(); setSize(juce::roundToInt(normalW * 1.5f), juce::roundToInt(normalW * 1.5f * canvasH() / 1160.f)); } else setSize(normalW, juce::roundToInt(normalW * canvasH() / 1160.f));
    enlarged = !enlarged;
}
void RajvikEditor::setSidechainOpen(bool open)
{
    scOpen = open; applyLayoutVisibility(); getConstrainer()->setFixedAspectRatio(1160.0 / canvasH());
    setSize(getWidth(), juce::roundToInt((float) getWidth() * canvasH() / 1160.f)); repaint();
}
void RajvikEditor::showCharacterPanel(bool show) { charPanel.setVisible(show); if (show) { charPanel.refresh(); charPanel.toFront(false); } }
void RajvikEditor::mouseDown(const juce::MouseEvent& e)
{
    if (charPanel.isVisible() && e.eventComponent != &charPanel && !charPanel.isParentOf(e.eventComponent) && e.eventComponent != &charBtn) showCharacterPanel(false);
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
        { &thr, "Threshold: the level above which the compressor starts working. AUTO THR makes it follow the signal level. The thin green ring shows the detector level." },
        { &ratio, "Ratio: how strongly the signal above the threshold is reduced." }, { &atk, "Attack: how fast the compressor reacts to a level above the threshold." },
        { &rel, "Release: how fast the gain recovers. AUTO REL adapts it to the material." }, { &gain, "Wet gain: level of the compressed signal. AUTO GAIN matches loudness to the input." }, { &mixK, "Mix: overall dry/wet of the whole plug-in, after everything." },
        { &knee, "Knee: width of the soft transition around the threshold (shown as the shaded zone in the waveform)." }, { &range, "Range: the maximum amount of gain change in dB." },
        { &look, "Lookahead: delays the audio so the compressor can react before a peak arrives." }, { &hold, "Hold: how long the gain reduction is held before the release starts." },
        { &dryG, "Dry gain: level of the uncompressed signal mixed in (parallel compression)." }, { &wetMS, "Wet balance: slide toward Mid (or Left) or Side (or Right) to boost that part of the compressed signal by up to 6 dB." },
        { &dryMS, "Dry balance: the same Mid / Side (or Left / Right) boost for the dry signal." },
        { &stylePill, "Style: click to choose one of the 14 compression styles (in multiband: the style of the selected band)." }, { &charBtn, "Character: Tube, Diode or Bright saturation with drive and Pre / Post routing." },
        { scToggle.get(), "Side chain: open the trigger section: source, side chain EQ, level and stereo link." }, { &eqGraph, "Side chain EQ: shapes only the trigger signal, not your audio. Right-click a node for the filter type, double-click to switch a band on / off." },
        { &display, "Waveform: grey is below the threshold, gold is what the compressor removes, the white line is the output. The panel on the left shows the knee / curve." },
        { &meter, "Meters: gain reduction and input / output levels. Click the bottom read-out to cycle peak, true peak, RMS and LUFS." },
        { &extBtn, "Ext SC: the selected band listens to the external side chain input instead of this track. Every band can choose its own source." }, { &detBtn, "Detector: Style default, Peak or RMS. Applies to single band and multiband." }, { &modeBtn, "Left / Right or Mid / Side processing." },
        { &presetBtn, "Preset browser: search by name or tag." }, { &guiBtn, "GUI settings: window size from tiny to huge, default size." },
        { &trig[0], "Trigger: internal signal." }, { &trig[1], "Trigger: external side chain input." }, { &trig[2], "Trigger: MIDI notes (active while a note is held)." }, { &trig[3], "Trigger: host-tempo pulse. Click again for rate, offset and pulse width." },
    };
}
void RajvikEditor::showPresets()
{
    juce::Component::SafePointer<RajvikEditor> self(this);
    auto box = std::make_unique<PresetBrowser>(proc, [self](const juce::String& n) { if (self == nullptr) return; self->presetName = n; self->presetBtn.setButtonText(n); self->presetBtn.repaint(); },
                                               [self] { if (self != nullptr) self->promptSave(); });
    closeOverlays(); openBox = &juce::CallOutBox::launchAsynchronously(std::move(box), presetBtn.getScreenBounds(), nullptr);
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
    openAlert = aw; aw->addTextEditor("name", presetName); aw->addButton("Save", 1); aw->addButton("Cancel", 0);
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
    closeOverlays(); openBox = &juce::CallOutBox::launchAsynchronously(std::move(panel), toolsBtn.getScreenBounds(), nullptr);
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
        else if (r == 3) toggleEnlarge();
        else if (r == 4) { helpOn = !helpOn; hint = {}; repaint(); }
        else if (r == 10) {
            auto* aw = new juce::AlertWindow("Custom size", "Window width in pixels (220 - 4400):", juce::MessageBoxIconType::NoIcon);
            openAlert = aw; aw->addTextEditor("w", juce::String(getWidth())); aw->addButton("OK", 1); aw->addButton("Cancel", 0);
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
    extSm = std::max(proc.extPk.load(), extSm * 0.92f);
    { juce::String t = rdp::kModes[(int) ui::getP(a, "mode")].name;
      if (mbShown) { const int v = (int) ui::getP(a, "mbMode" + juce::String(selBand + 1)); t = "B" + juce::String(selBand + 1) + ": " + (v == 0 ? t + " (main)" : juce::String(rdp::kModes[v - 1].name)); }
      stylePill.text = t; stylePill.repaint(); }
    { const char* cn[] = { "Off", "Tube", "Diode", "Bright" }; charBtn.text = juce::String("Character: ") + cn[(int) ui::getP(a, "character")]; charBtn.repaint(); }
    if (charPanel.isVisible()) charPanel.refresh();
    { const char* osn[] = { "Off", "2x", "4x", "8x", "16x", "32x" }; osBtn.text = juce::String("Oversampling  ") + osn[(int) ui::getP(a, "oversampling")]; osBtn.repaint(); }
    { const char* dn[] = { "Style", "Peak", "RMS" }; detBtn.text = dn[(int) ui::getP(a, "detector")]; detBtn.repaint(); }
    const bool ms = ui::getP(a, "stereoMode") > 0.5f; modeBtn.text = ms ? "Mode  M / S" : "Mode  L / R"; modeBtn.repaint();
    midiBtn.text = learnOn ? "MIDI Learn: ON" : "MIDI Learn"; midiBtn.setToggleState(learnOn, juce::dontSendNotification);
    mbBtn.setToggleState(mb, juce::dontSendNotification); aBtn.setToggleState(proc.currentAB() == 0, juce::dontSendNotification); bBtn.setToggleState(proc.currentAB() == 1, juce::dontSendNotification);
    eqOnBtn.setToggleState(ui::getP(a, "scEqOn") > 0.5f, juce::dontSendNotification);
    const int tg = triggerNow(); for (int i = 0; i < 4; ++i) trig[i].setToggleState(i == tg, juce::dontSendNotification);
    wetMS.endL = dryMS.endL = ms ? "MID" : "LEFT"; wetMS.endR = dryMS.endR = ms ? "SIDE" : "RIGHT"; wetMS.caption = ms ? "WET  M / S" : "WET  L / R"; dryMS.caption = ms ? "DRY  M / S" : "DRY  L / R";
    wetMS.repaint(); dryMS.repaint();
    latencyText = "Latency  " + juce::String(proc.getLatencySamples()) + " smp";
    if (midBtn.isVisible() != (scOpen && ms)) applyLayoutVisibility();
    if (scOpen) { eqGraph.pull(); const int t = (int) ui::getP(a, "eqType" + juce::String(eqSel + 1)); if (bGain.isVisible() != (rdp::eqHasGain(t) && !mb)) applyLayoutVisibility(); }
    repaint(Ri(0, 38, 1160, scH()));
}

void RajvikEditor::paint(juce::Graphics& g)
{
    const float s = k(), W = (float) getWidth(), H = (float) getHeight(); const juce::Colour line(0xff23232f), gold(0xfff5c542), dim(0xff8c8c9c);
    g.fillAll(juce::Colour(0xff07070c));
    g.setColour(juce::Colour(0xff0e0e15)); g.fillRect(0.f, 0.f, W, 38.f * s); g.setColour(line); g.fillRect(0.f, 38.f * s - 1.f, W, 1.f);
    g.setColour(juce::Colour(0xffe9e9f0)); g.setFont(F(16.f * s, true, 0.02f)); g.drawText("RAJVIK", Ri(12, 0, 80, 38).toFloat(), juce::Justification::centredLeft);
    g.setColour(gold); g.drawText("Pro Comp", Ri(80, 0, 100, 38).toFloat(), juce::Justification::centredLeft);
    // side chain strip / panel (top)
    const float sh = scH(); g.setColour(juce::Colour(0xff0b0b12)); g.fillRect(0.f, 38.f * s, W, sh * s); g.setColour(line); g.fillRect(0.f, (38.f + sh) * s - 1.f, W, 1.f);
    {
        const int tg = triggerNow(); juce::String d; bool warn = false;
        if (tg == 0) d = "listening to this track"; else if (tg == 1) { if (extSm > 1e-5f) d = "signal on side chain input"; else { d = "no signal on side chain input"; warn = true; } } else if (tg == 2) d = "active while a MIDI note is held"; else d = "pulses with host tempo";
        g.setColour(warn ? juce::Colour(0xffff5d5d) : juce::Colour(0xff5ee08a)); g.fillEllipse(Ri(436, 48, 8, 8).toFloat()); g.setColour(warn ? juce::Colour(0xffff8a8a) : dim); g.setFont(F(10.5f * s)); g.drawText(d, Ri(450, 42, 330, 20).toFloat(), juce::Justification::centredLeft);
        const bool on = std::fabs(display.curGr) > 0.1f; g.setColour(on ? juce::Colour(0xffff5d5d) : juce::Colour(0xff3a3a46)); g.fillEllipse(Ri(826, 48, 8, 8).toFloat());
        g.setColour(on ? juce::Colour(0xffe9e9f0) : dim); g.setFont(F(10.5f * s, on)); g.drawText(on ? "triggered  " + juce::String(display.curGr, 1) + " dB" : "not triggered", Ri(840, 42, 150, 20).toFloat(), juce::Justification::centredLeft);
    }
    // cards
    const float by = 38.f + sh + 422.f;
    for (auto r : { Ri(10, by, 1140, 84), Ri(10, by + 92, 1140, 74) }) { g.setColour(juce::Colour(0xff0f0f17)); g.fillRoundedRectangle(r.toFloat(), 12.f * s); g.setColour(line); g.drawRoundedRectangle(r.toFloat().reduced(0.5f), 12.f * s, 1.f); }
    g.setColour(juce::Colour(0xff5a5a6a)); g.setFont(F(8.5f * s, true, 0.14f)); g.drawText("STYLE", Ri(26, by + 12, 118, 14).toFloat(), juce::Justification::centred);
    { const float x = 598.f * s - 16.f * s; g.setColour(line); g.fillRect(x, (by + 92.f + 14.f) * s, 1.f, 46.f * s); }
    // status bar
    const float sy = (by + 174.f) * s; g.setColour(juce::Colour(0xff0e0e15)); g.fillRect(0.f, sy, W, H - sy); g.setColour(line); g.fillRect(0.f, sy, W, 1.f);
    g.setColour(dim); g.setFont(F(11.f * s)); g.drawText(latencyText, Ri(12, by + 174, 150, 40).toFloat(), juce::Justification::centredLeft);
    g.drawText(presetName, Ri(520, by + 174, 220, 40).toFloat(), juce::Justification::centred);
}

void RajvikEditor::paintOverChildren(juce::Graphics& g)
{
    const float s = k();
    if (helpOn) {   // hint bubble
        const juce::String text = hint.isEmpty() ? "Move the mouse over a control to see what it does." : hint;
        auto r = Ri(280, 74, 380, 100).toFloat(); const auto f = F(12.f * s); juce::AttributedString as; as.setJustification(juce::Justification::topLeft);
        as.append(text, f, juce::Colour(0xff111114)); juce::TextLayout tl; tl.createLayout(as, r.getWidth() - 20.f * s); r.setHeight(tl.getHeight() + 22.f * s);
        g.setColour(juce::Colours::black.withAlpha(0.3f)); g.fillRoundedRectangle(r.translated(0.f, 3.f * s), 8.f * s);
        g.setColour(juce::Colour(0xffd6d6da)); g.fillRoundedRectangle(r, 8.f * s); tl.draw(g, r.reduced(10.f * s, 10.f * s));
    }
}

void RajvikEditor::resized()
{
    const float sh = scH(), vy = 38.f + sh, by = vy + 422.f, sy = by + 174.f;
    // header
    undoBtn.setBounds(Ri(190, 6, 50, 26)); redoBtn.setBounds(Ri(246, 6, 50, 26)); aBtn.setBounds(Ri(314, 6, 30, 26)); bBtn.setBounds(Ri(350, 6, 30, 26)); copyBtn.setBounds(Ri(386, 6, 50, 26));
    detBtn.setBounds(Ri(690, 6, 74, 26)); prevBtn.setBounds(Ri(770, 6, 28, 26)); presetBtn.setBounds(Ri(804, 6, 104, 26)); nextBtn.setBounds(Ri(914, 6, 28, 26));
    saveBtn.setBounds(Ri(948, 6, 52, 26)); toolsBtn.setBounds(Ri(1006, 6, 70, 26)); guiBtn.setBounds(Ri(1096, 5, 30, 28));
    // side chain strip + panel
    scToggle->setBounds(Ri(6, 41, 124, 22));
    for (int i = 0; i < 4; ++i) trig[i].setBounds(Ri(140.f + (float) i * 70.f, 42, 64, 20));
    eqOnBtn.setBounds(Ri(1010, 42, 46, 20)); listenBtn.setBounds(Ri(1062, 42, 86, 20));
    bTypeBox.setBounds(Ri(14, 70, 100, 24)); bFreq.setBounds(Ri(120, 70, 100, 24)); bGain.setBounds(Ri(226, 70, 92, 24)); bQ.setBounds(Ri(324, 70, 72, 24)); bSlopeBox.setBounds(Ri(324, 70, 72, 24));
    bChBox.setBounds(Ri(402, 70, 84, 24)); bOn.setBounds(Ri(492, 70, 44, 24));
    eqGraph.setBounds(Ri(14, 100, 860, 142));
    scLevelSl.setBounds(Ri(894, 70, 250, 48)); linkSl.setBounds(Ri(894, 118, 250, 48)); scOnBtn.setBounds(Ri(894, 176, 120, 24)); eqBandsBox.setBounds(Ri(1022, 176, 122, 24));
    invertBtn.setBounds(Ri(894, 206, 58, 22)); routeBox.setBounds(Ri(958, 206, 100, 22)); midBtn.setBounds(Ri(1064, 206, 36, 22)); sideBtn.setBounds(Ri(1104, 206, 40, 22));
    // main view
    display.setBounds(Ri(0, vy, 900, 412)); mbDisplay.setBounds(Ri(0, vy, 900, 412)); meter.setBounds(Ri(900, vy, 260, 412));
    for (int i = 0; i < 4; ++i) bandBtn[i].setBounds(Ri(10.f + (float) i * 68.f, vy + 6, 64, 22));
    soloBtn.setBounds(Ri(290, vy + 6, 34, 22)); muteBtn.setBounds(Ri(328, vy + 6, 34, 22)); bypBtn.setBounds(Ri(366, vy + 6, 44, 22)); bandsBox.setBounds(Ri(418, vy + 6, 92, 22)); bandGain.setBounds(Ri(520, vy + 6, 200, 22)); extBtn.setBounds(Ri(728, vy + 6, 90, 22));
    // cards
    stylePill.setBounds(Ri(26, by + 30, 118, 28));
    thr.setBounds(Ri(190, by + 14, 140, 56)); ratio.setBounds(Ri(332, by + 14, 140, 56)); atk.setBounds(Ri(474, by + 14, 140, 56)); rel.setBounds(Ri(616, by + 14, 140, 56)); gain.setBounds(Ri(758, by + 14, 140, 56)); mixK.setBounds(Ri(900, by + 14, 130, 56));
    autoThrBtn.setBounds(Ri(1046, by + 6, 96, 22)); autoRelBtn.setBounds(Ri(1046, by + 31, 96, 22)); autoGainBtn.setBounds(Ri(1046, by + 56, 96, 22));
    const float c2 = by + 92.f + 12.f;
    knee.setBounds(Ri(28, c2, 118, 52)); range.setBounds(Ri(166, c2, 118, 52)); look.setBounds(Ri(304, c2, 118, 52)); hold.setBounds(Ri(442, c2, 118, 52));
    dryG.setBounds(Ri(598, c2, 104, 52)); wetMS.setBounds(Ri(722, c2, 136, 52)); dryMS.setBounds(Ri(882, c2, 136, 52)); charBtn.setBounds(Ri(1032, by + 92.f + 24.f, 108, 26));
    charPanel.setBounds(Ri(890, by - 66, 252, 132));
    // status bar
    osBtn.setBounds(Ri(168, sy + 7, 168, 26)); midiBtn.setBounds(Ri(344, sy + 7, 96, 26)); modeBtn.setBounds(Ri(748, sy + 7, 118, 26)); mbBtn.setBounds(Ri(874, sy + 7, 84, 26)); inV.setBounds(Ri(980, sy + 7, 160, 26));
}
