#include "PluginEditor.h"

RajvikEditor::RajvikEditor(RajvikProcessor& p)
    : AudioProcessorEditor(&p), proc(p), display(p), eqGraph(p)
{
    setLookAndFeel(&lnf);
    auto& a = proc.apvts;
    for (auto* c : std::initializer_list<juce::Component*>{ &display, &meter, &eqGraph, &undoBtn, &redoBtn, &abBtn, &copyBtn, &pasteBtn, &saveBtn,
         &scBtn, &clearMidiBtn, &learnBtn, &syncBtn, &stereoBox, &rateBox, &offsetSl, &pulseSl, &offsetLbl, &pulseLbl, &defaultsBtn, &resetBtn, &presetBox, &modeBox, &charBox, &detBox, &osBox, &slopeBox, &autoGainBtn, &autoRelBtn, &extScBtn, &listenBtn, &modeDesc })
        addAndMakeVisible(c);
    eqGraph.setVisible(false); slopeBox.setVisible(false); extScBtn.setVisible(false); listenBtn.setVisible(false);

    thr.init(*this, a, "threshold", "THRESHOLD"); ratio.init(*this, a, "ratio", "RATIO"); atk.init(*this, a, "attack", "ATTACK");
    rel.init(*this, a, "release", "RELEASE"); mix.init(*this, a, "mix", "MIX");
    knee.init(*this, a, "knee", "Knee"); range.init(*this, a, "range", "Range"); hold.init(*this, a, "hold", "Hold");
    look.init(*this, a, "lookahead", "Lookahead"); link.init(*this, a, "link", "Link"); inG.init(*this, a, "inGain", "Input");
    outG.init(*this, a, "outGain", "Output"); drive.init(*this, a, "drive", "Drive");

    for (int i = 0; i < rdp::kNumModes; ++i) modeBox.addItem(rdp::kModes[i].name, i + 1);
    charBox.addItemList({ "Clean", "Tube", "Diode", "Bright" }, 1); detBox.addItemList({ "Detector: Style", "Detector: Peak", "Detector: RMS" }, 1);
    osBox.addItemList({ "Oversampling: Off", "2x", "4x", "8x", "16x", "32x" }, 1); slopeBox.addItemList({ "HPF 12 dB", "HPF 24 dB", "HPF 48 dB" }, 1);
    modeA = std::make_unique<SA::ComboBoxAttachment>(a, "mode", modeBox); charA = std::make_unique<SA::ComboBoxAttachment>(a, "character", charBox);
    detA = std::make_unique<SA::ComboBoxAttachment>(a, "detector", detBox); osA = std::make_unique<SA::ComboBoxAttachment>(a, "oversampling", osBox);
    slopeA = std::make_unique<SA::ComboBoxAttachment>(a, "scHpfSlope", slopeBox);
    agA = std::make_unique<SA::ButtonAttachment>(a, "autoGain", autoGainBtn); arA = std::make_unique<SA::ButtonAttachment>(a, "autoRelease", autoRelBtn);
    exA = std::make_unique<SA::ButtonAttachment>(a, "scExternal", extScBtn); liA = std::make_unique<SA::ButtonAttachment>(a, "scListen", listenBtn);

    stereoBox.addItemList({ "Stereo link", "Mid/Side" }, 1); rateBox.addItemList({ "1/1", "1/2", "1/4", "1/8", "1/16", "1/32" }, 1);
    stereoA = std::make_unique<SA::ComboBoxAttachment>(a, "stereoMode", stereoBox); rateA = std::make_unique<SA::ComboBoxAttachment>(a, "syncRate", rateBox);
    syncA = std::make_unique<SA::ButtonAttachment>(a, "syncOn", syncBtn);
    offA = std::make_unique<SA::SliderAttachment>(a, "syncOffset", offsetSl); pulA = std::make_unique<SA::SliderAttachment>(a, "syncPulse", pulseSl);
    for (auto* l : { &offsetLbl, &pulseLbl }) l->setColour(juce::Label::textColourId, ui::dim);
    for (auto* sl : { &offsetSl, &pulseSl }) sl->setTextBoxStyle(juce::Slider::TextBoxRight, false, 62, 18);
    // MIDI learn: enable "MIDI Learn", touch a knob, then move a hardware CC
    learnBtn.setClickingTogglesState(true);
    learnBtn.onClick = [this] { if (!learnBtn.getToggleState()) proc.cancelLearn(); };
    clearMidiBtn.onClick = [this] { proc.clearMidiMap(); };
    auto hook = [this](Knob& k, const char* id) { k.s.onDragStart = [this, id] { if (learnBtn.getToggleState()) proc.beginLearn(id); }; };
    hook(thr, "threshold"); hook(ratio, "ratio"); hook(atk, "attack"); hook(rel, "release"); hook(mix, "mix"); hook(knee, "knee");
    hook(range, "range"); hook(hold, "hold"); hook(look, "lookahead"); hook(link, "link"); hook(inG, "inGain"); hook(outG, "outGain"); hook(drive, "drive");
    meter.onReset = [this] { proc.meterReset = true; };
    proc.metersWanted = true;
    modeDesc.setColour(juce::Label::textColourId, ui::dim); modeDesc.setFont(juce::FontOptions(12.f));
    modeBox.onChange = [this] { modeDesc.setText(rdp::kModes[std::max(0, modeBox.getSelectedId() - 1)].description, juce::dontSendNotification); };
    modeBox.onChange();

    undoBtn.onClick = [this] { proc.undo.undo(); }; redoBtn.onClick = [this] { proc.undo.redo(); };
    abBtn.onClick = [this] { proc.toggleAB(); abBtn.setButtonText(proc.currentAB() ? "B" : "A"); };
    copyBtn.onClick = [this] { proc.copySettings(); }; pasteBtn.onClick = [this] { proc.pasteSettings(); };
    defaultsBtn.onClick = [this] { proc.applyModeDefaults(); };
    resetBtn.onClick = [this] { proc.undo.beginNewTransaction("Reset"); proc.presets.resetToDefaults(); };
    saveBtn.onClick = [this] { promptSave(); };
    scBtn.setClickingTogglesState(true);
    scBtn.onClick = [this] {
        scOpen = scBtn.getToggleState();
        eqGraph.setVisible(scOpen); slopeBox.setVisible(scOpen); extScBtn.setVisible(scOpen); listenBtn.setVisible(scOpen); resized();
    };
    presetBox.setTextWhenNothingSelected("Presets");
    rebuildPresetMenu();
    presetBox.onChange = [this] {
        const int id = presetBox.getSelectedId();
        if (id >= 1 && id < 1000) proc.presets.loadFactory(id - 1);
        else if (id >= 1000) { auto u = proc.presets.userPresets(); if (id - 1000 < u.size()) proc.presets.loadUser(u[id - 1000]); }
    };

    setResizable(true, true); setResizeLimits(760, 520, 1800, 1100); setSize(980, 680);
    startTimerHz(30);
}

