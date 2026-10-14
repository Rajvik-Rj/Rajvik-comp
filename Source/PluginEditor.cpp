#include "PluginEditor.h"

namespace {
// popup with the controls that have no place on the main panel (character/drive, tempo sync, preset + state actions)
class SettingsPanel : public juce::Component {
public:
    using SA = juce::AudioProcessorValueTreeState;
    SettingsPanel(SA& a, std::function<void(int)> act) : action(std::move(act)) {
        setLookAndFeel(&lnf);
        ui::fillCombo(charBox, a, "character"); ui::fillCombo(rateBox, a, "syncRate");
        charA = std::make_unique<SA::ComboBoxAttachment>(a, "character", charBox); rateA = std::make_unique<SA::ComboBoxAttachment>(a, "syncRate", rateBox);
        syncA = std::make_unique<SA::ButtonAttachment>(a, "syncOn", syncBtn);
        driveA = std::make_unique<SA::SliderAttachment>(a, "drive", drive); offA = std::make_unique<SA::SliderAttachment>(a, "syncOffset", offset); pulA = std::make_unique<SA::SliderAttachment>(a, "syncPulse", pulse);
        syncBtn.getProperties().set("style", "pill");
        for (auto* s : { &drive, &offset, &pulse }) s->setTextBoxStyle(juce::Slider::TextBoxRight, false, 64, 20);
        const char* names[] = { "Character", "Drive", "Tempo Sync (single band)", "Rate", "Offset", "Pulse width" };
        for (int i = 0; i < 6; ++i) { lbl[i].setText(names[i], juce::dontSendNotification); lbl[i].setColour(juce::Label::textColourId, ui::dim); addAndMakeVisible(lbl[i]); }
        const char* acts[] = { "Mode defaults", "Save preset...", "Copy settings", "Paste settings", "Clear MIDI map", "Reset all" };
        for (int i = 0; i < 6; ++i) { btn[i].setButtonText(acts[i]); btn[i].onClick = [this, i] { action(i); }; addAndMakeVisible(btn[i]); }
        for (auto* c : std::initializer_list<juce::Component*>{ &charBox, &drive, &syncBtn, &rateBox, &offset, &pulse }) addAndMakeVisible(c);
        setSize(340, 6 * 34 + 3 * 34 + 24);
    }
    ~SettingsPanel() override { setLookAndFeel(nullptr); }
    void paint(juce::Graphics& g) override { g.fillAll(juce::Colour(0xff111827)); }
    void resized() override {
        auto r = getLocalBounds().reduced(12); juce::Component* rows[] = { &charBox, &drive, &syncBtn, &rateBox, &offset, &pulse };
        for (int i = 0; i < 6; ++i) { auto row = r.removeFromTop(34); lbl[i].setBounds(row.removeFromLeft(130)); rows[i]->setBounds(row.reduced(0, 3)); }
        r.removeFromTop(6);
        for (int i = 0; i < 6; i += 2) { auto row = r.removeFromTop(34); btn[i].setBounds(row.removeFromLeft(row.getWidth() / 2).reduced(2)); btn[i + 1].setBounds(row.reduced(2)); }
    }
private:
    RajvikLnF lnf; std::function<void(int)> action; juce::ComboBox charBox, rateBox; juce::ToggleButton syncBtn; juce::Label lbl[6]; juce::TextButton btn[6];
    juce::Slider drive{ juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight }, offset{ juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight }, pulse{ juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight };
    std::unique_ptr<SA::ComboBoxAttachment> charA, rateA; std::unique_ptr<SA::ButtonAttachment> syncA; std::unique_ptr<SA::SliderAttachment> driveA, offA, pulA;
};
}

