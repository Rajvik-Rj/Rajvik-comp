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
    String caption, autoWord; float ring = -1.f; bool big = false, bipolar = false, mini = false; std::function<void()> onTouch; String paramId;
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
        if (mini) { paintMini(g); return; }
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
    void paintMini(Graphics& g) {   // small ring knob, label + value to the right
        const float k = K(*this); auto b = getLocalBounds().toFloat(); const float r = jmin(b.getHeight() * 0.5f - 4.f * k, 20.f * k); const Point<float> c(r + 6.f * k, b.getCentreY());
        const auto rp = getRotaryParameters(); const float a0 = rp.startAngleRadians, a1 = rp.endAngleRadians, mid = a0 + 0.5f * (a1 - a0);
        const float pos = (float) valueToProportionOfLength(getValue()), ang = a0 + pos * (a1 - a0);
        g.setColour(Colour(0xff14141d)); g.fillEllipse(c.x - r, c.y - r, r * 2.f, r * 2.f);
        Path t; t.addCentredArc(c.x, c.y, r, r, 0.f, a0, a1, true); g.setColour(Colour(0xff2a2a38)); g.strokePath(t, PathStrokeType(3.f * k));
        { Path v; const float s0 = bipolar ? mid : a0; v.addCentredArc(c.x, c.y, r, r, 0.f, jmin(s0, ang), jmax(s0, ang) + 0.0001f, true); g.setColour(Colour(0xfff5c542)); g.strokePath(v, PathStrokeType(3.4f * k)); }
        if (ring >= 0.f) { Path v; v.addCentredArc(c.x, c.y, r + 5.f * k, r + 5.f * k, 0.f, a0, a0 + jlimit(0.f, 1.f, ring) * (a1 - a0), true); g.setColour(cGreen.withAlpha(0.85f)); g.strokePath(v, PathStrokeType(1.6f * k)); }
        const Point<float> p0(c.x + std::sin(ang) * r * 0.25f, c.y - std::cos(ang) * r * 0.25f), p1(c.x + std::sin(ang) * (r - 6.f * k), c.y - std::cos(ang) * (r - 6.f * k));
        g.setColour(Colours::white); g.drawLine(Line<float>(p0, p1), 2.f * k);
        const float tx = c.x + r + 12.f * k;
        g.setColour(Colour(0xff8c8c9c)); g.setFont(F(9.f * k, true, 0.12f)); g.drawText(caption, Rectangle<float>(tx, c.y - 18.f * k, b.getWidth() - tx, 13.f * k), Justification::centredLeft);
        g.setColour(isMouseOverOrDragging() ? Colour(0xfff5c542) : Colour(0xffe9e9f0)); g.setFont(F(14.5f * k, true)); g.drawText(getTextFromValue(getValue()), Rectangle<float>(tx, c.y - 4.f * k, b.getWidth() - tx, 20.f * k), Justification::centredLeft);
    }
private:
    Label entry; double defaultV = 0; std::unique_ptr<SA::SliderAttachment> att; std::unique_ptr<SA::ButtonAttachment> autoAtt;
};