void RajvikEditor::rebuildPresetMenu()
{
    presetBox.clear(juce::dontSendNotification);
    juce::String cat; int i = 1;
    for (auto& f : proc.presets.factory()) {
        if (f.category != cat) { cat = f.category; presetBox.addSectionHeading(cat); }
        presetBox.addItem(f.name, i++);
    }
    auto u = proc.presets.userPresets();
    if (u.size()) presetBox.addSectionHeading("User");
    for (int k = 0; k < u.size(); ++k) presetBox.addItem(u[k].getFileNameWithoutExtension(), 1000 + k);
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

void RajvikEditor::timerCallback()
{
    display.pull();
    meter.update(display.curIn, display.curOut, display.curGr);
    meter.mom = proc.mMom; meter.sho = proc.mShort; meter.integ = proc.mInt; meter.tp = proc.mTp; meter.rmsIn = proc.mRmsIn; meter.rmsOut = proc.mRmsOut;
    proc.syncMidiMap();
}

void RajvikEditor::paint(juce::Graphics& g)
{
    g.fillAll(ui::bg);
    g.setColour(ui::accent); g.setFont(juce::FontOptions(20.f, juce::Font::bold)); g.drawText("RAJVIK", 12, 8, 90, 26, juce::Justification::left);
    g.setColour(ui::dim); g.setFont(juce::FontOptions(11.f)); g.drawText("DYNAMICS PRO", 12, 26, 110, 12, juce::Justification::left);
}

void RajvikEditor::resized()
{
    auto r = getLocalBounds().reduced(8);
    auto head = r.removeFromTop(34); head.removeFromLeft(124);
    for (auto* b : { &undoBtn, &redoBtn, &abBtn, &copyBtn, &pasteBtn }) { b->setBounds(head.removeFromLeft(54).reduced(2)); }
    presetBox.setBounds(head.removeFromLeft(200).reduced(2)); saveBtn.setBounds(head.removeFromLeft(54).reduced(2));
    resetBtn.setBounds(head.removeFromLeft(54).reduced(2)); scBtn.setBounds(head.removeFromRight(100).reduced(2));
    r.removeFromTop(4);
    auto bar = r.removeFromBottom(30); r.removeFromBottom(4);
    learnBtn.setBounds(bar.removeFromLeft(100)); clearMidiBtn.setBounds(bar.removeFromLeft(86).reduced(2)); stereoBox.setBounds(bar.removeFromLeft(100).reduced(2));
    syncBtn.setBounds(bar.removeFromLeft(100)); rateBox.setBounds(bar.removeFromLeft(62).reduced(2));
    offsetLbl.setBounds(bar.removeFromLeft(44)); offsetSl.setBounds(bar.removeFromLeft(130));
    pulseLbl.setBounds(bar.removeFromLeft(40)); pulseSl.setBounds(bar.removeFromLeft(130));
    meter.setBounds(r.removeFromRight(110)); r.removeFromRight(8);
    display.setBounds(r.removeFromTop((int) (r.getHeight() * 0.34f))); r.removeFromTop(6);

    if (scOpen) {
        auto sc = r.removeFromBottom((int) (r.getHeight() * 0.38f)); r.removeFromBottom(6);
        auto side = sc.removeFromRight(130); eqGraph.setBounds(sc.withTrimmedRight(6));
        slopeBox.setBounds(side.removeFromTop(28).reduced(2)); extScBtn.setBounds(side.removeFromTop(28)); listenBtn.setBounds(side.removeFromTop(28));
    }
    auto big = r.removeFromTop((int) (r.getHeight() * 0.50f));
    const int bw = big.getWidth() / 5;
    for (auto* k : { &thr, &ratio, &atk, &rel, &mix }) k->place(big.removeFromLeft(bw));
    auto sel = r.removeFromBottom(30);
    auto small = r;
    const int sw = small.getWidth() / 8;
    for (auto* k : { &knee, &range, &hold, &look, &link, &inG, &outG, &drive }) k->place(small.removeFromLeft(sw));
    modeBox.setBounds(sel.removeFromLeft(110).reduced(2)); defaultsBtn.setBounds(sel.removeFromLeft(100).reduced(2));
    charBox.setBounds(sel.removeFromLeft(80).reduced(2));
    detBox.setBounds(sel.removeFromLeft(130).reduced(2)); osBox.setBounds(sel.removeFromLeft(140).reduced(2));
    autoGainBtn.setBounds(sel.removeFromLeft(90)); autoRelBtn.setBounds(sel.removeFromLeft(110));
    modeDesc.setBounds(sel);
}