RajvikEditor::RajvikEditor(RajvikProcessor& p)
    : AudioProcessorEditor(&p), proc(p), display(p), mbDisplay(p, selBand, [this] { onBandSelected(); }),
      eqGraph(p, eqSel, [this] { onEqSelected(); }), eqTable(p, eqSel, [this] { onEqSelected(); })
{
    setLookAndFeel(&lnf);
    auto& a = proc.apvts;
    for (auto* c : std::initializer_list<juce::Component*>{ &display, &mbDisplay, &meter, &undoBtn, &redoBtn, &prevBtn, &nextBtn, &fullBtn, &abBtn, &copyBtn, &helpBtn, &presetBox,
         &modeBox, &modeDesc, &singleBtn, &multiBtn, &soloBtn, &muteBtn, &bypBtn, &bandsBox, &bandGain, &inG, &thr, &ratio, &atk, &rel, &mix, &outG, &knee, &range, &hold, &look, &scLevel,
         &autoGainBtn, &autoRelBtn, &stereoBox, &linkSl, &peakBtn, &rmsBtn, &scOnBtn, &chevBtn, &eqPowerBtn, &eqBandsBox, &srcBox, &routeBox, &eqModeBox, &osBox,
         &toolBell, &toolShelf, &toolCut, &listenBtn, &invertBtn, &midBtn, &sideBtn, &eqGraph, &eqTable, &midiBox, &osItem, &lookItem, &mixItem, &outItem, &scaleItem, &gearBtn })
        addAndMakeVisible(c);
    for (int i = 0; i < 4; ++i) {
        bandBtn[i].setButtonText("Band " + juce::String(i + 1)); bandBtn[i].getProperties().set("tc", (int) ui::bandCol[i].getARGB());
        bandBtn[i].onClick = [this, i] { selBand = i; onBandSelected(); }; addAndMakeVisible(bandBtn[i]);
    }

    // ---- fixed knob bindings ----
    inG.bind(a, "inGain"); mix.bind(a, "mix"); outG.bind(a, "outGain"); knee.bind(a, "knee"); range.bind(a, "range"); hold.bind(a, "hold"); look.bind(a, "lookahead"); scLevel.bind(a, "scLevel");
    bindMainKnobs();
    // MIDI learn: enable "MIDI Learn" in the bottom-left menu, touch a knob, then move a hardware CC
    for (auto* kb : { &inG, &thr, &ratio, &atk, &rel, &mix, &outG, &knee, &range, &hold, &look, &scLevel })
        kb->onTouch = [this, kb] { if (learnOn) proc.beginLearn(kb->paramId); };

    // ---- header ----
    undoBtn.onClick = [this] { proc.undo.undo(); }; redoBtn.onClick = [this] { proc.undo.redo(); };
    abBtn.onClick = [this] { proc.toggleAB(); abBtn.underline = proc.currentAB(); abBtn.repaint(); }; abBtn.underline = proc.currentAB();
    copyBtn.onClick = [this] { proc.copyToOtherAB(); };   // copy the active A/B slot into the other one
    helpBtn.onClick = [this] { showHelp(); };
    fullBtn.onClick = [this] { if (!enlarged) { normalW = getWidth(); setSize(1728, 1152); } else setSize(normalW, normalW * 2 / 3); enlarged = !enlarged; };
    presetBox.setTextWhenNothingSelected("Default Preset"); rebuildPresetMenu();
    presetBox.onChange = [this] {
        const int id = presetBox.getSelectedId();
        if (id >= 1 && id < 1000) proc.presets.loadFactory(id - 1);
        else if (id >= 1000) { auto u = proc.presets.userPresets(); if (id - 1000 < u.size()) proc.presets.loadUser(u[id - 1000]); }
    };
    prevBtn.onClick = [this] { stepPreset(-1); }; nextBtn.onClick = [this] { stepPreset(1); };

    // ---- style + single / multiband switch ----
    for (int i = 0; i < rdp::kNumModes; ++i) modeBox.addItem(rdp::kModes[i].name, i + 1);
    modeA = std::make_unique<SA::ComboBoxAttachment>(a, "mode", modeBox);
    modeBox.setColour(juce::ComboBox::textColourId, ui::accent);
    modeDesc.setColour(juce::Label::textColourId, ui::text.withAlpha(0.85f));
    singleBtn.onClick = [this] { ui::gesture(proc.apvts, "mbOn", true); ui::setP(proc.apvts, "mbOn", 0.f); ui::gesture(proc.apvts, "mbOn", false); };
    multiBtn.onClick = [this] { ui::gesture(proc.apvts, "mbOn", true); ui::setP(proc.apvts, "mbOn", 1.f); ui::gesture(proc.apvts, "mbOn", false); };

    // ---- band strip (multiband) ----
    ui::fillCombo(bandsBox, a, "mbBands"); bandsA = std::make_unique<SA::ComboBoxAttachment>(a, "mbBands", bandsBox);
    bandGain.setTextBoxStyle(juce::Slider::TextBoxRight, false, 70, 22);
    for (auto* b : { &soloBtn, &muteBtn, &bypBtn }) b->setClickingTogglesState(true);
    soloBtn.getProperties().set("tc", (int) ui::accent.getARGB()); muteBtn.getProperties().set("tc", (int) ui::red.getARGB()); bypBtn.getProperties().set("tc", (int) ui::dim.brighter(0.4f).getARGB());
    bindBandControls();

    // ---- right-hand controls ----
    autoGainBtn.getProperties().set("style", "pill"); autoRelBtn.getProperties().set("style", "dot");
    agA = std::make_unique<SA::ButtonAttachment>(a, "autoGain", autoGainBtn); arA = std::make_unique<SA::ButtonAttachment>(a, "autoRelease", autoRelBtn);
    ui::fillCombo(stereoBox, a, "stereoMode"); stereoA = std::make_unique<SA::ComboBoxAttachment>(a, "stereoMode", stereoBox);
    linkSl.setTextBoxStyle(juce::Slider::TextBoxRight, false, 44, 18); linkA = std::make_unique<SA::SliderAttachment>(a, "link", linkSl);
    for (auto* b : { &peakBtn, &rmsBtn }) b->setClickingTogglesState(false);
    peakBtn.onClick = [this] { const bool on = ui::getP(proc.apvts, "detector") == 1.f; ui::gesture(proc.apvts, "detector", true); ui::setP(proc.apvts, "detector", on ? 0.f : 1.f); ui::gesture(proc.apvts, "detector", false); };
    rmsBtn.onClick = [this] { const bool on = ui::getP(proc.apvts, "detector") == 2.f; ui::gesture(proc.apvts, "detector", true); ui::setP(proc.apvts, "detector", on ? 0.f : 2.f); ui::gesture(proc.apvts, "detector", false); };

    // ---- sidechain & EQ panel ----
    scOnBtn.getProperties().set("style", "header"); scOnA = std::make_unique<SA::ButtonAttachment>(a, "scOn", scOnBtn);
    chevBtn.onClick = [this] { scOpen = !scOpen; chevBtn.icon = scOpen ? "chevUp" : "chev"; resized(); repaint(); };
    eqPowerBtn.setClickingTogglesState(false);
    eqPowerBtn.onClick = [this] { const float on = ui::getP(proc.apvts, "scEqOn") > 0.5f ? 0.f : 1.f; ui::gesture(proc.apvts, "scEqOn", true); ui::setP(proc.apvts, "scEqOn", on); ui::gesture(proc.apvts, "scEqOn", false); };
    ui::fillCombo(eqBandsBox, a, "eqBands"); eqBandsA = std::make_unique<SA::ComboBoxAttachment>(a, "eqBands", eqBandsBox);
    ui::fillCombo(srcBox, a, "scExternal"); srcA = std::make_unique<SA::ComboBoxAttachment>(a, "scExternal", srcBox);
    ui::fillCombo(routeBox, a, "scRoute"); routeA = std::make_unique<SA::ComboBoxAttachment>(a, "scRoute", routeBox);
    ui::fillCombo(eqModeBox, a, "scEqOn"); eqModeA = std::make_unique<SA::ComboBoxAttachment>(a, "scEqOn", eqModeBox);
    ui::fillCombo(osBox, a, "oversampling"); osA = std::make_unique<SA::ComboBoxAttachment>(a, "oversampling", osBox);
    for (auto* b : { &toolBell, &toolShelf, &toolCut }) b->setClickingTogglesState(true);
    toolBell.onClick = [this] { setEqType(0); }; toolShelf.onClick = [this] { setEqType(1); }; toolCut.onClick = [this] { setEqType(2); };
    for (auto* b : { &listenBtn, &invertBtn, &midBtn, &sideBtn }) b->setClickingTogglesState(true);
    listenA = std::make_unique<SA::ButtonAttachment>(a, "scListen", listenBtn); invertA = std::make_unique<SA::ButtonAttachment>(a, "scInvert", invertBtn);
    midA = std::make_unique<SA::ButtonAttachment>(a, "procMid", midBtn); sideA = std::make_unique<SA::ButtonAttachment>(a, "procSide", sideBtn);

    // ---- status bar ----
    midiBox.setTextWhenNothingSelected("MIDI Learn"); midiBox.addItem("Toggle MIDI Learn", 1); midiBox.addItem("Clear MIDI mappings", 2);
    midiBox.onChange = [this] {
        const int id = midiBox.getSelectedId();
        if (id == 1) { learnOn = !learnOn; if (!learnOn) proc.cancelLearn(); midiBox.setTextWhenNothingSelected(learnOn ? "MIDI Learn: ON" : "MIDI Learn"); }
        else if (id == 2) proc.clearMidiMap();
        midiBox.setSelectedId(0, juce::dontSendNotification);
    };
    osItem.onClick = [this] {
        juce::PopupMenu m; const int cur = (int) ui::getP(proc.apvts, "oversampling"); const char* n[] = { "Off", "2x", "4x", "8x", "16x", "32x" };
        for (int i = 0; i < 6; ++i) m.addItem(i + 1, n[i], true, i == cur);
        m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&osItem), [this](int r) { if (r > 0) { ui::gesture(proc.apvts, "oversampling", true); ui::setP(proc.apvts, "oversampling", (float) (r - 1)); ui::gesture(proc.apvts, "oversampling", false); } });
    };
    outItem.onClick = [this] { proc.meterReset = true; meter.inHold = meter.outHold = -100.f; meter.grMax = 0.f; };
    scaleItem.onClick = [this] {
        juce::PopupMenu m; for (int pct : { 75, 100, 125, 150, 200 }) m.addItem(pct, juce::String(pct) + " %");
        m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&scaleItem), [this](int r) { if (r > 0) setScalePercent(r); });
    };
    gearBtn.onClick = [this] { showSettings(); };
    meter.onReset = [this] { proc.meterReset = true; };
    proc.metersWanted = true;

    setResizable(true, true); setResizeLimits(768, 512, 2304, 1536); getConstrainer()->setFixedAspectRatio(1536.0 / 1024.0);
    setSize(1152, 768);
    mbShown = ui::getP(a, "mbOn") > 0.5f; applyModeLayout(); onEqSelected();
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
void RajvikEditor::onEqSelected()
{
    const int t = (int) ui::getP(proc.apvts, "eqType" + juce::String(eqSel + 1));
    toolBell.setToggleState(t == rdp::EqBell, juce::dontSendNotification);
    toolShelf.setToggleState(t == rdp::EqLowShelf || t == rdp::EqHighShelf, juce::dontSendNotification);
    toolCut.setToggleState(rdp::eqIsCut(t), juce::dontSendNotification);
}
void RajvikEditor::setEqType(int kind)   // 0 bell, 1 shelf, 2 cut (low/high picked from the band's frequency)
{
    const auto n = juce::String(eqSel + 1); const bool low = ui::getP(proc.apvts, "eqFreq" + n) < 1000.f;
    const int t = kind == 0 ? rdp::EqBell : (kind == 1 ? (low ? rdp::EqLowShelf : rdp::EqHighShelf) : (low ? rdp::EqHPF : rdp::EqLPF));
    ui::gesture(proc.apvts, "eqType" + n, true); ui::setP(proc.apvts, "eqType" + n, (float) t); ui::gesture(proc.apvts, "eqType" + n, false); onEqSelected();
}

