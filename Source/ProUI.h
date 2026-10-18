#pragma once
#include "EditorComponents.h"

// Components for the "graphite" main window: big ringed knobs with AUTO toggles, curved sliders, full-bleed level display,
// right-hand level meter, style pill + character panel, drag values and the preset browser.
namespace pro {
using namespace juce;
using SA = AudioProcessorValueTreeState;

inline const Colour cTop{ 0xff2c2c31 }, cBot{ 0xff121215 }, cHdr{ 0xff1f1f23 }, cLine{ 0xff0a0a0c }, cText{ 0xffd9d9df }, cDim{ 0xff8f8f99 }, cFaint{ 0xff55555d },
    cYellow{ 0xfff4c430 }, cGreen{ 0xff86c93e }, cRed{ 0xffe9474c }, cCurve{ 0xffbccf3c }, cTrack{ 0xff3b3b42 }, cPanel{ 0xff19191d }, cPanelEdge{ 0xff34343b };

inline float K(const Component& c) { return ui::scaleOf(c); }
inline Font F(float h, bool bold = false, float kern = 0.f) { return ui::font(h, bold, kern); }
inline float textW(const String& t, const Font& f) { GlyphArrangement ga; ga.addLineOfText(f, t, 0.f, 0.f); return ga.getBoundingBox(0, -1, true).getWidth(); }

// ------------------------------------------------------------------ rotary knob: value ring + dot, optional sidechain level ring, AUTO toggle in the caption
class ProKnob : public Slider {
public:
    String caption, autoWord; float ring = -1.f; bool big = false, bipolar = false; std::function<void()> onTouch; String paramId;
    ToggleButton autoBtn;
    ProKnob() : Slider(RotaryHorizontalVerticalDrag, NoTextBox) {
        setRotaryParameters(MathConstants<float>::pi * 1.18f, MathConstants<float>::pi * 2.82f, true);
        setMouseDragSensitivity(240); setScrollWheelEnabled(true);
        addChildComponent(autoBtn); autoBtn.setAlpha(0.01f); autoBtn.setMouseCursor(MouseCursor::PointingHandCursor);
        entry.setJustificationType(Justification::centred); entry.setEditable(false, true, false); addChildComponent(entry);
        entry.onEditorHide = [this] { entry.setVisible(false); };
        entry.onTextChange = [this] { setValue(getValueFromText(entry.getText()), sendNotificationSync); };
        onDragStart = [this] { if (onTouch) onTouch(); };
    }
    ~ProKnob() override { att.reset(); autoAtt.reset(); }
    void bind(SA& a, const String& id) {
        att.reset(); att = std::make_unique<SA::SliderAttachment>(a, id, *this); paramId = id;
        if (auto* p = a.getParameter(id)) defaultV = p->convertFrom0to1(p->getDefaultValue());
    }
    void bindAuto(SA& a, const String& id) { autoAtt.reset(); autoAtt = std::make_unique<SA::ButtonAttachment>(a, id, autoBtn); autoBtn.setVisible(true); }
    void resized() override {
        const float k = K(*this), lh = 26.f * k; entry.setBounds(Rectangle<int>(0, 0, 60, 18).withCentre(Point<int>(getWidth() / 2, (int) ((getHeight() - lh) * 0.5f))));
        entry.setFont(F(12.f * k, true));
        if (autoWord.isNotEmpty()) {
            const auto f = F(10.5f * k, true, 0.08f); const float wa = textW(autoWord, f), wc = textW(caption, f), gap = 5.f * k, x0 = ((float) getWidth() - (wa + gap + wc)) * 0.5f;
            autoBtn.setBounds(Rectangle<float>(x0 - 3.f * k, (float) getHeight() - lh + 2.f * k, wa + 6.f * k, lh - 4.f * k).toNearestInt());
        }
    }
    void mouseDoubleClick(const MouseEvent& e) override {
        if (e.mods.isAltDown() || e.mods.isCtrlDown()) { setValue(defaultV, sendNotificationSync); return; }
        entry.setText(getTextFromValue(getValue()), dontSendNotification); entry.setVisible(true); entry.showEditor();
    }
    void mouseEnter(const MouseEvent& e) override { Slider::mouseEnter(e); repaint(); }
    void mouseExit(const MouseEvent& e) override { Slider::mouseExit(e); repaint(); }
    void paint(Graphics& g) override {
        const float k = K(*this), lh = 26.f * k; auto b = getLocalBounds().toFloat(); const float D = jmin(b.getWidth(), b.getHeight() - lh);
        const Point<float> c(b.getCentreX(), D * 0.5f); const float R = D * 0.5f - 3.f * k;
        const auto rp = getRotaryParameters(); const float a0 = rp.startAngleRadians, a1 = rp.endAngleRadians;
        const float pos = (float) valueToProportionOfLength(getValue()), ang = a0 + pos * (a1 - a0), mid = a0 + 0.5f * (a1 - a0);
        const bool hot = isMouseOverOrDragging(); float arcR = R; const float th = (big ? 4.4f : 3.8f) * k;
        if (ring >= 0.f) {   // sidechain input level ring (outer, green)
            Path t; t.addCentredArc(c.x, c.y, R, R, 0.f, a0, a1, true); g.setColour(Colour(0xff2a3225)); g.strokePath(t, PathStrokeType(5.2f * k, PathStrokeType::curved, PathStrokeType::butt));
            if (ring > 0.003f) { Path v; v.addCentredArc(c.x, c.y, R, R, 0.f, a0, a0 + jlimit(0.f, 1.f, ring) * (a1 - a0), true);
                g.setColour(cGreen); g.strokePath(v, PathStrokeType(5.2f * k, PathStrokeType::curved, PathStrokeType::butt)); }
            arcR = R - 9.f * k;
        }
        Path track; track.addCentredArc(c.x, c.y, arcR, arcR, 0.f, a0, a1, true);
        g.setColour(cTrack); g.strokePath(track, PathStrokeType(th * 0.8f, PathStrokeType::curved, PathStrokeType::rounded));
        { Path v; const float s0 = bipolar ? mid : a0; v.addCentredArc(c.x, c.y, arcR, arcR, 0.f, jmin(s0, ang), jmax(s0, ang) + 0.0001f, true);
          g.setColour(cYellow.withAlpha(0.18f)); g.strokePath(v, PathStrokeType(th * 2.2f, PathStrokeType::curved, PathStrokeType::rounded));
          g.setColour(cYellow); g.strokePath(v, PathStrokeType(th, PathStrokeType::curved, PathStrokeType::rounded)); }
        const float bodyR = arcR - th - 3.f * k;
        g.setGradientFill(ColourGradient(Colour(0xff45454c), c.x - bodyR * 0.5f, c.y - bodyR * 0.8f, Colour(0xff0f0f12), c.x + bodyR * 0.4f, c.y + bodyR * 0.9f, false));
        g.fillEllipse(c.x - bodyR, c.y - bodyR, bodyR * 2.f, bodyR * 2.f);
        g.setColour(Colour(hot ? 0xff5a5a63 : 0xff3a3a41)); g.drawEllipse(c.x - bodyR, c.y - bodyR, bodyR * 2.f, bodyR * 2.f, 1.1f * k);
        { const auto d = c.getPointOnCircumference(arcR, ang - MathConstants<float>::halfPi + MathConstants<float>::halfPi); (void) d; }
        const Point<float> dot(c.x + std::sin(ang) * arcR, c.y - std::cos(ang) * arcR);
        g.setColour(Colour(0xff0d0d10)); g.fillEllipse(dot.x - 4.f * k, dot.y - 4.f * k, 8.f * k, 8.f * k);
        g.setColour(cYellow); g.fillEllipse(dot.x - 2.9f * k, dot.y - 2.9f * k, 5.8f * k, 5.8f * k);
        {   // value read-out in the knob centre (always visible, shrinks to fit)
            const String vt = getTextFromValue(getValue()); float fh = std::max(8.f, bodyR * 0.36f);
            while (fh > 6.f && textW(vt, F(fh, true)) > bodyR * 1.75f) fh -= 0.5f;
            g.setColour(isMouseButtonDown() ? cYellow : cText.withAlpha(0.85f)); g.setFont(F(fh, true)); g.drawText(vt, Rectangle<float>(c.x - bodyR, c.y - bodyR * 0.4f, bodyR * 2.f, bodyR * 0.8f), Justification::centred);
        }
        // caption: [AUTO] CAPTION  (AUTO is a toggle)
        const auto f = F(10.5f * k, true, 0.08f); g.setFont(f);
        const float wa = autoWord.isNotEmpty() ? textW(autoWord, f) : 0.f, wc = textW(caption, f), gap = autoWord.isNotEmpty() ? 5.f * k : 0.f, x0 = (b.getWidth() - (wa + gap + wc)) * 0.5f, ty = D + 3.f * k;
        if (autoWord.isNotEmpty()) { g.setColour(autoBtn.getToggleState() ? cYellow : cFaint.brighter(0.15f)); g.drawText(autoWord, Rectangle<float>(x0, ty, wa + 2.f, lh - 4.f * k), Justification::centredLeft); }
        g.setColour(hot ? Colours::white : cText.withAlpha(0.92f)); g.drawText(caption, Rectangle<float>(x0 + wa + gap, ty, wc + 4.f, lh - 4.f * k), Justification::centredLeft);
    }
private:
    Label entry; double defaultV = 0; std::unique_ptr<SA::SliderAttachment> att; std::unique_ptr<SA::ButtonAttachment> autoAtt;
};

// ------------------------------------------------------------------ small wavy-line slider (Knee / Range / Lookahead / Hold)
class CurveSlider : public Slider {
public:
    String caption; std::function<void()> onTouch;
    CurveSlider() : Slider(LinearBar, NoTextBox) { setSliderSnapsToMousePosition(false); setMouseDragSensitivity(180); onDragStart = [this] { if (onTouch) onTouch(); }; }
    ~CurveSlider() override { att.reset(); }
    void bind(SA& a, const String& id) { att.reset(); att = std::make_unique<SA::SliderAttachment>(a, id, *this); if (auto* p = a.getParameter(id)) defaultV = p->convertFrom0to1(p->getDefaultValue()); }
    void mouseDoubleClick(const MouseEvent&) override { setValue(defaultV, sendNotificationSync); }
    void mouseEnter(const MouseEvent& e) override { Slider::mouseEnter(e); repaint(); }
    void mouseExit(const MouseEvent& e) override { Slider::mouseExit(e); repaint(); }
    void paint(Graphics& g) override {
        const float k = K(*this), lh = 17.f * k; auto b = getLocalBounds().toFloat(); const Rectangle<float> tr(b.getX() + 6.f * k, 0.f, b.getWidth() - 12.f * k, b.getHeight() - lh);
        const float cy = tr.getCentreY(), amp = 3.4f * k; const float pos = (float) valueToProportionOfLength(getValue()), xv = tr.getX() + pos * tr.getWidth();
        auto yAt = [&](float x) { return cy + amp * std::sin((x - tr.getX()) / tr.getWidth() * MathConstants<float>::twoPi * 1.5f); };
        Path t, v; for (float x = tr.getX(); x <= tr.getRight() + 0.1f; x += 2.f * k) { if (x == tr.getX()) t.startNewSubPath(x, yAt(x)); else t.lineTo(x, yAt(x)); }
        for (float x = tr.getX(); x <= xv; x += 1.5f * k) { if (x == tr.getX()) v.startNewSubPath(x, yAt(x)); else v.lineTo(x, yAt(x)); }
        g.setColour(cTrack); g.strokePath(t, PathStrokeType(2.2f * k, PathStrokeType::curved, PathStrokeType::rounded));
        if (pos > 0.004f) { g.setColour(cYellow.withAlpha(0.2f)); g.strokePath(v, PathStrokeType(6.f * k, PathStrokeType::curved, PathStrokeType::rounded)); g.setColour(cYellow); g.strokePath(v, PathStrokeType(3.2f * k, PathStrokeType::curved, PathStrokeType::rounded)); }
        g.setColour(Colour(0xff0d0d10)); g.fillEllipse(xv - 4.4f * k, yAt(xv) - 4.4f * k, 8.8f * k, 8.8f * k); g.setColour(cYellow); g.fillEllipse(xv - 3.2f * k, yAt(xv) - 3.2f * k, 6.4f * k, 6.4f * k);
        const bool drag = isMouseButtonDown(); g.setFont(F(10.f * k, true, 0.08f)); g.setColour(drag ? cYellow : (isMouseOver() ? Colours::white : cText.withAlpha(0.9f)));
        g.drawText(drag ? getTextFromValue(getValue()) : caption + "  " + getTextFromValue(getValue()), Rectangle<float>(0.f, b.getBottom() - lh, b.getWidth(), lh), Justification::centred);
    }
private:
    double defaultV = 0; std::unique_ptr<SA::SliderAttachment> att;
};

// ------------------------------------------------------------------ text value you can drag (status bar, EQ band editor); double-click = reset
class DragValue : public Slider {
public:
    String prefix; bool alignLeft = false;
    DragValue() : Slider(LinearBarVertical, NoTextBox) { setMouseDragSensitivity(220); setSliderSnapsToMousePosition(false); setVelocityBasedMode(false); }
    ~DragValue() override { att.reset(); }
    void bind(SA& a, const String& id) { att.reset(); att = std::make_unique<SA::SliderAttachment>(a, id, *this); if (auto* p = a.getParameter(id)) defaultV = p->convertFrom0to1(p->getDefaultValue()); }
    void unbind() { att.reset(); }
    void mouseDoubleClick(const MouseEvent&) override { setValue(defaultV, sendNotificationSync); }
    void mouseEnter(const MouseEvent& e) override { Slider::mouseEnter(e); repaint(); }
    void mouseExit(const MouseEvent& e) override { Slider::mouseExit(e); repaint(); }
    void paint(Graphics& g) override {
        const float k = K(*this); auto b = getLocalBounds().toFloat(); g.setFont(F(std::min(11.5f * k, b.getHeight() * 0.62f)));
        const auto txt = getTextFromValue(getValue()); const bool hot = isMouseOverOrDragging();
        if (prefix.isNotEmpty()) { g.setColour(cFaint.brighter(0.25f)); g.drawText(prefix, b.withTrimmedLeft(4.f * k), Justification::centredLeft); }
        g.setColour(hot ? cYellow : cText.withAlpha(0.9f)); g.drawText(txt, b.withTrimmedRight(prefix.isNotEmpty() ? 4.f * k : 0.f), alignLeft ? Justification::centredLeft : (prefix.isNotEmpty() ? Justification::centredRight : Justification::centred));
    }
private:
    double defaultV = 0; std::unique_ptr<SA::SliderAttachment> att;
};

// ------------------------------------------------------------------ vertical bipolar level slider (side chain level)
class VSlider : public Slider {
public:
    VSlider() : Slider(LinearBarVertical, NoTextBox) { setSliderSnapsToMousePosition(false); setMouseDragSensitivity(160); setDoubleClickReturnValue(true, 0.0); }
    void paint(Graphics& g) override {
        const float k = K(*this); auto b = getLocalBounds().toFloat(); const float cx = b.getCentreX(), top = b.getY() + 4.f * k, bot = b.getBottom() - 4.f * k;
        const float p = (float) valueToProportionOfLength(getValue()), y = bot - p * (bot - top), ym = (top + bot) * 0.5f;
        g.setColour(Colour(0xff0c0c0f)); g.fillRoundedRectangle(cx - 3.f * k, top, 6.f * k, bot - top, 3.f * k);
        g.setColour(cYellow); g.fillRoundedRectangle(cx - 3.f * k, jmin(y, ym), 6.f * k, std::fabs(y - ym) + 1.f, 2.f * k);
        g.setColour(Colour(0xff0d0d10)); g.fillEllipse(cx - 7.f * k, y - 7.f * k, 14.f * k, 14.f * k);
        g.setGradientFill(ColourGradient(Colour(0xffe8e8ee), cx, y - 6.f * k, Colour(0xff9a9aa4), cx, y + 6.f * k, false)); g.fillEllipse(cx - 5.5f * k, y - 5.5f * k, 11.f * k, 11.f * k);
    }
};

// ------------------------------------------------------------------ rounded text button used for style pill, mix pills, small toggles
class PillButton : public Button {
public:
    String text; bool led = false, bold = true; Colour ledCol = Colour(0xffe2452e);
    explicit PillButton(const String& t = {}) : Button(t), text(t) {}
    void paintButton(Graphics& g, bool hl, bool down) override {
        auto r = getLocalBounds().toFloat().reduced(1.f); const float k = K(*this), cr = r.getHeight() * 0.5f * 0.9f; const bool on = getToggleState();
        g.setGradientFill(ColourGradient(Colour(down ? 0xff2a2a30 : 0xff3b3b42), 0.f, r.getY(), Colour(0xff222227), 0.f, r.getBottom(), false)); g.fillRoundedRectangle(r, cr);
        g.setColour(on ? cYellow.withAlpha(0.9f) : Colour(hl ? 0xff5b5b64 : 0xff4a4a52)); g.drawRoundedRectangle(r, cr, 1.1f * k);
        g.setColour(on ? cYellow : cText); g.setFont(F(std::min(12.f * k, r.getHeight() * 0.5f), bold, 0.04f));
        auto tr = r; if (led) tr = tr.withTrimmedRight(r.getHeight() * 0.6f);
        g.drawText(text.isEmpty() ? getButtonText() : text, tr, Justification::centred);
        if (led) { const float d = r.getHeight() * 0.22f; g.setColour(ledCol.withAlpha(0.35f)); g.fillEllipse(r.getRight() - r.getHeight() * 0.5f - d, r.getCentreY() - d, d * 2.f, d * 2.f); g.setColour(ledCol); g.fillEllipse(r.getRight() - r.getHeight() * 0.5f - d * 0.55f, r.getCentreY() - d * 0.55f, d * 1.1f, d * 1.1f); }
    }
};

// ------------------------------------------------------------------ segmented choice (Off / Tube / Diode / Bright) bound to a choice parameter
class Segmented : public Component {
public:
    Segmented(SA& a, const String& id, StringArray names) : apvts(a), pid(id), items(std::move(names)) {}
    void paint(Graphics& g) override {
        const float k = K(*this); auto r = getLocalBounds().toFloat(); const int n = items.size(), cur = (int) ui::getP(apvts, pid);
        g.setColour(Colour(0xff0f0f12)); g.fillRoundedRectangle(r, 5.f * k); g.setColour(Colour(0xff3d3d44)); g.drawRoundedRectangle(r.reduced(0.5f), 5.f * k, 1.f);
        for (int i = 0; i < n; ++i) {
            auto cell = r.withWidth(r.getWidth() / (float) n).withX(r.getX() + r.getWidth() * (float) i / (float) n).reduced(2.f * k);
            if (i == cur) { g.setGradientFill(ColourGradient(Colour(0xff55555d), 0.f, cell.getY(), Colour(0xff3a3a41), 0.f, cell.getBottom(), false)); g.fillRoundedRectangle(cell, 4.f * k); }
            g.setColour(i == cur ? Colours::white : cDim); g.setFont(F(12.f * k, i == cur)); g.drawText(items[i], cell, Justification::centred);
        }
    }
    void mouseDown(const MouseEvent& e) override {
        const int i = jlimit(0, items.size() - 1, (int) (e.position.x / (float) getWidth() * (float) items.size()));
        ui::gesture(apvts, pid, true); ui::setP(apvts, pid, (float) i); ui::gesture(apvts, pid, false); repaint();
    }
private:
    SA& apvts; String pid; StringArray items;
};

// ------------------------------------------------------------------ orange drive bar with value text
class DriveBar : public Slider {
public:
    DriveBar() : Slider(LinearBar, NoTextBox) { setSliderSnapsToMousePosition(true); setMouseDragSensitivity(200); setDoubleClickReturnValue(true, 0.0); }
    void paint(Graphics& g) override {
        const float k = K(*this); auto r = getLocalBounds().toFloat(); g.setColour(Colour(0xff0c0c0f)); g.fillRoundedRectangle(r, 4.f * k);
        const float p = (float) valueToProportionOfLength(getValue()); auto fill = r.withWidth(jmax(r.getWidth() * p, 6.f * k));
        g.setGradientFill(ColourGradient(Colour(0xffe0752b), fill.getX(), 0.f, Colour(0xffa84a1e), fill.getRight(), 0.f, false)); g.fillRoundedRectangle(fill, 4.f * k);
        g.setColour(Colour(0xff4a4a52)); g.drawRoundedRectangle(r.reduced(0.5f), 4.f * k, 1.f);
        g.setColour(Colours::white); g.setFont(F(11.f * k, true)); g.drawText(getTextFromValue(getValue()), r, Justification::centred);
    }
};

// ------------------------------------------------------------------ floating CHARACTER panel (mode, drive, pre/post)
class CharacterPanel : public Component {
public:
    explicit CharacterPanel(SA& a) : seg(a, "character", { "Off", "Tube", "Diode", "Bright" }), apvts(a) {
        addAndMakeVisible(seg); addAndMakeVisible(drive); addAndMakeVisible(post);
        driveA = std::make_unique<SA::SliderAttachment>(a, "drive", drive); 
        post.led = true; post.onClick = [this] { const bool p = ui::getP(apvts, "charPost") > 0.5f; ui::gesture(apvts, "charPost", true); ui::setP(apvts, "charPost", p ? 0.f : 1.f); ui::gesture(apvts, "charPost", false); };
        setInterceptsMouseClicks(true, true);
    }
    void refresh() { post.text = ui::getP(apvts, "charPost") > 0.5f ? "POST" : "PRE"; post.repaint(); drive.setAlpha(ui::getP(apvts, "character") > 0.5f ? 1.f : 0.45f); repaint(); }
    void resized() override {
        const float k = K(*this); auto r = getLocalBounds().reduced(int(10 * k), int(10 * k)); r.removeFromTop(int(16 * k));
        seg.setBounds(r.removeFromTop(int(30 * k))); r.removeFromTop(int(10 * k));
        auto row = r.removeFromTop(int(28 * k)); post.setBounds(row.removeFromRight(int(88 * k))); drive.setBounds(row.withTrimmedRight(int(10 * k)));
    }
    void paint(Graphics& g) override {
        const float k = K(*this); auto r = getLocalBounds().toFloat().reduced(1.f);
        g.setColour(Colours::black.withAlpha(0.35f)); g.fillRoundedRectangle(r.translated(0.f, 3.f * k), 9.f * k);
        g.setGradientFill(ColourGradient(Colour(0xff34343a), 0.f, r.getY(), Colour(0xff202025), 0.f, r.getBottom(), false)); g.fillRoundedRectangle(r, 9.f * k);
        g.setColour(Colour(0xff55555d)); g.drawRoundedRectangle(r, 9.f * k, 1.f); g.setColour(cDim); g.setFont(F(10.5f * k, true, 0.12f)); g.drawText("CHARACTER", r.removeFromTop(24.f * k), Justification::centred);
        auto dr = drive.getBounds().toFloat();
        g.setColour(cDim); g.setFont(F(10.f * k, true, 0.1f)); g.drawText(ui::getP(apvts, "character") > 0.5f ? "DRIVE  (drag or click)" : "DRIVE  (pick Tube / Diode / Bright)", dr.translated(0.f, dr.getHeight() + 3.f * k).withHeight(14.f * k).withWidth(dr.getWidth() + 90.f * k), Justification::centred);
    }
    Segmented seg; DriveBar drive; PillButton post;
private:
    SA& apvts; std::unique_ptr<SA::SliderAttachment> driveA;
};

// ------------------------------------------------------------------ level display: input level history, red gain-reduction trace, transfer curve with live point
class ProDisplay : public Component {
public:
    explicit ProDisplay(RajvikProcessor& p) : proc(p) { lastRead = proc.meters.w.load(); }
    float curIn = -100, curOut = -100, curGr = 0;
    static constexpr float kTop = 0.f, kBot = -36.f;   // level axis shared with the meter scale
    void pull() {
        const unsigned w = proc.meters.w.load(std::memory_order_acquire); unsigned from = lastRead; if (w - from > MeterRing::N) from = w - MeterRing::N;
        for (unsigned i = from; i != w; ++i) { const auto k = i % MeterRing::N; hist.push_back({ rdp::linToDb(proc.meters.in[k].load()), rdp::linToDb(proc.meters.out[k].load()), proc.meters.gr[k].load() }); }
        lastRead = w; while (hist.size() > 480) hist.erase(hist.begin());
        if (!hist.empty()) { curIn = hist.back().in; curOut = hist.back().out; curGr = hist.back().gr; }
        if (isVisible()) repaint();
    }
    float yOf(float db) const { const float H = (float) getHeight(); return H * 0.045f + (jlimit(kBot, kTop, db) - kTop) / (kBot - kTop) * H * 0.91f; }
    void paint(Graphics& g) override {
        const float k = K(*this), W = (float) getWidth(), H = (float) getHeight();
        g.setGradientFill(ColourGradient(cTop, 0.f, 0.f, cBot, 0.f, H, false)); g.fillAll();
        for (int db = 0; db >= -36; db -= 6) { g.setColour(Colour(0x14ffffff)); g.fillRect(0.f, yOf((float) db), W, 1.f); }
        const int n = (int) hist.size(); const float dx = W / 480.f, x0 = W - (float) n * dx;
        if (n > 1) {   // input level (grey, jagged)
            Path line, fill; fill.startNewSubPath(x0, H);
            for (int i = 0; i < n; ++i) { const float x = x0 + (float) i * dx, y = yOf(hist[(size_t) i].in + 3.f); if (i == 0) line.startNewSubPath(x, y); else line.lineTo(x, y); fill.lineTo(x, y); }
            fill.lineTo(W, H); fill.closeSubPath();
            g.setGradientFill(ColourGradient(Colour(0x40b8b8c4), 0.f, yOf(-6.f), Colour(0x04b8b8c4), 0.f, H, false)); g.fillPath(fill);
            g.setColour(Colour(0xffa8a8b3).withAlpha(0.85f)); g.strokePath(line, PathStrokeType(1.1f * k));
            // red gain reduction trace hanging from the top
            Path gr, grFill; const float gy0 = H * 0.03f; grFill.startNewSubPath(x0, gy0);
            for (int i = 0; i < n; ++i) { const float x = x0 + (float) i * dx, y = gy0 + jmin(std::fabs(hist[(size_t) i].gr), 24.f) / 24.f * H * 0.30f; if (i == 0) gr.startNewSubPath(x, y); else gr.lineTo(x, y); grFill.lineTo(x, y); }
            grFill.lineTo(W, gy0); grFill.closeSubPath(); g.setColour(cRed.withAlpha(0.10f)); g.fillPath(grFill);
            g.setColour(cRed.withAlpha(0.35f)); g.strokePath(gr, PathStrokeType(4.f * k)); g.setColour(cRed); g.strokePath(gr, PathStrokeType(1.4f * k));
        }
        { auto& a = proc.apvts; const float thr = ui::getP(a, "autoThr") > 0.5f ? proc.effThr.load() : ui::getP(a, "threshold");
          const float dash[] = { 5.f * k, 5.f * k }; g.setColour(Colour(0x66ffffff)); g.drawDashedLine(Line<float>(0.f, yOf(thr), W, yOf(thr)), dash, 2, 1.f * k); }
        g.setColour(Colour(0x55000000)); g.fillRect(0.f, H - 1.f, W, 1.f);
    }
private:
    struct Frame { float in, out, gr; };
    RajvikProcessor& proc; std::vector<Frame> hist; unsigned lastRead = 0;
};


// ------------------------------------------------------------------ left panel: threshold / knee transfer-curve view (drag the line = threshold, drag the knee handle = knee)
class KneePanel : public Component {
public:
    explicit KneePanel(RajvikProcessor& p) : proc(p) {}
    float curIn = -100.f, curOut = -100.f;
    Rectangle<float> plot() const { const float k = K(*this); return Rectangle<float>(30.f * k, 52.f * k, (float) getWidth() - 44.f * k, (float) getWidth() - 44.f * k); }
    float thrNow() const { auto& a = proc.apvts; return ui::getP(a, "autoThr") > 0.5f ? proc.effThr.load() : ui::getP(a, "threshold"); }
    float kneeNow() const { auto& a = proc.apvts; return jlimit(0.f, 72.f, ui::getP(a, "knee") + rdp::kModes[(int) ui::getP(a, "mode")].kneeAdd); }
    float X(float db) const { auto r = plot(); return r.getX() + (jlimit(-60.f, 0.f, db) + 60.f) / 60.f * r.getWidth(); }
    float Y(float db) const { auto r = plot(); return r.getBottom() - (jlimit(-60.f, 0.f, db) + 60.f) / 60.f * r.getHeight(); }
    void paint(Graphics& g) override {
        const float k = K(*this), W = (float) getWidth(), H = (float) getHeight(); auto r = plot();
        g.setGradientFill(ColourGradient(Colour(0xff25252a), 0.f, 0.f, Colour(0xff17171a), 0.f, H, false)); g.fillAll();
        g.setColour(cLine); g.fillRect(W - 1.f, 0.f, 1.f, H);
        g.setColour(cDim); g.setFont(F(10.f * k, true, 0.1f)); g.drawText("THRESHOLD / KNEE", Rectangle<float>(0.f, 10.f * k, W, 14.f * k), Justification::centred);
        g.setColour(Colour(0xff0d0d10)); g.fillRoundedRectangle(r, 3.f * k);
        g.setFont(F(8.5f * k)); 
        for (int db = -60; db <= 0; db += 12) { g.setColour(Colour(0x18ffffff)); g.fillRect(r.getX(), Y((float) db), r.getWidth(), 1.f); g.fillRect(X((float) db), r.getY(), 1.f, r.getHeight());
            g.setColour(cDim); g.drawText(String(db), Rectangle<float>(0.f, Y((float) db) - 6.f * k, 27.f * k, 12.f * k), Justification::centredRight); }
        auto& a = proc.apvts; const auto& m = rdp::kModes[(int) ui::getP(a, "mode")];
        const float thr = thrNow(), kn = kneeNow(), ratio = ui::getP(a, "ratio"), rng = ui::getP(a, "range");
        // knee zone (around the threshold, on both axes)
        g.setColour(cYellow.withAlpha(0.10f)); g.fillRect(Rectangle<float>(r.getX(), Y(thr + kn * 0.5f), r.getWidth(), jmax(2.f, Y(thr - kn * 0.5f) - Y(thr + kn * 0.5f))).getIntersection(r));
        g.setColour(Colour(0x30ffffff)); g.drawLine(r.getX(), r.getBottom(), r.getRight(), r.getY(), 1.f * k);   // 1:1 reference
        const float dash[] = { 4.f * k, 4.f * k }; g.setColour(cYellow.withAlpha(0.8f)); g.drawDashedLine(Line<float>(r.getX(), Y(thr), r.getRight(), Y(thr)), dash, 2, 1.f * k);
        g.drawDashedLine(Line<float>(X(thr), r.getY(), X(thr), r.getBottom()), dash, 2, 1.f * k);
        Path tc; for (int i = 0; i <= 240; ++i) { const float db = -60.f + (float) i * 0.25f, o = db + rdp::staticGainDb(m.kind, db, thr, ratio, kn, rng); const float x = X(db), y = Y(o); if (i == 0) tc.startNewSubPath(x, y); else tc.lineTo(x, y); }
        g.saveState(); g.reduceClipRegion(r.toNearestInt()); g.setColour(cCurve.withAlpha(0.2f)); g.strokePath(tc, PathStrokeType(5.f * k)); g.setColour(cCurve); g.strokePath(tc, PathStrokeType(1.8f * k));
        if (curIn > -59.f) { const float px = X(curIn), py = Y(curOut); g.setColour(Colours::white.withAlpha(0.25f)); g.fillEllipse(px - 6.f * k, py - 6.f * k, 12.f * k, 12.f * k); g.setColour(Colours::white); g.fillEllipse(px - 2.5f * k, py - 2.5f * k, 5.f * k, 5.f * k); }
        g.restoreState();
        // handles
        g.setColour(cYellow); g.fillEllipse(r.getRight() - 6.f * k, Y(thr) - 4.f * k, 8.f * k, 8.f * k);
        g.setColour(Colours::white); g.fillRoundedRectangle(Rectangle<float>(7.f * k, 7.f * k).withCentre({ r.getX() + 8.f * k, Y(thr + kn * 0.5f) }), 1.5f * k);
        g.setColour(cDim); g.setFont(F(8.5f * k)); g.drawText("IN", Rectangle<float>(r.getX(), r.getBottom() + 2.f * k, r.getWidth(), 11.f * k), Justification::centred);
        // read-outs
        const float ry = r.getBottom() + 20.f * k; g.setFont(F(9.5f * k, true, 0.08f));
        g.setColour(cDim); g.drawText("THRESHOLD", Rectangle<float>(8.f * k, ry, W - 16.f * k, 12.f * k), Justification::centredLeft);
        g.setColour(cYellow); g.setFont(F(14.f * k, true)); g.drawText(String(thr, 1) + " dB", Rectangle<float>(8.f * k, ry + 12.f * k, W - 16.f * k, 18.f * k), Justification::centredLeft);
        g.setColour(cDim); g.setFont(F(9.5f * k, true, 0.08f)); g.drawText("KNEE", Rectangle<float>(8.f * k, ry + 36.f * k, W - 16.f * k, 12.f * k), Justification::centredLeft);
        g.setColour(cText); g.setFont(F(14.f * k, true)); g.drawText(String(ui::getP(a, "knee"), 1) + " dB", Rectangle<float>(8.f * k, ry + 48.f * k, W - 16.f * k, 18.f * k), Justification::centredLeft);
    }
    void mouseDown(const MouseEvent& e) override {
        auto r = plot(); const float k = K(*this); mode = 0;
        if (std::fabs(e.position.x - (r.getX() + 8.f * k)) < 10.f * k && std::fabs(e.position.y - Y(thrNow() + kneeNow() * 0.5f)) < 10.f * k) mode = 2; else if (r.expanded(8.f * k).contains(e.position)) mode = 1;
        if (mode) ui::gesture(proc.apvts, mode == 1 ? "threshold" : "knee", true); mouseDrag(e);
    }
    void mouseDrag(const MouseEvent& e) override {
        auto r = plot(); const float db = jlimit(-60.f, 0.f, -60.f + (r.getBottom() - e.position.y) / r.getHeight() * 60.f);
        if (mode == 1) { if (ui::getP(proc.apvts, "autoThr") < 0.5f) ui::setP(proc.apvts, "threshold", jlimit(-60.f, 0.f, db)); }
        else if (mode == 2) ui::setP(proc.apvts, "knee", jlimit(0.f, 72.f, 2.f * (db - thrNow())));
    }
    void mouseUp(const MouseEvent&) override { if (mode) ui::gesture(proc.apvts, mode == 1 ? "threshold" : "knee", false); mode = 0; }
    void mouseWheelMove(const MouseEvent&, const MouseWheelDetails& w) override { ui::gesture(proc.apvts, "knee", true); ui::setP(proc.apvts, "knee", jlimit(0.f, 72.f, ui::getP(proc.apvts, "knee") + w.deltaY * 12.f)); ui::gesture(proc.apvts, "knee", false); }
private:
    RajvikProcessor& proc; int mode = 0;
};

// ------------------------------------------------------------------ right-hand level meter: scale, GR bar, input / output bars, peak read-outs
class ProMeter : public Component {
public:
    float in = -100, out = -100, gr = 0, inHold = -100, outHold = -100, grMax = 0;
    float mom = -100, sho = -100, integ = -100, tp = -100, rmsIn = -100, rmsOut = -100; int outMode = 0; std::function<void()> onReset;
    void update(float i, float o, float g) {
        in = std::max(i, in - 1.6f); out = std::max(o, out - 1.6f); gr = (std::fabs(g) > std::fabs(gr) ? g : gr * 0.9f + g * 0.1f);
        inHold = std::max(i, inHold - 0.1f); outHold = std::max(o, outHold - 0.1f); if (g < grMax) grMax = g; repaint();
    }
    void mouseDown(const MouseEvent& e) override {
        if (e.position.y < 22.f * K(*this) && e.position.x > (float) getWidth() * 0.5f) { outMode = (outMode + 1) % 6; repaint(); return; }
        inHold = outHold = -100; grMax = 0; if (onReset) onReset();
    }
    String outText() const {
        auto f = [](float v) { return v <= -99.f ? String("--") : String(v, 1); };
        switch (outMode) { case 1: return f(tp); case 2: return f(rmsOut); case 3: return f(mom); case 4: return f(sho); case 5: return f(integ); default: return f(outHold); }
    }
    void paint(Graphics& g) override {
        const float k = K(*this), W = (float) getWidth(), H = (float) getHeight();
        g.setGradientFill(ColourGradient(Colour(0xff25252a), 0.f, 0.f, Colour(0xff101013), 0.f, H, false)); g.fillAll(); g.setColour(Colour(0xff0a0a0c)); g.fillRect(0.f, 0.f, 1.f, H);
        const float top = 28.f * k, bot = H - 8.f * k; auto Y = [&](float db) { return top + (jlimit(-36.f, 0.f, db)) / -36.f * (bot - top); };
        g.setFont(F(9.5f * k)); g.setColour(cDim);
        for (int db = 0; db >= -33; db -= 3) g.drawText(String(db), Rectangle<float>(0.f, Y((float) db) - 6.f * k, 25.f * k, 12.f * k), Justification::centredRight);
        auto bar = [&](float x, float w, float level, float hold) {
            auto b = Rectangle<float>(x, top, w, bot - top); g.setColour(Colour(0xff0b0b0d)); g.fillRoundedRectangle(b, 1.5f * k);
            const float t = Y(level); if (t < b.getBottom()) { ColourGradient cg(Colour(0xffd6b72c), 0.f, Y(0.f), Colour(0xff4fa83a), 0.f, Y(-30.f), false); cg.addColour(0.0, cRed); cg.addColour(0.16, Colour(0xffd6b72c)); g.setGradientFill(cg); g.fillRoundedRectangle(b.withTop(t), 1.5f * k); }
            g.setColour(Colours::white.withAlpha(0.9f)); g.fillRect(b.getX(), Y(hold), b.getWidth(), 1.3f * k);
        };
        { auto b = Rectangle<float>(29.f * k, top, 6.f * k, bot - top); g.setColour(Colour(0xff0b0b0d)); g.fillRoundedRectangle(b, 1.5f * k);   // GR (hangs from the top)
          const float h = jmin(std::fabs(gr), 36.f) / 36.f * b.getHeight(); g.setColour(cRed); g.fillRoundedRectangle(b.withHeight(h), 1.5f * k); }
        bar(38.f * k, 8.f * k, in, inHold); bar(48.f * k, 8.f * k, out, outHold);
        g.setFont(F(10.f * k, true)); auto rd = [&](Rectangle<float> r, const String& t, bool hot) { g.setColour(hot ? cRed : cGreen); g.drawText(t, r, Justification::centred); };
        rd(Rectangle<float>(0.f, 6.f * k, 30.f * k, 14.f * k), inHold <= -99.f ? "--" : String(inHold, 1), inHold > 0.f);
        rd(Rectangle<float>(30.f * k, 6.f * k, 30.f * k, 14.f * k), outText(), outMode == 0 && outHold > 0.f);
        if (outMode > 0) { static const char* t[] = { "", "TP", "RMS", "M", "S", "I" }; g.setColour(cYellow); g.setFont(F(8.f * k, true)); g.drawText(t[outMode], Rectangle<float>(W - 24.f * k, 18.f * k, 22.f * k, 9.f * k), Justification::centred); }
    }
};

// ------------------------------------------------------------------ preset browser (search + categories as tags)
class PresetBrowser : public Component, private ListBoxModel {
public:
    PresetBrowser(RajvikProcessor& p, std::function<void(const String&)> picked, std::function<void()> saveCb) : proc(p), onPicked(std::move(picked)), onSave(std::move(saveCb)) {
        int i = 0; for (auto& f : proc.presets.factory()) all.push_back({ f.name, f.category, i++ });
        auto u = proc.presets.userPresets(); for (int j = 0; j < u.size(); ++j) all.push_back({ u[j].getFileNameWithoutExtension(), "User", 1000 + j });
        search.setTextToShowWhenEmpty("Search presets or tags...", cFaint); search.onTextChange = [this] { filter(); }; search.setFont(F(14.f));
        addAndMakeVisible(search); list.setModel(this); list.setRowHeight(26); list.setColour(ListBox::backgroundColourId, Colours::transparentBlack); addAndMakeVisible(list);
        saveBtn.setButtonText("Save current..."); saveBtn.onClick = [this] { if (onSave) onSave(); }; addAndMakeVisible(saveBtn); filter(); setSize(380, 440);
    }
    void resized() override { auto r = getLocalBounds().reduced(10); search.setBounds(r.removeFromTop(28)); r.removeFromTop(8); saveBtn.setBounds(r.removeFromBottom(28)); r.removeFromBottom(6); list.setBounds(r); }
    void paint(Graphics& g) override { g.fillAll(Colour(0xff1e1e22)); }
    int getNumRows() override { return (int) shown.size(); }
    void paintListBoxItem(int row, Graphics& g, int w, int h, bool sel) override {
        if (row < 0 || row >= (int) shown.size()) return; auto& it = shown[(size_t) row];
        if (sel) { g.setColour(cYellow.withAlpha(0.18f)); g.fillRect(0, 0, w, h); }
        g.setColour(cText); g.setFont(F(14.f)); g.drawText(it.name, 10, 0, w - 140, h, Justification::centredLeft);
        g.setColour(cDim); g.setFont(F(11.f)); g.drawText(it.cat, w - 150, 0, 140, h, Justification::centredRight);
    }
    void listBoxItemClicked(int row, const MouseEvent&) override {
        if (row < 0 || row >= (int) shown.size()) return; auto& it = shown[(size_t) row];
        if (it.id < 1000) proc.presets.loadFactory(it.id); else { auto u = proc.presets.userPresets(); if (it.id - 1000 < u.size()) proc.presets.loadUser(u[it.id - 1000]); }
        if (onPicked) onPicked(it.name);
    }
private:
    struct Item { String name, cat; int id; };
    void filter() { shown.clear(); const auto q = search.getText().trim().toLowerCase(); for (auto& i : all) if (q.isEmpty() || i.name.toLowerCase().contains(q) || i.cat.toLowerCase().contains(q)) shown.push_back(i); list.updateContent(); list.repaint(); }
    RajvikProcessor& proc; std::function<void(const String&)> onPicked; std::function<void()> onSave; std::vector<Item> all, shown;
    TextEditor search; ListBox list; TextButton saveBtn;
};

} // namespace pro