// ------------------------------------------------------------------ flat slider (Knee / Range / Lookahead / Hold / Dry gain; bipolar Mid-Side balance)
class CurveSlider : public Slider {
public:
    String caption, endL = "MID", endR = "SIDE"; bool bal = false; Colour col{ 0xfff5c542 }; std::function<void()> onTouch;
    CurveSlider() : Slider(LinearBar, NoTextBox) { setSliderSnapsToMousePosition(false); setMouseDragSensitivity(180); onDragStart = [this] { if (onTouch) onTouch(); }; }
    ~CurveSlider() override { att.reset(); }
    void bind(SA& a, const String& id) { att.reset(); att = std::make_unique<SA::SliderAttachment>(a, id, *this); if (auto* p = a.getParameter(id)) defaultV = p->convertFrom0to1(p->getDefaultValue()); }
    void mouseDoubleClick(const MouseEvent&) override { setValue(defaultV, sendNotificationSync); }
    void mouseEnter(const MouseEvent& e) override { Slider::mouseEnter(e); repaint(); }
    void mouseExit(const MouseEvent& e) override { Slider::mouseExit(e); repaint(); }
    String getTextFromValue(double v) override {
        if (!bal) return Slider::getTextFromValue(v);
        if (std::fabs(v) < 0.5) return "0.0 dB";
        const auto& e = v < 0 ? endL : endR; return e.substring(0, 1) + e.substring(1).toLowerCase() + " +" + String(std::fabs(v) * 0.06, 1) + " dB";
    }
    void paint(Graphics& g) override {
        const float k = K(*this); auto b = getLocalBounds().toFloat(); const float ty = b.getHeight() * 0.62f, h = 3.4f * k;
        g.setColour(Colour(0xff8c8c9c)); g.setFont(F(9.f * k, true, 0.12f)); g.drawText(caption, Rectangle<float>(0.f, 0.f, b.getWidth() * 0.62f, 14.f * k), Justification::centredLeft);
        g.setColour(isMouseOverOrDragging() ? col : Colour(0xffe9e9f0)); g.setFont(F(12.f * k, true)); g.drawText(getTextFromValue(getValue()), Rectangle<float>(b.getWidth() * 0.3f, 0.f, b.getWidth() * 0.7f, 14.f * k), Justification::centredRight);
        const float x0 = 6.f * k, w = b.getWidth() - 12.f * k, pos = (float) valueToProportionOfLength(getValue()), px = x0 + pos * w;
        g.setColour(Colour(0xff2a2a38)); g.fillRoundedRectangle(x0, ty - h * 0.5f, w, h, h * 0.5f);
        if (bal) { const float cx = x0 + w * 0.5f; g.setColour(col); g.fillRoundedRectangle(jmin(cx, px), ty - h * 0.5f, std::fabs(px - cx), h, h * 0.5f); g.setColour(Colour(0xff55555d)); g.fillRect(cx - 0.7f * k, ty - 6.f * k, 1.4f * k, 12.f * k);
            g.setColour(Colour(0xff5a5a6a)); g.setFont(F(8.f * k, true, 0.1f)); g.drawText(endL, Rectangle<float>(0.f, ty + 8.f * k, 60.f * k, 11.f * k), Justification::centredLeft); g.drawText(endR, Rectangle<float>(b.getWidth() - 60.f * k, ty + 8.f * k, 60.f * k, 11.f * k), Justification::centredRight); }
        else { g.setColour(col); g.fillRoundedRectangle(x0, ty - h * 0.5f, jmax(h, pos * w), h, h * 0.5f); }
        g.setColour(Colour(0xff07070c)); g.fillEllipse(px - 7.2f * k, ty - 7.2f * k, 14.4f * k, 14.4f * k); g.setColour(col); g.fillEllipse(px - 5.2f * k, ty - 5.2f * k, 10.4f * k, 10.4f * k);
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

// ------------------------------------------------------------------ flat rounded button (header, status bar, toggles); gold when on
class BoxButton : public Button {
public:
    String text; bool caret = false;
    explicit BoxButton(const String& t = {}) : Button(t), text(t) {}
    void paintButton(Graphics& g, bool hl, bool down) override {
        const float k = K(*this); auto r = getLocalBounds().toFloat(); const bool on = getToggleState();
        g.setColour(on ? Colour(0xfff5c542) : Colour(down ? 0xff2a2a38 : (hl ? 0xff262634 : 0xff1d1d29))); g.fillRoundedRectangle(r, 6.f * k);
        const auto c = on ? Colour(0xff17140a) : Colour(0xffd2d3de); g.setColour(c); g.setFont(F(std::min(11.5f * k, r.getHeight() * 0.5f), on)); 
        g.drawText(text.isEmpty() ? getButtonText() : text, caret ? r.withTrimmedRight(12.f * k) : r, Justification::centred);
        if (caret) { Path p; const float cx = r.getRight() - 12.f * k, cy = r.getCentreY(); p.addTriangle(cx - 3.4f * k, cy - 1.7f * k, cx + 3.4f * k, cy - 1.7f * k, cx, cy + 2.6f * k); g.fillPath(p); }
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

// ------------------------------------------------------------------ main view: dB-scaled waveform with threshold / knee drawn on it, plus the knee-curve panel on the left
class ProDisplay : public Component {
public:
    explicit ProDisplay(RajvikProcessor& p) : proc(p) { lastRead = proc.meters.w.load(); }
    float curIn = -100, curOut = -100, curGr = 0; bool showCurve = true;
    void pull() {
        const unsigned w = proc.meters.w.load(std::memory_order_acquire); unsigned from = lastRead; if (w - from > MeterRing::N) from = w - MeterRing::N;
        for (unsigned i = from; i != w; ++i) { const auto k = i % MeterRing::N; hist.push_back({ rdp::linToDb(proc.meters.in[k].load()), rdp::linToDb(proc.meters.out[k].load()), proc.meters.gr[k].load() }); }
        lastRead = w; while (hist.size() > 480) hist.erase(hist.begin());
        if (!hist.empty()) { curIn = hist.back().in; curOut = hist.back().out; curGr = hist.back().gr; }
        if (isVisible()) repaint();
    }
    static float rr(float db) { return jlimit(0.f, 1.f, (db + 48.f) / 48.f); }
    void paint(Graphics& g) override {
        const float k = K(*this), W = (float) getWidth(), H = (float) getHeight();
        g.fillAll(Colour(0xff06060b));
        auto& a = proc.apvts; const auto& m = rdp::kModes[(int) ui::getP(a, "mode")];
        const float thr = ui::getP(a, "autoThr") > 0.5f ? proc.effThr.load() : ui::getP(a, "threshold"), ratio = ui::getP(a, "ratio"), rng = ui::getP(a, "range"), knee = jlimit(0.f, 72.f, ui::getP(a, "knee") + m.kneeAdd);
        const float cw = showCurve ? 214.f * k : 0.f, x0 = cw, ww = W - cw, hh = (H - 70.f * k) * 0.5f, yc = 60.f * k + hh, rt = rr(thr) * hh;
        const Colour gold(0xfff5c542);
        // grid
        g.setColour(Colour(0xff12121a)); for (int i = 0; i <= 12; ++i) g.fillRect(x0 + ww * (float) i / 12.f, 0.f, 1.f, H);
        g.setColour(Colour(0xff14141d)); for (float db : { 0.f, -12.f, -24.f, -36.f }) for (int s = -1; s <= 1; s += 2) g.fillRect(x0, yc + (float) s * rr(db) * hh, ww, 1.f);
        g.setColour(Colour(0xff1d1d29)); g.fillRect(x0, yc, ww, 1.f);
        // knee zone around the threshold (mirrored above / below the centre line)
        for (int s = -1; s <= 1; s += 2) { const float yA = yc + (float) s * rr(thr + knee * 0.5f) * hh, yB = yc + (float) s * rr(thr - knee * 0.5f) * hh; g.setColour(gold.withAlpha(0.10f)); g.fillRect(x0, jmin(yA, yB), ww, std::fabs(yA - yB)); }
        // waveform: grey = below threshold (untouched), gold = part above the threshold, white line = output
        const int n = (int) hist.size(); const float dx = ww / 480.f, xs = W - (float) n * dx; Path top, bot, grp, grf; const float gy0 = 8.f * k;
        grf.startNewSubPath(xs, 6.f * k);
        for (int i = 0; i < n; ++i) {
            const float x = xs + (float) i * dx, ri = rr(hist[(size_t) i].in) * hh, ro = rr(hist[(size_t) i].out) * hh, bw = dx + 0.7f, rl = jmin(ri, rt);
            g.setColour(Colour(0xff4a4a5e)); g.fillRect(x, yc - rl, bw, 2.f * rl);
            if (ri > rt) { g.setColour(gold); g.fillRect(x, yc - ri, bw, ri - rt); g.fillRect(x, yc + rt, bw, ri - rt); }
            if (i == 0) { top.startNewSubPath(x, yc - ro); bot.startNewSubPath(x, yc + ro); } else { top.lineTo(x, yc - ro); bot.lineTo(x, yc + ro); }
            const float gy = gy0 + jmin(16.f, std::fabs(hist[(size_t) i].gr)) * 2.4f * k; if (i == 0) grp.startNewSubPath(x, gy); else grp.lineTo(x, gy); grf.lineTo(x, gy);
        }
        g.setColour(Colours::white.withAlpha(0.95f)); g.strokePath(top, PathStrokeType(1.5f * k)); g.strokePath(bot, PathStrokeType(1.5f * k));
        // gain reduction strip
        g.setColour(Colour(0xff0b0b12).withAlpha(0.85f)); g.fillRect(x0, 4.f * k, ww, 44.f * k); g.setColour(Colour(0xff1d1d29)); g.fillRect(x0, 48.f * k, ww, 1.f);
        if (n > 1) { grf.lineTo(W, 6.f * k); grf.closeSubPath(); g.setColour(gold.withAlpha(0.16f)); g.fillPath(grf); g.setColour(gold); g.strokePath(grp, PathStrokeType(1.6f * k)); }
        g.setColour(Colour(0xff5a5a6a)); g.setFont(F(8.5f * k, true, 0.12f)); g.drawText("GAIN REDUCTION", Rectangle<float>(x0 + 8.f * k, 6.f * k, 120.f * k, 12.f * k), Justification::centredLeft);
        // threshold lines + labels
        { const float d[] = { 7.f * k, 5.f * k }; g.setColour(gold.withAlpha(0.9f)); for (int s = -1; s <= 1; s += 2) g.drawDashedLine(Line<float>(x0, yc + (float) s * rt, W, yc + (float) s * rt), d, 2, 1.4f * k); }
        { auto chip = Rectangle<float>(W - 186.f * k, yc - rt - 26.f * k, 178.f * k, 20.f * k); g.setColour(Colour(0xff0f0f17)); g.fillRoundedRectangle(chip, 5.f * k); g.setColour(gold); g.drawRoundedRectangle(chip, 5.f * k, 1.f);
          g.setFont(F(10.f * k, true, 0.06f)); g.drawText("THRESHOLD  " + String(thr, 1) + " dB", chip, Justification::centred); }
        g.setColour(Colour(0xff8c8c9c)); g.setFont(F(9.f * k)); g.drawText("above the line the compressor reacts  (" + String(ratio, 1) + " : 1)", Rectangle<float>(W - 330.f * k, yc - rr(-6.f) * hh - 6.f * k, 322.f * k, 14.f * k), Justification::centredRight);
        if (knee > 0.9f) { g.setColour(gold); g.setFont(F(8.5f * k, true, 0.1f)); g.drawText("KNEE ZONE  " + String(knee, 1) + " dB", Rectangle<float>(x0 + 54.f * k, yc - rr(thr - knee * 0.5f) * hh + 3.f * k, 150.f * k, 12.f * k), Justification::centredLeft); }
        for (float db : { 0.f, -12.f, -24.f, -36.f }) { const float y = yc - rr(db) * hh; g.setColour(Colour(0xd906060b)); g.fillRoundedRectangle(x0 + 8.f * k, y - 8.f * k, 28.f * k, 16.f * k, 4.f * k); g.setColour(Colour(0xff8c8c9c)); g.setFont(F(9.5f * k, true)); g.drawText(String((int) db), Rectangle<float>(x0 + 8.f * k, y - 8.f * k, 28.f * k, 16.f * k), Justification::centred); }
        { float lx = x0 + 44.f * k; const float ly = H - 14.f * k; const struct { Colour c; const char* t; } lg[] = { { Colour(0xff4a4a5e), "below threshold (untouched)" }, { gold, "removed by compressor" }, { Colours::white, "output" } };
          g.setFont(F(9.f * k)); for (auto& e : lg) { g.setColour(e.c); g.fillRoundedRectangle(lx, ly - 8.f * k, 10.f * k, 10.f * k, 2.f * k); g.setColour(Colour(0xff8c8c9c)); g.drawText(e.t, Rectangle<float>(lx + 15.f * k, ly - 9.f * k, 180.f * k, 12.f * k), Justification::centredLeft); lx += 15.f * k + textW(e.t, F(9.f * k)) + 26.f * k; } }
        if (!showCurve) return;
        // ---- knee / threshold panel (left) ----
        g.setColour(Colour(0xff1d1d29)); g.fillRect(cw - 1.f, 0.f, 1.f, H);
        const float cx0 = 10.f * k, cy0 = 14.f * k, cs = 194.f * k; Rectangle<float> pr(cx0, cy0, cs, cs);
        g.setColour(Colour(0xff0c0c13)); g.fillRoundedRectangle(pr, 10.f * k); g.setColour(Colour(0xff2a2a38)); g.drawRoundedRectangle(pr, 10.f * k, 1.f);
        auto XY = [&](float i, float o) { return Point<float>(cx0 + 14.f * k + (i + 60.f) / 60.f * (cs - 28.f * k), cy0 + cs - 14.f * k - (o + 60.f) / 60.f * (cs - 28.f * k)); };
        g.setColour(Colour(0xff1c1c28)); for (float v : { -48.f, -36.f, -24.f, -12.f, 0.f }) { const auto p = XY(v, v); g.fillRect(p.x, cy0 + 14.f * k, 1.f, cs - 28.f * k); g.fillRect(cx0 + 14.f * k, p.y, cs - 28.f * k, 1.f); }
        { const auto p0 = XY(-60.f, -60.f), p1 = XY(0.f, 0.f); const float d[] = { 4.f * k, 4.f * k }; g.setColour(Colour(0xff55556a)); g.drawDashedLine(Line<float>(p0, p1), d, 2, 1.f); }
        Path cv, sh; for (int i = -60; i <= 0; ++i) { const float o = (float) i + rdp::staticGainDb(m.kind, (float) i, thr, ratio, knee, rng); const auto p = XY((float) i, o); if (i == -60) { cv.startNewSubPath(p); sh.startNewSubPath(p); } else { cv.lineTo(p); sh.lineTo(p); } }
        sh.lineTo(XY(0.f, 0.f)); sh.closeSubPath(); g.setColour(gold.withAlpha(0.12f)); g.fillPath(sh);
        { const auto ka = XY(thr - knee * 0.5f, 0.f), kb = XY(thr + knee * 0.5f, 0.f); g.setColour(gold.withAlpha(0.10f)); g.fillRect(ka.x, cy0 + 14.f * k, jmax(2.f, kb.x - ka.x), cs - 28.f * k); }
        g.setColour(gold.withAlpha(0.14f)); g.strokePath(cv, PathStrokeType(6.f * k)); g.setColour(gold); g.strokePath(cv, PathStrokeType(2.4f * k));
        { const float li = jlimit(-60.f, 0.f, curIn); const auto p = XY(li, li + rdp::staticGainDb(m.kind, li, thr, ratio, knee, rng)); if (curIn > -59.f) { g.setColour(gold.withAlpha(0.25f)); g.fillEllipse(p.x - 11.f * k, p.y - 11.f * k, 22.f * k, 22.f * k); g.setColour(Colours::white); g.fillEllipse(p.x - 5.f * k, p.y - 5.f * k, 10.f * k, 10.f * k); } }
        g.setColour(Colour(0xff8c8c9c)); g.setFont(F(8.5f * k, true, 0.12f)); g.drawText("KNEE / CURVE", Rectangle<float>(cx0 + 14.f * k, cy0 + 4.f * k, 120.f * k, 12.f * k), Justification::centredLeft);
        g.setColour(Colour(0xff5a5a6a)); g.drawText("IN", Rectangle<float>(cx0, cy0 + cs - 14.f * k, cs, 12.f * k), Justification::centred); g.drawText("OUT", Rectangle<float>(cx0 + 2.f * k, cy0 + cs * 0.5f - 6.f * k, 26.f * k, 12.f * k), Justification::centredLeft);
        const struct { const char* l; String v; } rows[] = { { "THRESHOLD", String(thr, 1) + " dB" }, { "RATIO", String(ratio, 1) + " : 1" }, { "KNEE", String(knee, 1) + " dB" }, { "RANGE", String(rng, 0) + " dB" }, { "GAIN REDUCTION", String(curGr, 1) + " dB" } };
        float ry = 226.f * k; for (auto& r : rows) { g.setColour(Colour(0xff8c8c9c)); g.setFont(F(9.f * k, true, 0.1f)); g.drawText(r.l, Rectangle<float>(16.f * k, ry, 110.f * k, 20.f * k), Justification::centredLeft);
            g.setColour(r.l[0] == 'G' ? gold : Colour(0xffe9e9f0)); g.setFont(F(12.5f * k, true)); g.drawText(r.v, Rectangle<float>(100.f * k, ry, 98.f * k, 20.f * k), Justification::centredRight); ry += 29.f * k; }
    }
private:
    struct Frame { float in, out, gr; };
    RajvikProcessor& proc; std::vector<Frame> hist; unsigned lastRead = 0;
};

// ------------------------------------------------------------------ right-hand meter: GR chip + scale, input / output level bars
class ProMeter : public Component {
public:
    float in = -100, out = -100, gr = 0, inHold = -100, outHold = -100, grMax = 0;
    float mom = -100, sho = -100, integ = -100, tp = -100, rmsIn = -100, rmsOut = -100; int outMode = 0; std::function<void()> onReset;
    void update(float i, float o, float g) {
        in = std::max(i, in - 1.6f); out = std::max(o, out - 1.6f); gr = (std::fabs(g) > std::fabs(gr) ? g : gr * 0.9f + g * 0.1f);
        inHold = std::max(i, inHold - 0.1f); outHold = std::max(o, outHold - 0.1f); if (g < grMax) grMax = g; repaint();
    }
    void mouseDown(const MouseEvent& e) override {
        if (e.position.y > (float) getHeight() - 26.f * K(*this)) { outMode = (outMode + 1) % 6; repaint(); return; }
        inHold = outHold = -100; grMax = 0; if (onReset) onReset();
    }
    String outText() const {
        auto f = [](float v) { return v <= -99.f ? String("--") : String(v, 1); };
        switch (outMode) { case 1: return "TP " + f(tp); case 2: return "RMS " + f(rmsOut); case 3: return "M " + f(mom); case 4: return "S " + f(sho); case 5: return "I " + f(integ); default: return f(outHold); }
    }
    void paint(Graphics& g) override {
        const float k = K(*this), W = (float) getWidth(), H = (float) getHeight(); const Colour gold(0xfff5c542);
        g.fillAll(Colour(0xff08080d));
        auto chip = Rectangle<float>(8.f * k, 8.f * k, 96.f * k, 22.f * k); g.setColour(Colour(0xff101018)); g.fillRoundedRectangle(chip, 5.f * k); g.setColour(gold); g.setFont(F(10.5f * k, true)); g.drawText("GR " + String(gr, 1) + " dB", chip, Justification::centred);
        const float top = 44.f * k, bot = H - 30.f * k; auto Ygr = [&](float db) { return top + jlimit(0.f, 24.f, -db) / 24.f * (bot - top); }; auto Ylv = [&](float db) { return top + (jlimit(-80.f, 0.f, db)) / -80.f * (bot - top); };
        g.setFont(F(10.f * k, true)); g.setColour(gold); for (int d = 0; d >= -24; d -= 6) g.drawText(String(d), Rectangle<float>(0.f, Ygr((float) d) - 7.f * k, 40.f * k, 14.f * k), Justification::centredRight);
        { Rectangle<float> t(46.f * k, top, 20.f * k, bot - top); g.setColour(Colour(0xff12121a)); g.fillRoundedRectangle(t, 4.f * k);   // gain reduction bar (hangs from the top)
          for (int d = -3; d > -24; d -= 3) { g.setColour(Colour(0xff23232f)); g.fillRect(t.getX(), Ygr((float) d), t.getWidth(), 1.f); }
          const float y = Ygr(gr); ColourGradient cg(gold, 0.f, top, Colour(0xffff5d5d), 0.f, Ygr(-24.f), false); g.setGradientFill(cg); g.fillRoundedRectangle(t.withBottom(jmax(top + 1.f, y)), 4.f * k);
          g.setColour(Colours::white); g.fillRect(t.getX(), Ygr(grMax), t.getWidth(), 1.6f * k);
          g.setColour(Colour(0xff8c8c9c)); g.setFont(F(8.5f * k, true, 0.1f)); g.drawText("GR", Rectangle<float>(t.getX() - 6.f * k, bot + 6.f * k, t.getWidth() + 12.f * k, 14.f * k), Justification::centred); }
        g.setColour(Colour(0xff5a5a6a)); g.setFont(F(10.5f * k)); for (int d : { -10, -30, -50, -70 }) g.drawText(String(d), Rectangle<float>(100.f * k, Ylv((float) d) - 7.f * k, 54.f * k, 14.f * k), Justification::centredRight);
        auto bar = [&](float x, float level, float hold) {
            Rectangle<float> t(x, top, 10.f * k, bot - top); g.setColour(Colour(0xff12121a)); g.fillRoundedRectangle(t, 5.f * k);
            ColourGradient cg(Colour(0xffff5d5d), 0.f, Ylv(0.f), Colour(0xff5ee08a), 0.f, Ylv(-60.f), false); cg.addColour(0.35, gold); g.setGradientFill(cg);
            const float y = Ylv(level); if (y < t.getBottom()) g.fillRoundedRectangle(t.withTop(y), 5.f * k);
            g.setColour(Colours::white.withAlpha(0.9f)); g.fillRect(t.getX(), Ylv(hold), t.getWidth(), 1.3f * k); };
        bar(166.f * k, in, inHold); bar(184.f * k, out, outHold);
        g.setFont(F(9.5f * k, true)); g.setColour(inHold > 0.f ? Colour(0xffff5d5d) : cGreen); g.drawText(inHold <= -99.f ? "--" : String(inHold, 1), Rectangle<float>(150.f * k, bot + 6.f * k, 40.f * k, 16.f * k), Justification::centred);
        g.setColour(outMode == 0 && outHold > 0.f ? Colour(0xffff5d5d) : gold); g.drawText(outText(), Rectangle<float>(188.f * k, bot + 6.f * k, 60.f * k, 16.f * k), Justification::centredLeft);
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