void RajvikEditor::applyModeLayout()
{
    const bool mb = mbShown;
    display.setVisible(!mb); mbDisplay.setVisible(mb);
    for (auto* c : std::initializer_list<juce::Component*>{ &soloBtn, &muteBtn, &bypBtn, &bandsBox, &bandGain, &bandBtn[0], &bandBtn[1], &bandBtn[2], &bandBtn[3] }) c->setVisible(mb);
    singleBtn.setToggleState(!mb, juce::dontSendNotification); multiBtn.setToggleState(mb, juce::dontSendNotification);
    bindMainKnobs(); repaint();
}

void RajvikEditor::setScalePercent(int pct) { const int w = juce::roundToInt(1152.f * (float) pct / 100.f); setSize(w, w * 2 / 3); enlarged = false; }

void RajvikEditor::rebuildPresetMenu()
{
    presetBox.clear(juce::dontSendNotification); juce::String cat; int i = 1;
    for (auto& f : proc.presets.factory()) { if (f.category != cat) { cat = f.category; presetBox.addSectionHeading(cat); } presetBox.addItem(f.name, i++); }
    auto u = proc.presets.userPresets(); if (u.size()) presetBox.addSectionHeading("User");
    for (int j = 0; j < u.size(); ++j) presetBox.addItem(u[j].getFileNameWithoutExtension(), 1000 + j);
}
void RajvikEditor::stepPreset(int dir)
{
    const int n = presetBox.getNumItems(); if (n == 0) return;
    int idx = presetBox.getSelectedItemIndex(); idx = idx < 0 ? (dir > 0 ? 0 : n - 1) : (idx + dir + n) % n;
    presetBox.setSelectedItemIndex(idx, juce::sendNotificationSync);
}
void RajvikEditor::promptSave()
{
    auto* aw = new juce::AlertWindow("Save preset", "Preset name:", juce::MessageBoxIconType::NoIcon);
    aw->addTextEditor("name", "My Preset"); aw->addButton("Save", 1); aw->addButton("Cancel", 0);
    juce::Component::SafePointer<RajvikEditor> self(this);
    aw->enterModalState(true, juce::ModalCallbackFunction::create([self, aw](int r) {
        if (r == 1 && self != nullptr) { self->proc.presets.saveUser(aw->getTextEditorContents("name")); self->rebuildPresetMenu(); }
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
void RajvikEditor::showHelp()
{
    juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon, "RAJVIK DYNAMICS PRO",
        "Single / Multiband switch is next to the Style menu.\n\n"
        "MULTIBAND (2-4 bands): drag the white crossover handles, click a band to select it, drag its dashed line to set the band threshold. "
        "Threshold / Ratio / Attack / Release knobs edit the selected band; S / M / Byp = solo / mute / bypass, Gain = band make-up. "
        "Style, Knee, Range, Hold, Lookahead, Detector, Mix, In/Out gain and the sidechain section are shared by all bands.\n\n"
        "SIDECHAIN EQ: drag nodes (freq / gain), mouse wheel on a node = Q, double-click a node = band on/off. "
        "Table: drag freq / gain values, click power to toggle, click the curve icon to change filter type.\n\n"
        "OUT read-out (top right): click to cycle Peak / True-peak / RMS / LUFS M-S-I. Click elsewhere on the meters to reset.\n"
        "Gear (bottom right): Character, Drive, Tempo Sync, preset and state actions.", "OK", this);
}

void RajvikEditor::timerCallback()
{
    display.pull(); mbDisplay.pull();
    meter.update(display.curIn, display.curOut, display.curGr);
    meter.mom = proc.mMom; meter.sho = proc.mShort; meter.integ = proc.mInt; meter.tp = proc.mTp; meter.rmsIn = proc.mRmsIn; meter.rmsOut = proc.mRmsOut;
    proc.syncMidiMap();
    auto& a = proc.apvts;
    const bool mb = ui::getP(a, "mbOn") > 0.5f; if (mb != mbShown) { mbShown = mb; applyModeLayout(); }
    const int det = (int) ui::getP(a, "detector"); peakBtn.setToggleState(det == 1, juce::dontSendNotification); rmsBtn.setToggleState(det == 2, juce::dontSendNotification);
    const auto desc = juce::String(rdp::kModes[(int) ui::getP(a, "mode")].description);
    if (desc != lastDesc) { lastDesc = desc; modeDesc.setText(desc, juce::dontSendNotification); }
    eqPowerBtn.setToggleState(ui::getP(a, "scEqOn") > 0.5f, juce::dontSendNotification);
    onEqSelected(); eqGraph.pull(); eqTable.repaint();
    const char* osn[] = { "Off", "2x", "4x", "8x", "16x", "32x" };
    osItem.setInfo("Oversampling: " + juce::String(osn[(int) ui::getP(a, "oversampling")])); lookItem.setInfo("Lookahead: " + juce::String(ui::getP(a, "lookahead"), 1) + " ms");
    mixItem.setInfo("Mix: " + juce::String(juce::roundToInt(ui::getP(a, "mix"))) + " %");
    outItem.setInfo("Output: " + (meter.outHold <= -99.f ? juce::String("--") : juce::String(meter.outHold, 1)) + " dB");
    scaleItem.setInfo("Scale: " + juce::String(juce::roundToInt((float) getWidth() / 1152.f * 100.f)) + " %");
}

void RajvikEditor::place(KnobBox& kb, juce::Rectangle<float> cell, const KnobGeom& g)
{
    kb.setBounds(Ri(cell.getX(), cell.getY(), cell.getWidth(), cell.getHeight())); kb.setGeom(g, cell.getTopLeft());
}

void RajvikEditor::paint(juce::Graphics& g)
{
    const float s = k();
    g.setGradientFill(juce::ColourGradient(ui::bgTop, 0.f, 0.f, ui::bg, 0.f, (float) getHeight(), false)); g.fillAll();
    auto R = [&](float x, float y, float w, float h) { return juce::Rectangle<float>(x * s, y * s, w * s, h * s); };
    auto box = [&](juce::Rectangle<float> r, float rad = 10.f) { g.setColour(ui::panel); g.fillRoundedRectangle(r, rad * s); g.setColour(ui::panelEdge); g.drawRoundedRectangle(r.reduced(0.5f), rad * s, 1.f); };
    auto txt = [&](const juce::String& t, float x, float cy, float size, juce::Colour c, juce::Justification j, bool bold = false, float kern = 0.f, float w = 400.f) {
        g.setFont(ui::font(size * s, bold, kern)); g.setColour(c);
        const float ww = w * s, left = j == juce::Justification::centredLeft ? x * s : (j == juce::Justification::centredRight ? x * s - ww : x * s - ww * 0.5f);
        g.drawText(t, juce::Rectangle<float>(left, cy * s - size * s, ww, size * s * 2.f), j); };
    const auto L = juce::Justification::centredLeft; const auto C = juce::Justification::centred;

    // header
    ui::drawIcon(g, "logo", R(34, 20, 48, 32), ui::accent, 2.6f * s);
    txt("RAJVIK", 100, 38, 37, ui::text, L, true, 0.34f); txt("DYNAMICS", 271, 28, 24, ui::text, L, false, 0.20f); txt("PRO", 395, 28, 24, ui::accent, L, true, 0.20f);
    txt("ADVANCED DYNAMICS PROCESSOR", 271, 48, 13, ui::dim, L, false, 0.12f);
    g.setColour(ui::panelEdge); for (float x : { 250.f, 917.f, 1445.f }) g.drawLine(x * s, 14.f * s, x * s, 54.f * s, 1.f);

    // main display + meter panels
    box(R(18, 80, 1304, 360)); box(R(1340, 80, 180, 360));
    txt("Style", 40, 100, 17, ui::text, L);

    // knob row
    box(R(18, 458, 1326, 217)); g.setColour(ui::panelEdge);
    for (float x : { 160.f, 498.f, 996.f }) g.drawLine(x * s, 472.f * s, x * s, 662.f * s, 1.f);
    g.drawRoundedRectangle(R(1036, 466, 134, 34), 17.f * s, 1.f);
    for (float y : { 535.f, 603.f }) { g.drawLine(1028.f * s, y * s, 1038.f * s, y * s, 1.f); }
    for (float y : { 607.f, 659.f }) { g.drawLine(1196.f * s, y * s, 1208.f * s, y * s, 1.f); }
    box(R(1362, 460, 158, 215));
    ui::drawIcon(g, "link", R(1380, 472, 28, 28), ui::text.withAlpha(0.85f), 1.5f * s); txt("Stereo Link", 1416, 486, 15, ui::text.withAlpha(0.9f), L);
    g.setColour(ui::panelEdge); g.drawLine(1378.f * s, 563.f * s, 1506.f * s, 563.f * s, 1.f);
    txt("Detector", 1380, 586, 15, ui::text.withAlpha(0.9f), L);
    txt("Link", 1380, 556, 12, ui::dim, L, false, 0.f, 40.f);

    // sidechain & EQ
    const float scH = scOpen ? 262.f : 46.f; box(R(18, 688, 1502, scH));
    if (scOpen) {
        box(R(995, 730, 338, 206)); box(R(1363, 730, 157, 206));
        txt("Source", 36, 746, 14, ui::dim, L); txt("Routing", 36, 794, 14, ui::dim, L);
        txt("Sidechain EQ", 1377, 751, 14, ui::dim, L); txt("M / S", 1377, 814, 14, ui::dim, L); txt("Oversampling", 1377, 884, 14, ui::dim, L);
        g.setColour(ui::fieldEdge); g.drawRoundedRectangle(R(994, 696, 114, 30), 6.f * s, 1.f);
        txt("EQ band type", 1114, 711, 12, ui::faint, L, false, 0.f, 90.f);
    }
    // bottom bar
    g.setColour(juce::Colour(0xff0a0e16)); g.fillRect(R(0, 960, 1536, 64)); g.setColour(ui::panelEdge); g.drawLine(0.f, 960.f * s, (float) getWidth(), 960.f * s, 1.f);
    if (mbShown) txt("Gain", 636, 419, 14, ui::dim, L, false, 0.f, 40.f);
}

void RajvikEditor::resized()
{
    const float s = k();
    // header
    undoBtn.setBounds(Ri(632, 16, 36, 36)); redoBtn.setBounds(Ri(682, 16, 36, 36)); abBtn.setBounds(Ri(752, 14, 64, 40)); copyBtn.setBounds(Ri(826, 14, 60, 40));
    prevBtn.setBounds(Ri(945, 16, 34, 36)); presetBox.setBounds(Ri(991, 15, 249, 36)); nextBtn.setBounds(Ri(1252, 16, 34, 36));
    helpBtn.setBounds(Ri(1362, 14, 60, 40)); fullBtn.setBounds(Ri(1474, 16, 36, 36));
    // display
    modeBox.setBounds(Ri(88, 85, 136, 30)); modeDesc.setBounds(Ri(246, 87, 560, 26)); modeDesc.setFont(ui::font(14.f * s));
    singleBtn.setBounds(Ri(1058, 85, 100, 30)); multiBtn.setBounds(Ri(1164, 85, 128, 30));
    display.setBounds(Ri(30, 118, 1282, 320)); mbDisplay.setBounds(Ri(30, 118, 1282, 320)); meter.setBounds(Ri(1340, 80, 180, 360));
    for (int i = 0; i < 4; ++i) bandBtn[i].setBounds(Ri(70.f + (float) i * 72.f, 406, 68, 26));
    soloBtn.setBounds(Ri(364, 406, 40, 26)); muteBtn.setBounds(Ri(408, 406, 40, 26)); bypBtn.setBounds(Ri(452, 406, 52, 26)); bandsBox.setBounds(Ri(516, 406, 104, 26)); bandGain.setBounds(Ri(676, 406, 240, 26));
    // knob row (design coordinates from the reference image)
    auto G = [](float cx, float cy, float r, const char* cap, float capX, float capY, float capSize, float valX, float valY, float valSize, const char* lo, float loX, float loY, const char* hi, float hiX, float hiY) {
        KnobGeom g; g.cx = cx; g.cy = cy; g.r = r; g.cap = cap; g.capX = capX; g.capY = capY; g.capSize = capSize; g.valX = valX; g.valY = valY; g.valSize = valSize; g.valW = 150.f; g.valH = 26.f;
        g.lo = lo; g.loX = loX; g.loY = loY; g.hi = hi; g.hiX = hiX; g.hiY = hiY; return g; };
    place(inG, { 30, 466, 124, 170 }, G(92, 530, 46, "IN GAIN", 92, 594, 15, 92, 617, 16, "-24", 52, 576, "+24", 131, 576));
    place(thr, { 176, 462, 146, 196 }, G(249, 527, 66, "THRESHOLD", 249, 611, 15, 249, 636, 20, "-60", 196, 591, "0", 304, 591));
    place(ratio, { 340, 470, 148, 188 }, G(414, 533, 56, "RATIO", 414, 611, 15, 414, 636, 17, "1:1", 364, 585, "100:1", 463, 585));
    place(atk, { 506, 470, 148, 188 }, G(580, 533, 56, "ATTACK", 580, 611, 15, 580, 636, 17, "0.01", 528, 584, "500", 632, 584));
    place(rel, { 676, 470, 148, 188 }, G(750, 533, 56, "RELEASE", 750, 611, 15, 750, 636, 17, "1", 700, 584, "5000", 803, 583));
    place(mix, { 838, 470, 148, 188 }, G(912, 533, 56, "MIX", 912, 611, 15, 912, 636, 17, "0", 869, 584, "100", 957, 584));
    {   // output gain: no value box, the value replaces the caption while dragging
        KnobGeom g; g.cx = 1256; g.cy = 505; g.r = 41; g.cap = "OUTPUT GAIN"; g.capX = 1256; g.capY = 560; g.capSize = 14; g.valW = 0; g.valueOnDrag = true;
        g.lo = "-24"; g.loX = 1213; g.loY = 537; g.hi = "+24"; g.hiX = 1303; g.hiY = 537; place(outG, { 1180, 462, 152, 112 }, g); }
    auto Sd = [](float cx, float cy, float r, const char* cap, float capY, float boxCx, float boxY, float boxW) {
        KnobGeom g; g.cx = cx; g.cy = cy; g.r = r; g.cap = cap; g.capX = cx + 31.f; g.capY = capY; g.capSize = 14; g.capLeft = true; g.valX = boxCx; g.valY = boxY; g.valW = boxW; g.valH = 22; g.valSize = 13; g.valBox = true; return g; };
    place(knee, { 1030, 508, 128, 64 }, Sd(1058, 535, 19, "Knee", 527, 1114, 555, 52));
    place(range, { 1030, 576, 128, 64 }, Sd(1058, 603, 19, "Range", 595, 1114, 623, 52));
    place(hold, { 1198, 586, 128, 50 }, Sd(1227, 607, 18, "Hold", 600, 1288, 621, 66));
    place(look, { 1198, 636, 128, 46 }, Sd(1228, 659, 14, "Lookahead", 641, 1288, 661, 66));
    autoGainBtn.setBounds(Ri(1042, 470, 124, 26)); autoRelBtn.setBounds(Ri(686, 642, 124, 22));
    stereoBox.setBounds(Ri(1378, 506, 129, 28)); linkSl.setBounds(Ri(1412, 540, 96, 18));
    peakBtn.setBounds(Ri(1380, 604, 62, 30)); rmsBtn.setBounds(Ri(1444, 604, 62, 30));

    // sidechain & EQ
    scOnBtn.setBounds(Ri(30, 696, 200, 28)); chevBtn.setBounds(Ri(222, 696, 28, 28)); eqBandsBox.setBounds(Ri(876, 697, 108, 26));
    toolBell.setBounds(Ri(996, 698, 36, 26)); toolShelf.setBounds(Ri(1033, 698, 36, 26)); toolCut.setBounds(Ri(1070, 698, 36, 26)); eqPowerBtn.setBounds(Ri(1474, 694, 36, 32));
    for (auto* c : std::initializer_list<juce::Component*>{ &eqBandsBox, &toolBell, &toolShelf, &toolCut, &eqPowerBtn, &srcBox, &routeBox, &eqModeBox, &osBox, &listenBtn, &invertBtn, &midBtn, &sideBtn, &eqGraph, &eqTable, &scLevel })
        c->setVisible(scOpen);
    srcBox.setBounds(Ri(36, 757, 186, 26)); routeBox.setBounds(Ri(36, 805, 186, 26));
    {   KnobGeom g; g.cx = 68; g.cy = 884; g.r = 22; g.cap = "Sidechain Level"; g.capX = 36; g.capY = 846; g.capSize = 14; g.capLeft = true; g.valX = 128; g.valY = 880; g.valW = 46; g.valH = 22; g.valSize = 13; g.valBox = true;
        g.lo = "-24"; g.loX = 38; g.loY = 910; g.hi = "+24"; g.hiX = 118; g.hiY = 910; scLevel.setBounds(Ri(28, 836, 150, 86)); scLevel.setGeom(g, { 28, 836 }); }
    listenBtn.setBounds(Ri(36, 924, 93, 24)); invertBtn.setBounds(Ri(140, 924, 84, 24));
    eqGraph.setBounds(Ri(258, 735, 718, 205)); eqTable.setBounds(Ri(995, 730, 338, 206));
    eqModeBox.setBounds(Ri(1377, 766, 127, 28)); midBtn.setBounds(Ri(1377, 830, 62, 26)); sideBtn.setBounds(Ri(1442, 830, 62, 26)); osBox.setBounds(Ri(1377, 897, 127, 28));

    // status bar
    midiBox.setBounds(Ri(22, 972, 141, 35)); osItem.setBounds(Ri(444, 972, 172, 35)); lookItem.setBounds(Ri(690, 972, 160, 35)); mixItem.setBounds(Ri(930, 972, 120, 35));
    outItem.setBounds(Ri(1146, 972, 146, 35)); scaleItem.setBounds(Ri(1344, 972, 130, 35)); gearBtn.setBounds(Ri(1480, 972, 36, 35));
}
