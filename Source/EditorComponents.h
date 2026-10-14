#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

// All layout is designed on a 1536 x 1024 reference canvas (the design image) and scaled with  k = width / 1536.
namespace ui {
inline const juce::Colour bg{ 0xff090c13 }, bgTop{ 0xff0e121c }, panel{ 0xff0c111b }, panelEdge{ 0xff1b2335 }, field{ 0xff0e1420 }, fieldEdge{ 0xff28324a },
                          text{ 0xffe8ebf2 }, dim{ 0xff8b93a4 }, faint{ 0xff4a5366 }, accent{ 0xffffc21a }, orange{ 0xffff8a1f }, blue{ 0xff2f81ff },
                          green{ 0xff3fd35a }, red{ 0xffff4545 }, grid{ 0xff161d2b };
inline const juce::Colour eqCol[6] = { juce::Colour(0xff8a93a3), juce::Colour(0xffffc21a), juce::Colour(0xff3fd35a), juce::Colour(0xff2f8cff), juce::Colour(0xffb04dff), juce::Colour(0xffff6a2a) };
inline const juce::Colour bandCol[4] = { juce::Colour(0xffff6a2a), juce::Colour(0xffffc21a), juce::Colour(0xff3fd35a), juce::Colour(0xff2f8cff) };

inline juce::Font font(float h, bool bold = false, float kern = 0.f) {
    auto o = juce::FontOptions(h, bold ? juce::Font::bold : juce::Font::plain);
    if (kern > 0.f) o = o.withKerningFactor(kern);
    return juce::Font(o);
}
inline float scaleOf(const juce::Component& c) {
    if (auto* t = c.getTopLevelComponent()) return std::max(0.2f, std::min((float) t->getWidth() / 1536.f, (float) t->getHeight() / 1024.f));
    return 0.75f; }
// font that is shrunk (never grown) so that `t` fits into maxW pixels - nothing is ever clipped, whatever the system font is
inline juce::Font fitFont(const juce::String& t, float h, float maxW, bool bold = false, float kern = 0.f) {
    auto f = font(h, bold, kern);
    if (t.isEmpty() || maxW <= 0.f) return f;
    juce::GlyphArrangement ga; ga.addLineOfText(f, t, 0.f, 0.f);
    const float w = ga.getBoundingBox(0, -1, true).getWidth();
    if (w > maxW) f = font(std::max(6.f, h * maxW / w), bold, kern);
    return f; }
inline float getP(juce::AudioProcessorValueTreeState& a, const juce::String& id) { return a.getRawParameterValue(id)->load(); }
inline void setP(juce::AudioProcessorValueTreeState& a, const juce::String& id, float plain) {
    if (auto* p = a.getParameter(id)) p->setValueNotifyingHost(p->convertTo0to1(plain)); }
inline void gesture(juce::AudioProcessorValueTreeState& a, const juce::String& id, bool begin) {
    if (auto* p = a.getParameter(id)) { if (begin) p->beginChangeGesture(); else p->endChangeGesture(); } }
inline void fillCombo(juce::ComboBox& cb, juce::AudioProcessorValueTreeState& a, const juce::String& id) {
    if (auto* cp = dynamic_cast<juce::AudioParameterChoice*>(a.getParameter(id))) cb.addItemList(cp->choices, 1); }

// ------------------------------------------------------------------ icons (vector, no assets)
inline void drawIcon(juce::Graphics& g, const juce::String& n, juce::Rectangle<float> r, juce::Colour c, float lw = 1.8f)
{
    const float side = std::min(r.getWidth(), r.getHeight());
    auto sq = r.withSizeKeepingCentre(side, side); const float x = sq.getX(), y = sq.getY(), w = side;
    auto P = [&](float u, float v) { return juce::Point<float>(x + u * w, y + v * w); };
    juce::Path p; g.setColour(c);
    auto stroke = [&] { g.strokePath(p, juce::PathStrokeType(lw, juce::PathStrokeType::curved, juce::PathStrokeType::rounded)); };
    if (n == "undo" || n == "redo") {
        const bool re = n == "redo"; auto X = [&](float u) { return re ? 1.f - u : u; };
        p.startNewSubPath(P(X(0.34f), 0.22f)); p.lineTo(P(X(0.16f), 0.42f)); p.lineTo(P(X(0.36f), 0.60f));
        p.startNewSubPath(P(X(0.17f), 0.42f)); p.cubicTo(P(X(0.45f), 0.20f), P(X(0.88f), 0.30f), P(X(0.80f), 0.74f)); stroke();
    } else if (n == "prev" || n == "next") {
        const bool nx = n == "next"; auto X = [&](float u) { return nx ? 1.f - u : u; };
        p.startNewSubPath(P(X(0.62f), 0.2f)); p.lineTo(P(X(0.36f), 0.5f)); p.lineTo(P(X(0.62f), 0.8f)); stroke();
    } else if (n == "chev" || n == "chevUp") {
        const float a = n == "chev" ? 0.38f : 0.62f, b = n == "chev" ? 0.64f : 0.36f;
        p.startNewSubPath(P(0.22f, a)); p.lineTo(P(0.5f, b)); p.lineTo(P(0.78f, a)); stroke();
    } else if (n == "fullscreen") {
        p.startNewSubPath(P(.14f, .38f)); p.lineTo(P(.14f, .14f)); p.lineTo(P(.38f, .14f));
        p.startNewSubPath(P(.62f, .14f)); p.lineTo(P(.86f, .14f)); p.lineTo(P(.86f, .38f));
        p.startNewSubPath(P(.86f, .62f)); p.lineTo(P(.86f, .86f)); p.lineTo(P(.62f, .86f));
        p.startNewSubPath(P(.38f, .86f)); p.lineTo(P(.14f, .86f)); p.lineTo(P(.14f, .62f)); stroke();
    } else if (n == "power") {
        p.addCentredArc(x + w * .5f, y + w * .54f, w * .30f, w * .30f, 0.f, 0.62f, juce::MathConstants<float>::twoPi - 0.62f, true);
        p.startNewSubPath(P(.5f, .12f)); p.lineTo(P(.5f, .50f)); stroke();
    } else if (n == "gear") {
        p.addEllipse(x + w * .30f, y + w * .30f, w * .40f, w * .40f);
        for (int i = 0; i < 8; ++i) { const float a = (float) i * juce::MathConstants<float>::pi * 0.25f, s = std::sin(a), co = std::cos(a);
            p.startNewSubPath(P(.5f + .30f * s, .5f - .30f * co)); p.lineTo(P(.5f + .42f * s, .5f - .42f * co)); }
        p.addEllipse(x + w * .22f, y + w * .22f, w * .56f, w * .56f); stroke();
    } else if (n == "eye") {
        p.startNewSubPath(P(.08f, .5f)); p.quadraticTo(P(.5f, .08f), P(.92f, .5f)); p.quadraticTo(P(.5f, .92f), P(.08f, .5f));
        p.addEllipse(x + w * .38f, y + w * .38f, w * .24f, w * .24f); stroke();
    } else if (n == "clock") {
        p.addEllipse(x + w * .12f, y + w * .12f, w * .76f, w * .76f);
        p.startNewSubPath(P(.5f, .28f)); p.lineTo(P(.5f, .5f)); p.lineTo(P(.66f, .6f)); stroke();
    } else if (n == "speaker") {
        p.startNewSubPath(P(.12f, .4f)); p.lineTo(P(.28f, .4f)); p.lineTo(P(.5f, .2f)); p.lineTo(P(.5f, .8f)); p.lineTo(P(.28f, .6f)); p.lineTo(P(.12f, .6f)); p.closeSubPath();
        p.startNewSubPath(P(.63f, .38f)); p.quadraticTo(P(.76f, .5f), P(.63f, .62f));
        p.startNewSubPath(P(.72f, .27f)); p.quadraticTo(P(.97f, .5f), P(.72f, .73f)); stroke();
    } else if (n == "upload") {
        p.startNewSubPath(P(.5f, .64f)); p.lineTo(P(.5f, .16f));
        p.startNewSubPath(P(.3f, .34f)); p.lineTo(P(.5f, .14f)); p.lineTo(P(.7f, .34f));
        p.startNewSubPath(P(.2f, .6f)); p.lineTo(P(.2f, .84f)); p.lineTo(P(.8f, .84f)); p.lineTo(P(.8f, .6f)); stroke();
    } else if (n == "link") {
        p.addRoundedRectangle(x + w * .06f, y + w * .34f, w * .52f, w * .32f, w * .16f);
        p.addRoundedRectangle(x + w * .42f, y + w * .34f, w * .52f, w * .32f, w * .16f); stroke();
    } else if (n == "tBell") {
        p.startNewSubPath(P(.08f, .72f)); p.cubicTo(P(.36f, .72f), P(.34f, .24f), P(.5f, .24f)); p.cubicTo(P(.66f, .24f), P(.64f, .72f), P(.92f, .72f)); stroke();
    } else if (n == "tShelf") {
        p.startNewSubPath(P(.08f, .30f)); p.lineTo(P(.36f, .30f)); p.cubicTo(P(.54f, .30f), P(.46f, .72f), P(.64f, .72f)); p.lineTo(P(.92f, .72f)); stroke();
    } else if (n == "tCut") {
        p.startNewSubPath(P(.08f, .78f)); p.lineTo(P(.34f, .78f)); p.cubicTo(P(.52f, .78f), P(.46f, .26f), P(.64f, .26f)); p.lineTo(P(.92f, .26f)); stroke();
    } else if (n == "logo") {   // wide, uses the full rect
        static const float pts[][2] = { {0,.5f},{.07f,.5f},{.12f,.18f},{.2f,.86f},{.28f,.08f},{.36f,.94f},{.44f,.28f},{.52f,.7f},{.6f,.42f},{.68f,.58f},{.76f,.46f},{.84f,.54f},{1,.5f} };
        for (int i = 0; i < 13; ++i) { auto q = juce::Point<float>(r.getX() + pts[i][0] * r.getWidth(), r.getY() + pts[i][1] * r.getHeight()); if (i == 0) p.startNewSubPath(q); else p.lineTo(q); }
        g.strokePath(p, juce::PathStrokeType(lw, juce::PathStrokeType::mitered, juce::PathStrokeType::butt));
    }
}

// ------------------------------------------------------------------ small button types
class IconButton : public juce::Button {
public:
    explicit IconButton(const juce::String& iconName, float lineWidth = 1.8f) : juce::Button(iconName), icon(iconName), lw(lineWidth) {}
    void paintButton(juce::Graphics& g, bool hl, bool down) override {
        auto r = getLocalBounds().toFloat();
        const bool tog = getClickingTogglesState(), on = getToggleState();
        if (tog && on && icon.startsWith("t")) {   // tool buttons: highlighted cell
            g.setColour(accent.withAlpha(0.14f)); g.fillRoundedRectangle(r.reduced(1.f), 5.f); g.setColour(accent); g.drawRoundedRectangle(r.reduced(1.f), 5.f, 1.2f);
        }
        const auto col = tog ? (on ? accent : dim) : (down ? accent : (hl ? ui::text : dim.brighter(0.25f)));
        drawIcon(g, icon, r.reduced(r.getHeight() * 0.18f), col, lw * std::max(1.f, r.getHeight() / 34.f));
    }
    juce::String icon; float lw;
};

class TextLinkButton : public juce::Button {
public:
    explicit TextLinkButton(const juce::String& t) : juce::Button(t) { setButtonText(t); }
    int underline = -1;   // A/B: 0 = A, 1 = B
    void paintButton(juce::Graphics& g, bool hl, bool) override {
        auto r = getLocalBounds().toFloat(); g.setFont(font(r.getHeight() * 0.42f));
        g.setColour(hl ? ui::text : ui::text.withAlpha(0.88f));
        if (underline < 0) { g.drawText(getButtonText(), r, juce::Justification::centred); return; }
        const float cw = r.getWidth() / 3.f; juce::Rectangle<float> parts[3] = { r.withWidth(cw), r.withX(r.getX() + cw).withWidth(cw), r.withX(r.getX() + 2 * cw).withWidth(cw) };
        g.drawText("A", parts[0], juce::Justification::centred); g.drawText("/", parts[1], juce::Justification::centred); g.drawText("B", parts[2], juce::Justification::centred);
        auto u = parts[underline * 2]; g.setColour(accent);
        g.fillRoundedRectangle(u.getCentreX() - cw * 0.55f, r.getBottom() - r.getHeight() * 0.12f, cw * 1.1f, std::max(2.f, r.getHeight() * 0.05f), 1.f);
    }
};

} // namespace ui

// bottom status bar cell: icon + live text
class StatusButton : public juce::Button {
public:
    StatusButton(const juce::String& iconName, const juce::String& t) : juce::Button(iconName), icon(iconName), label(t) {}
    void setInfo(const juce::String& t) { if (t != label) { label = t; repaint(); } }
    void paintButton(juce::Graphics& g, bool hl, bool) override {
        auto r = getLocalBounds().toFloat(); const float h = r.getHeight();
        ui::drawIcon(g, icon, r.removeFromLeft(h * 0.8f).reduced(h * 0.18f), ui::dim, 1.5f);
        auto tr = r.withTrimmedLeft(h * 0.1f); g.setFont(ui::fitFont(label, h * 0.40f, tr.getWidth() - 2.f)); g.setColour(hl ? ui::text : ui::text.withAlpha(0.85f));
        g.drawText(label, tr, juce::Justification::centredLeft);
    }
    juce::String icon, label;
};

// ------------------------------------------------------------------ look and feel
struct RajvikLnF : juce::LookAndFeel_V4 {
    RajvikLnF() {
        setColour(juce::ComboBox::textColourId, ui::text); setColour(juce::ComboBox::backgroundColourId, ui::field);
        setColour(juce::ComboBox::outlineColourId, ui::fieldEdge); setColour(juce::ComboBox::arrowColourId, ui::dim);
        setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff111827)); setColour(juce::PopupMenu::textColourId, ui::text);
        setColour(juce::PopupMenu::highlightedBackgroundColourId, ui::accent.withAlpha(0.22f)); setColour(juce::PopupMenu::highlightedTextColourId, ui::accent);
        setColour(juce::PopupMenu::headerTextColourId, ui::dim);
        setColour(juce::Label::textColourId, ui::text); setColour(juce::Label::outlineWhenEditingColourId, ui::accent);
        setColour(juce::TextEditor::backgroundColourId, ui::field); setColour(juce::TextEditor::textColourId, ui::text);
        setColour(juce::TextEditor::outlineColourId, ui::fieldEdge); setColour(juce::TextEditor::focusedOutlineColourId, ui::accent);
        setColour(juce::Slider::textBoxTextColourId, ui::text); setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        setColour(juce::AlertWindow::backgroundColourId, juce::Colour(0xff111827)); setColour(juce::AlertWindow::textColourId, ui::text);
        setColour(juce::AlertWindow::outlineColourId, ui::fieldEdge);
    }
    void drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider&) override {
        const auto b = juce::Rectangle<float>((float) x, (float) y, (float) w, (float) h);
        const float R = std::min(b.getWidth(), b.getHeight()) * 0.5f * 0.84f; const auto c = b.getCentre(); const float ang = a0 + pos * (a1 - a0);   // 16 % margin keeps the glow inside the slider bounds
        const float ringR = R * 0.90f, th = std::max(2.5f, R * 0.095f);
        juce::Path track; track.addCentredArc(c.x, c.y, ringR, ringR, 0.f, a0, a1, true);
        g.setColour(juce::Colour(0xff1a2130)); g.strokePath(track, juce::PathStrokeType(th, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        if (pos > 0.002f) {
            juce::Path v; v.addCentredArc(c.x, c.y, ringR, ringR, 0.f, a0, ang, true);
            g.setColour(ui::accent.withAlpha(0.20f)); g.strokePath(v, juce::PathStrokeType(th * 2.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.setGradientFill(juce::ColourGradient(ui::accent, c.x, c.y - ringR, ui::orange, c.x, c.y + ringR, false));
            g.strokePath(v, juce::PathStrokeType(th, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        const float bodyR = R * 0.74f;
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xff2a3040), c.x - bodyR * 0.4f, c.y - bodyR * 0.6f, juce::Colour(0xff090c12), c.x + bodyR * 0.5f, c.y + bodyR * 0.8f, false));
        g.fillEllipse(c.x - bodyR, c.y - bodyR, bodyR * 2, bodyR * 2);
        g.setColour(juce::Colour(0xff323b50)); g.drawEllipse(c.x - bodyR, c.y - bodyR, bodyR * 2, bodyR * 2, std::max(1.f, R * 0.02f));
        const float t = std::max(2.f, R * 0.075f);
        juce::Path ptr; ptr.addRoundedRectangle(-t * 0.5f, -bodyR * 0.88f, t, bodyR * 0.40f, t * 0.5f);
        g.setColour(ui::accent); g.fillPath(ptr, juce::AffineTransform::rotation(ang).translated(c.x, c.y));
    }
    void drawLinearSlider(juce::Graphics& g, int x, int y, int w, int h, float pos, float, float, juce::Slider::SliderStyle, juce::Slider&) override {
        const float cy = (float) y + (float) h * 0.5f, th = std::max(3.f, (float) h * 0.22f);
        g.setColour(juce::Colour(0xff1a2130)); g.fillRoundedRectangle((float) x, cy - th * 0.5f, (float) w, th, th * 0.5f);
        g.setColour(ui::accent); g.fillRoundedRectangle((float) x, cy - th * 0.5f, std::max(0.f, pos - (float) x), th, th * 0.5f);
        const float d = th * 2.4f; g.setColour(ui::text); g.fillEllipse(pos - d * 0.5f, cy - d * 0.5f, d, d);
    }
    void drawComboBox(juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box) override {
        auto r = juce::Rectangle<float>(0.f, 0.f, (float) w, (float) h).reduced(0.5f); const float cr = (float) h * 0.22f;
        g.setColour(ui::field); g.fillRoundedRectangle(r, cr);
        g.setColour(box.isPopupActive() ? ui::accent.withAlpha(0.7f) : ui::fieldEdge); g.drawRoundedRectangle(r, cr, 1.f);
        ui::drawIcon(g, "chev", juce::Rectangle<float>((float) w - (float) h * 0.85f, 0.f, (float) h * 0.7f, (float) h), ui::text.withAlpha(0.85f), 1.6f);
    }
    // slider text boxes scale with their height and squeeze instead of clipping
    juce::Label* createSliderTextBox(juce::Slider& sl) override { auto* l = juce::LookAndFeel_V4::createSliderTextBox(sl); l->setMinimumHorizontalScale(0.5f); l->setBorderSize(juce::BorderSize<int>(0, 1, 0, 1)); return l; }
    juce::Font getLabelFont(juce::Label& l) override {
        if (dynamic_cast<juce::Slider*>(l.getParentComponent()) != nullptr) return ui::font(std::max(9.f, (float) l.getHeight() * 0.66f));
        return juce::LookAndFeel_V4::getLabelFont(l); }
    juce::Font getComboBoxFont(juce::ComboBox& b) override { return ui::font((float) b.getHeight() * 0.50f); }
    void positionComboBoxText(juce::ComboBox& box, juce::Label& label) override {
        label.setBounds(juce::roundToInt(box.getHeight() * 0.35f), 1, box.getWidth() - juce::roundToInt(box.getHeight() * 1.2f), box.getHeight() - 2);
        label.setFont(getComboBoxFont(box));
    }
    void drawButtonBackground(juce::Graphics& g, juce::Button& b, const juce::Colour&, bool hl, bool down) override {
        auto r = b.getLocalBounds().toFloat().reduced(0.5f); const float cr = r.getHeight() * 0.22f; const bool on = b.getToggleState();
        const juce::Colour tc = b.getProperties().contains("tc") ? juce::Colour((juce::uint32) (int) b.getProperties()["tc"]) : ui::accent;
        g.setColour(on ? tc.withAlpha(0.12f) : (down ? ui::fieldEdge : (hl ? juce::Colour(0xff141b2a) : ui::field))); g.fillRoundedRectangle(r, cr);
        g.setColour(on ? tc : ui::fieldEdge); g.drawRoundedRectangle(r, cr, on ? 1.4f : 1.f);
    }
    void drawButtonText(juce::Graphics& g, juce::TextButton& b, bool, bool) override {
        const juce::Colour tc = b.getProperties().contains("tc") ? juce::Colour((juce::uint32) (int) b.getProperties()["tc"]) : ui::accent;
        g.setFont(ui::fitFont(b.getButtonText(), (float) b.getHeight() * 0.46f, (float) b.getWidth() - 6.f)); g.setColour(b.getToggleState() ? tc : ui::text.withAlpha(0.9f));
        g.drawText(b.getButtonText(), b.getLocalBounds(), juce::Justification::centred);
    }
    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& b, bool hl, bool) override {
        auto r = b.getLocalBounds().toFloat(); const bool on = b.getToggleState(); const float h = r.getHeight();
        const auto style = b.getProperties()["style"].toString();
        if (style == "dot") {
            const float d = h * 0.44f; auto dot = juce::Rectangle<float>(r.getX() + 3.f, r.getCentreY() - d * 0.5f, d, d);
            if (on) { g.setColour(ui::accent.withAlpha(0.28f)); g.fillEllipse(dot.expanded(d * 0.4f)); g.setColour(ui::accent); g.fillEllipse(dot); }
            else { g.setColour(ui::accent.withAlpha(0.45f)); g.drawEllipse(dot, 1.4f); }
            auto tr = r.withTrimmedLeft(d + 12.f); g.setFont(ui::fitFont(b.getButtonText(), h * 0.58f, tr.getWidth() - 2.f)); g.setColour(on || hl ? ui::text : ui::dim);
            g.drawText(b.getButtonText(), tr, juce::Justification::centredLeft); return;
        }
        const float tw = h * 1.45f, th = h * 0.74f; auto track = juce::Rectangle<float>(r.getX() + 2.f, r.getCentreY() - th * 0.5f, tw, th);
        if (on) { g.setColour(ui::accent.withAlpha(0.22f)); g.fillRoundedRectangle(track.expanded(2.f), th * 0.5f + 2.f); }
        g.setColour(on ? ui::accent : juce::Colour(0xff252d3f)); g.fillRoundedRectangle(track, th * 0.5f);
        const float d = th - 5.f; g.setColour(on ? juce::Colour(0xffffefb0) : ui::dim);
        g.fillEllipse(on ? track.getRight() - d - 2.5f : track.getX() + 2.5f, track.getCentreY() - d * 0.5f, d, d);
        auto tr = r.withTrimmedLeft(tw + h * 0.45f); g.setFont(ui::fitFont(b.getButtonText(), h * (style == "header" ? 0.62f : 0.56f), tr.getWidth() - 2.f)); g.setColour(ui::text);
        g.drawText(b.getButtonText(), tr, juce::Justification::centredLeft);
    }
};

// ------------------------------------------------------------------ knob with caption / range labels / value read-out
struct KnobGeom {
    float cx = 0, cy = 0, r = 30;
    juce::String cap; float capX = 0, capY = 0, capSize = 15, capW = 140; bool capLeft = false;
    float valX = 0, valY = 0, valW = 90, valH = 24, valSize = 16; bool valBox = false;
    juce::String lo, hi; float loX = 0, loY = 0, hiX = 0, hiY = 0;
    bool valueOnDrag = false;   // show the value in place of the caption while dragging (used when there is no value box)
};

class KnobBox : public juce::Component {
public:
    using SA = juce::AudioProcessorValueTreeState;
    juce::Slider s{ juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox };
    juce::Label value; std::function<void()> onTouch; juce::String paramId;
    KnobBox() {
        s.setRotaryParameters(juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
        s.setMouseDragSensitivity(200); s.setDoubleClickReturnValue(true, 0.0);
        value.setJustificationType(juce::Justification::centred); value.setEditable(false, true, false);
        value.setMinimumHorizontalScale(0.55f); value.setBorderSize(juce::BorderSize<int>(0, 1, 0, 1));
        value.setColour(juce::Label::textColourId, ui::text);
        value.onTextChange = [this] { s.setValue(s.getValueFromText(value.getText()), juce::sendNotificationSync); refresh(); };
        s.onValueChange = [this] { refresh(); };
        s.onDragStart = [this] { dragging = true; if (onTouch) onTouch(); repaint(); };
        s.onDragEnd = [this] { dragging = false; repaint(); };
        addAndMakeVisible(s); addAndMakeVisible(value); setInterceptsMouseClicks(false, true);
    }
    ~KnobBox() override { att.reset(); }
    void bind(SA& a, const juce::String& id) {
        att.reset(); att = std::make_unique<SA::SliderAttachment>(a, id, s); paramId = id;
        if (auto* p = a.getParameter(id)) s.setDoubleClickReturnValue(true, p->convertFrom0to1(p->getDefaultValue()));   // double-click = default
        refresh(); }
    void refresh() { value.setText(s.getTextFromValue(s.getValue()), juce::dontSendNotification); }
    // originImg: top-left of this box in design coordinates; scale: design px -> screen px
    void setGeom(const KnobGeom& gm, juce::Point<float> originImg, float scale) { geom = gm; origin = originImg; sc = scale; resized(); repaint(); }
    void resized() override {
        const float k = sc;
        auto L = [&](float x, float y) { return juce::Point<float>((x - origin.x) * k, (y - origin.y) * k); };
        const float pad = geom.r * (1.f / 0.84f - 1.f);   // slider is a little larger than the knob so the glow ring is never clipped
        s.setBounds(juce::Rectangle<float>(L(geom.cx - geom.r - pad, geom.cy - geom.r - pad), L(geom.cx + geom.r + pad, geom.cy + geom.r + pad)).toNearestInt());
        value.setVisible(geom.valW > 0.f);
        value.setBounds(juce::Rectangle<float>(L(geom.valX - geom.valW * 0.5f, geom.valY - geom.valH * 0.5f), L(geom.valX + geom.valW * 0.5f, geom.valY + geom.valH * 0.5f)).toNearestInt());
        value.setFont(ui::font(geom.valSize * k));
    }
    void paint(juce::Graphics& g) override {
        const float k = sc;
        auto L = [&](float x, float y) { return juce::Point<float>((x - origin.x) * k, (y - origin.y) * k); };
        if (geom.cap.isNotEmpty()) {
            const juce::String t = (geom.valueOnDrag && dragging) ? s.getTextFromValue(s.getValue()) : geom.cap;
            const float w = geom.capW * k, hh = geom.capSize * k * 1.6f;
            g.setFont(ui::fitFont(t, geom.capSize * k, w)); g.setColour(geom.valueOnDrag && dragging ? ui::accent : ui::text.withAlpha(0.9f));
            auto p = L(geom.capX, geom.capY);
            g.drawText(t, juce::Rectangle<float>(geom.capLeft ? p.x : p.x - w * 0.5f, p.y - hh * 0.5f, w, hh), geom.capLeft ? juce::Justification::centredLeft : juce::Justification::centred);
        }
        g.setColour(ui::dim);
        const float lw = 46.f * k;
        if (geom.lo.isNotEmpty()) { auto p = L(geom.loX, geom.loY); g.setFont(ui::fitFont(geom.lo, 11.5f * k, lw)); g.drawText(geom.lo, juce::Rectangle<float>(p.x - lw * 0.5f, p.y - 8.f * k, lw, 16.f * k), juce::Justification::centred); }
        if (geom.hi.isNotEmpty()) { auto p = L(geom.hiX, geom.hiY); g.setFont(ui::fitFont(geom.hi, 11.5f * k, lw)); g.drawText(geom.hi, juce::Rectangle<float>(p.x - lw * 0.5f, p.y - 8.f * k, lw, 16.f * k), juce::Justification::centred); }
        if (geom.valBox && geom.valW > 0.f) {
            auto vb = value.getBounds().toFloat(); g.setColour(ui::field); g.fillRoundedRectangle(vb, 4.f * k);
            g.setColour(ui::fieldEdge); g.drawRoundedRectangle(vb.reduced(0.5f), 4.f * k, 1.f);
        }
    }
private:
    KnobGeom geom; juce::Point<float> origin; float sc = 0.75f; bool dragging = false; std::unique_ptr<SA::SliderAttachment> att;
};


// ------------------------------------------------------------------ FFT spectrum analyzer (4096-point, Hann, fast attack / slow release)
class Analyzer {
public:
    static constexpr int N = AnalyzerFeed::N;
    explicit Analyzer(const AnalyzerFeed& f) : feed(f), fft(12), data(2 * N, 0.f) {
        for (int i = 0; i < N; ++i) win[(size_t) i] = 0.5f - 0.5f * std::cos(2.f * juce::MathConstants<float>::pi * (float) i / (float) (N - 1));
        db.fill(-120.f);
    }
    void update(double sr) {
        sampleRate = sr > 0 ? sr : 44100.0; const unsigned w = feed.w.load(std::memory_order_acquire);
        if (w == lastW) { for (auto& v : db) v = std::max(-120.f, v - 2.5f); return; }   // transport stopped: let it fall
        lastW = w;
        for (int i = 0; i < N; ++i) data[(size_t) i] = feed.buf[(w - (unsigned) N + (unsigned) i) & (unsigned) (N - 1)].load(std::memory_order_relaxed) * win[(size_t) i];
        std::fill(data.begin() + N, data.end(), 0.f);
        fft.performFrequencyOnlyForwardTransform(data.data());
        for (int i = 0; i < N / 2; ++i) {
            const float d = 20.f * std::log10(data[(size_t) i] / ((float) N * 0.25f) + 1e-9f);
            db[(size_t) i] = d > db[(size_t) i] ? d : db[(size_t) i] * 0.82f + d * 0.18f;
        }
    }
    // spectrum level (dBFS, +4.5 dB/oct tilt so music looks flat-ish) at frequency f: max over a 1/6-octave neighbourhood
    float at(float f) const {
        const int i0 = std::clamp((int) std::floor(f / 1.06f * (float) N / (float) sampleRate), 1, N / 2 - 1), i1 = std::clamp((int) std::ceil(f * 1.06f * (float) N / (float) sampleRate), i0, N / 2 - 1);
        float m = -120.f; for (int i = i0; i <= i1; ++i) m = std::max(m, db[(size_t) i]);
        return m + 4.5f * std::log2(f / 1000.f);
    }
    // filled / stroked spectrum path over the plot; yOf maps a dB value to a y pixel
    template <typename YFn> void build(juce::Rectangle<float> plot, YFn yOf, juce::Path& line, juce::Path& fill) const {
        const int steps = std::max(2, (int) (plot.getWidth() / 2.f));
        for (int s = 0; s <= steps; ++s) {
            const float t = (float) s / (float) steps, f = 20.f * std::pow(1000.f, t), x = plot.getX() + t * plot.getWidth(), y = yOf(at(f));
            if (s == 0) { line.startNewSubPath(x, y); fill.startNewSubPath(x, plot.getBottom()); fill.lineTo(x, y); } else { line.lineTo(x, y); fill.lineTo(x, y); }
        }
        fill.lineTo(plot.getRight(), plot.getBottom()); fill.closeSubPath();
    }
private:
    const AnalyzerFeed& feed; juce::dsp::FFT fft; std::vector<float> data; std::array<float, N> win{}; std::array<float, N / 2> db{}; unsigned lastW = 0; double sampleRate = 44100.0;
};

// ------------------------------------------------------------------ main graph: level history, gain reduction trace, transfer curve
class DisplayComponent : public juce::Component {
public:
    explicit DisplayComponent(RajvikProcessor& p) : proc(p), anIn(p.anIn), anOut(p.anOut) { lastRead = proc.meters.w.load(); }
    void pull() {
        if (isVisible()) { anIn.update(proc.getSampleRate()); anOut.update(proc.getSampleRate()); }
        const unsigned w = proc.meters.w.load(std::memory_order_acquire);
        unsigned from = lastRead; if (w - from > MeterRing::N) from = w - MeterRing::N;
        for (unsigned i = from; i != w; ++i) {
            const auto k = i % MeterRing::N;
            hist.push_back({ rdp::linToDb(proc.meters.in[k].load()), rdp::linToDb(proc.meters.out[k].load()), proc.meters.gr[k].load() });
        }
        lastRead = w;
        while (hist.size() > 600) hist.erase(hist.begin());
        if (!hist.empty()) { curIn = hist.back().in; curOut = hist.back().out; curGr = hist.back().gr; }
        if (isVisible()) repaint();
    }
    float curIn = -100, curOut = -100, curGr = 0;
    void paint(juce::Graphics& g) override {
        const float k = ui::scaleOf(*this);
        const juce::Rectangle<float> plot(40.f * k, 5.f * k, 1205.f * k, 268.f * k);
        auto Y = [&](float db) { return plot.getY() + plot.getHeight() * (-std::clamp(db, -60.f, 0.f) / 60.f); };
        auto X = [&](float db) { return plot.getX() + plot.getWidth() * ((std::clamp(db, -60.f, 0.f) + 60.f) / 60.f); };
        g.setFont(ui::font(13.f * k));
        for (int db = 0; db >= -60; db -= 6) {
            g.setColour(ui::grid); g.drawHorizontalLine((int) Y((float) db), plot.getX(), plot.getRight());
            g.setColour(ui::dim);
            if (db != 0) g.drawText(juce::String(db), juce::Rectangle<float>(0.f, Y((float) db) - 8.f * k, 34.f * k, 16.f * k), juce::Justification::centredRight);
            g.drawText(juce::String(db), juce::Rectangle<float>(plot.getRight() + 6.f * k, Y((float) db) - 8.f * k, 34.f * k, 16.f * k), juce::Justification::centredLeft);
        }
        g.setColour(ui::orange); g.setFont(ui::font(13.f * k, true)); g.drawText("GR", juce::Rectangle<float>(plot.getRight() - 10.f * k, plot.getY() - 6.f * k, 46.f * k, 14.f * k), juce::Justification::centredRight);

        {   // scrolling waveform (peak envelope, mirrored): input grey, compressed output blue - lives in the lower part of the plot
            const int nh = (int) hist.size();
            if (nh > 1) {
                const float cy = plot.getY() + plot.getHeight() * 0.80f, half = plot.getHeight() * 0.17f, dx = plot.getWidth() / 600.f, x0 = plot.getRight() - (float) nh * dx;
                g.setColour(ui::grid); g.drawHorizontalLine((int) cy, plot.getX(), plot.getRight());
                auto env = [&](bool useOut, juce::Path& pth) {
                    for (int i = 0; i < nh; ++i) { const float a = std::min(1.f, rdp::dbToLin(useOut ? hist[(size_t) i].out : hist[(size_t) i].in)); const float x = x0 + (float) i * dx;
                        if (i == 0) pth.startNewSubPath(x, cy - a * half); else pth.lineTo(x, cy - a * half); }
                    for (int i = nh - 1; i >= 0; --i) { const float a = std::min(1.f, rdp::dbToLin(useOut ? hist[(size_t) i].out : hist[(size_t) i].in)); pth.lineTo(x0 + (float) i * dx, cy + a * half); }
                    pth.closeSubPath(); };
                juce::Path wi, wo; env(false, wi); env(true, wo);
                g.setColour(juce::Colour(0xffb7bfcd).withAlpha(0.30f)); g.fillPath(wi);
                g.setColour(ui::blue.withAlpha(0.65f)); g.fillPath(wo);
            }
        }
        {   // live spectrum of the input (grey) and the compressed output (blue)
            auto specY = [&](float dbv) { return Y(dbv + 30.f); };
            juce::Path li, fi, lo, fo; anIn.build(plot, specY, li, fi); anOut.build(plot, specY, lo, fo);
            g.setGradientFill(juce::ColourGradient(ui::blue.withAlpha(0.40f), 0.f, plot.getY(), ui::blue.withAlpha(0.03f), 0.f, plot.getBottom(), false)); g.fillPath(fo);
            g.setGradientFill(juce::ColourGradient(juce::Colour(0xffb7bfcd).withAlpha(0.22f), 0.f, plot.getY(), juce::Colour(0xffb7bfcd).withAlpha(0.02f), 0.f, plot.getBottom(), false)); g.fillPath(fi);
            g.setColour(juce::Colour(0xffb7bfcd).withAlpha(0.8f)); g.strokePath(li, juce::PathStrokeType(1.2f * k));
            g.setColour(ui::blue); g.strokePath(lo, juce::PathStrokeType(1.5f * k));
        }
        const int n = (int) hist.size();
        if (n > 1) {   // gain-reduction history (scrolls right to left)
            const float dx = plot.getWidth() / 600.f, x0 = plot.getRight() - (float) n * dx; juce::Path gr, grFill;
            for (int i = 0; i < n; ++i) {
                const float x = x0 + (float) i * dx, yg = plot.getY() + 14.f * k + std::min(std::fabs(hist[(size_t) i].gr), 40.f) / 60.f * plot.getHeight();
                if (i == 0) { gr.startNewSubPath(x, yg); grFill.startNewSubPath(x, plot.getY() + 14.f * k); grFill.lineTo(x, yg); } else { gr.lineTo(x, yg); grFill.lineTo(x, yg); }
            }
            grFill.lineTo(plot.getRight(), plot.getY() + 14.f * k); grFill.closeSubPath();
            g.setColour(ui::orange.withAlpha(0.22f)); g.fillPath(grFill); g.setColour(ui::orange); g.strokePath(gr, juce::PathStrokeType(1.6f * k));
        }
        // transfer curve from the live parameters
        auto& a = proc.apvts; const auto& m = rdp::kModes[(int) ui::getP(a, "mode")];
        const float thr = ui::getP(a, "threshold"), ratio = ui::getP(a, "ratio"), rng = ui::getP(a, "range"), kneeP = ui::getP(a, "knee");
        const float knee = std::clamp(kneeP + m.kneeAdd, 0.f, 72.f);
        juce::Path tc;
        for (int i = 0; i <= 240; ++i) {
            const float db = -60.f + (float) i * 0.25f, o = db + rdp::staticGainDb(m.kind, db, thr, ratio, knee, rng);
            if (i == 0) tc.startNewSubPath(X(db), Y(o)); else tc.lineTo(X(db), Y(o));
        }
        g.setColour(ui::accent.withAlpha(0.22f)); g.strokePath(tc, juce::PathStrokeType(5.f * k));
        g.setColour(ui::accent); g.strokePath(tc, juce::PathStrokeType(2.f * k));
        const float dash[] = { 4.f * k, 4.f * k }; g.setColour(ui::text.withAlpha(0.8f));
        g.drawDashedLine(juce::Line<float>(X(thr), plot.getY(), X(thr), plot.getBottom()), dash, 2, 1.2f * k);
        {   // knee read-out box
            auto box = juce::Rectangle<float>(X(thr) + 2.f * k, plot.getY() + plot.getHeight() * 0.72f, 66.f * k, 40.f * k);
            if (box.getRight() > plot.getRight()) box.setX(X(thr) - 68.f * k);
            g.setColour(juce::Colour(0xe0111826)); g.fillRoundedRectangle(box, 4.f * k); g.setColour(ui::fieldEdge); g.drawRoundedRectangle(box, 4.f * k, 1.f);
            g.setColour(ui::text); g.setFont(ui::font(13.f * k));
            g.drawText("Knee", box.withTrimmedLeft(8.f * k).withHeight(box.getHeight() * 0.5f), juce::Justification::centredLeft);
            g.drawText(juce::String(kneeP, 1) + " dB", box.withTrimmedLeft(8.f * k).withTrimmedTop(box.getHeight() * 0.5f), juce::Justification::centredLeft);
        }
        if (curIn > -59.f) {   // live operating point
            const float px = X(curIn), py = Y(curOut);
            g.setColour(ui::accent.withAlpha(0.3f)); g.fillEllipse(px - 8.f * k, py - 8.f * k, 16.f * k, 16.f * k); g.setColour(ui::accent); g.fillEllipse(px - 4.f * k, py - 4.f * k, 8.f * k, 8.f * k);
        }
        // legend
        const float ly = plot.getBottom() + 22.f * k; g.setFont(ui::font(13.f * k));
        auto item = [&](float absX, const juce::String& label, juce::Colour c, int kind) {
            const float x = (absX - 30.f) * k; g.setColour(c);
            if (kind == 0) g.fillRoundedRectangle(x, ly - 2.5f * k, 14.f * k, 5.f * k, 2.5f * k);
            else if (kind == 1) { const float d2[] = { 3.f * k, 3.f * k }; g.drawDashedLine(juce::Line<float>(x, ly, x + 22.f * k, ly), d2, 2, 1.2f * k); }
            else { const float d3[] = { 3.f * k, 3.f * k }; g.drawDashedLine(juce::Line<float>(x + 3.f * k, ly - 8.f * k, x + 3.f * k, ly + 8.f * k), d3, 2, 1.2f * k); }
            g.setColour(ui::dim); g.drawText(label, juce::Rectangle<float>(x + (kind == 1 ? 34.f : 22.f) * k, ly - 9.f * k, 110.f * k, 18.f * k), juce::Justification::centredLeft);
        };
        item(70, "Input", juce::Colour(0xffb7bfcd), 0); item(151, "Output", ui::blue, 0); item(241, "Gain Reduction", ui::orange, 0);
        item(379, "Knee", ui::dim, 1); item(483, "Threshold", ui::text, 2);
    }
private:
    struct Frame { float in, out, gr; };
    RajvikProcessor& proc; Analyzer anIn, anOut; std::vector<Frame> hist; unsigned lastRead = 0;
};

// ------------------------------------------------------------------ multiband display: log-frequency axis, 2-4 band regions, thresholds, GR bars, draggable crossovers
class MultibandDisplay : public juce::Component {
public:
    MultibandDisplay(RajvikProcessor& p, int& selRef, std::function<void()> onSel) : proc(p), apvts(p.apvts), sel(selRef), onSelect(std::move(onSel)), an(p.anIn) {}
    void pull() {
        if (isVisible()) an.update(proc.getSampleRate());
        for (int b = 0; b < 4; ++b) {
            const float gr = proc.mbGr[(size_t) b].load(), lv = proc.mbLvl[(size_t) b].load();
            grS[b] = gr < grS[b] ? gr : grS[b] * 0.85f + gr * 0.15f;
            lvS[b] = lv > lvS[b] ? lv : lvS[b] - 1.5f;
        }
        if (isVisible()) repaint();
    }
    int numBands() const { return 2 + (int) ui::getP(apvts, "mbBands"); }
    void paint(juce::Graphics& g) override {
        const float k = ui::scaleOf(*this); auto pr = plot();
        auto Y = [&](float db) { return pr.getY() + pr.getHeight() * (-std::clamp(db, -60.f, 0.f) / 60.f); };
        g.setFont(ui::font(13.f * k));
        for (int db = 0; db >= -60; db -= 6) {
            g.setColour(ui::grid); g.drawHorizontalLine((int) Y((float) db), pr.getX(), pr.getRight());
            if (db != 0) { g.setColour(ui::dim); g.drawText(juce::String(db), juce::Rectangle<float>(0.f, Y((float) db) - 8.f * k, 34.f * k, 16.f * k), juce::Justification::centredRight); }
        }
        static const float fl[] = { 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000 };
        for (float f : fl) {
            g.setColour(ui::grid); g.drawVerticalLine((int) fx(f), pr.getY(), pr.getBottom());
            g.setColour(ui::dim); g.drawText(f >= 1000.f ? juce::String((int) (f / 1000.f)) + "k" : juce::String((int) f),
                juce::Rectangle<float>(fx(f) - 20.f * k, pr.getBottom() + 4.f * k, 40.f * k, 16.f * k), juce::Justification::centred);
        }
        {   // input spectrum behind the band regions
            juce::Path l, f; an.build(pr, [&](float dbv) { return Y(dbv + 30.f); }, l, f);
            g.setColour(juce::Colour(0xffb7bfcd).withAlpha(0.12f)); g.fillPath(f); g.setColour(juce::Colour(0xffb7bfcd).withAlpha(0.6f)); g.strokePath(l, juce::PathStrokeType(1.2f * k));
        }
        const int n = numBands(); const auto xo = crossovers(n);
        for (int b = 0; b < n; ++b) {
            const float xl = b == 0 ? pr.getX() : fx(xo[b - 1]), xr = b == n - 1 ? pr.getRight() : fx(xo[b]);
            const auto col = ui::bandCol[b]; const bool selB = (b == sel);
            const bool muted = ui::getP(apvts, "mbMute" + juce::String(b + 1)) > 0.5f, solo = ui::getP(apvts, "mbSolo" + juce::String(b + 1)) > 0.5f, byp = ui::getP(apvts, "mbByp" + juce::String(b + 1)) > 0.5f;
            auto reg = juce::Rectangle<float>(xl, pr.getY(), xr - xl, pr.getHeight());
            g.setGradientFill(juce::ColourGradient(col.withAlpha(selB ? 0.20f : 0.09f), 0.f, reg.getY(), col.withAlpha(selB ? 0.07f : 0.02f), 0.f, reg.getBottom(), false)); g.fillRect(reg);
            // band input level
            const float ly = Y(lvS[b]); g.setColour(col.withAlpha(muted ? 0.08f : 0.30f)); g.fillRect(reg.withTop(ly).reduced(2.f * k, 0.f));
            g.setColour(col.withAlpha(muted ? 0.2f : 0.9f)); g.fillRect(juce::Rectangle<float>(reg.getX() + 2.f * k, ly, reg.getWidth() - 4.f * k, 1.5f * k));
            // threshold line
            const float thr = ui::getP(apvts, "mbThr" + juce::String(b + 1)), ty = Y(thr);
            const float dash[] = { 5.f * k, 4.f * k }; g.setColour(col.withAlpha(byp ? 0.35f : 1.f));
            g.drawDashedLine(juce::Line<float>(reg.getX() + 4.f * k, ty, reg.getRight() - 4.f * k, ty), dash, 2, 1.6f * k);
            g.setFont(ui::font(12.f * k)); g.drawText(juce::String(thr, 1) + " dB", juce::Rectangle<float>(reg.getX() + 6.f * k, ty - 17.f * k, 70.f * k, 15.f * k), juce::Justification::centredLeft);
            // gain reduction bar from the top
            const float gh = std::min(std::fabs(grS[b]), 30.f) / 30.f * pr.getHeight() * 0.45f; auto bar = juce::Rectangle<float>(reg.getCentreX() - 16.f * k, pr.getY(), 32.f * k, gh);
            g.setColour(ui::orange.withAlpha(0.85f)); g.fillRoundedRectangle(bar, 2.f * k);
            g.setColour(ui::text); g.setFont(ui::font(13.f * k, true));
            g.drawText(juce::String(grS[b], 1), juce::Rectangle<float>(reg.getCentreX() - 40.f * k, pr.getY() + gh + 2.f * k, 80.f * k, 16.f * k), juce::Justification::centred);
            // band tag
            auto tag = juce::Rectangle<float>(reg.getX() + 8.f * k, pr.getBottom() - 28.f * k, 22.f * k, 22.f * k);
            g.setColour(col.withAlpha(selB ? 1.f : 0.6f)); g.fillEllipse(tag); g.setColour(ui::bg); g.setFont(ui::font(13.f * k, true)); g.drawText(juce::String(b + 1), tag, juce::Justification::centred);
            g.setColour(ui::text.withAlpha(0.8f)); g.setFont(ui::font(12.f * k));
            juce::String flagTxt; if (solo) flagTxt << "S "; if (muted) flagTxt << "M "; if (byp) flagTxt << "BYP";
            g.drawText(flagTxt, juce::Rectangle<float>(tag.getRight() + 4.f * k, tag.getY(), 90.f * k, tag.getHeight()), juce::Justification::centredLeft);
            if (selB) { g.setColour(col.withAlpha(0.7f)); g.drawRect(reg.reduced(0.5f), 1.f); }
        }
        for (int j = 0; j < n - 1; ++j) {   // crossover handles
            const float x = fx(xo[j]); g.setColour(ui::text.withAlpha(0.75f)); g.drawVerticalLine((int) x, pr.getY(), pr.getBottom());
            g.setColour(ui::text); g.fillEllipse(x - 5.f * k, pr.getCentreY() - 5.f * k, 10.f * k, 10.f * k);
            g.setFont(ui::font(12.f * k)); g.drawText(rdp::freqText(xo[j]), juce::Rectangle<float>(x - 40.f * k, pr.getBottom() - 54.f * k, 80.f * k, 16.f * k), juce::Justification::centred);
        }
    }
    void mouseDown(const juce::MouseEvent& e) override {
        const float k = ui::scaleOf(*this); auto pr = plot(); const int n = numBands(); const auto xo = crossovers(n);
        dragXo = -1; dragThr = -1;
        for (int j = 0; j < n - 1; ++j) if (std::fabs(e.position.x - fx(xo[j])) < 9.f * k) dragXo = j;
        if (dragXo >= 0) { ui::gesture(apvts, "mbX" + juce::String(dragXo + 1), true); return; }
        const int b = bandAt(e.position.x, n, xo); sel = b; if (onSelect) onSelect();
        const float thr = ui::getP(apvts, "mbThr" + juce::String(b + 1)), ty = pr.getY() + pr.getHeight() * (-thr / 60.f);
        if (std::fabs(e.position.y - ty) < 10.f * k) { dragThr = b; ui::gesture(apvts, "mbThr" + juce::String(b + 1), true); }
        repaint();
    }
    void mouseDrag(const juce::MouseEvent& e) override {
        auto pr = plot(); const int n = numBands();
        if (dragXo >= 0) {
            float f = xToF(e.position.x);
            const float lo = dragXo == 0 ? 25.f : ui::getP(apvts, "mbX" + juce::String(dragXo)) * 1.25f;
            const float hi = dragXo + 1 < n - 1 ? ui::getP(apvts, "mbX" + juce::String(dragXo + 2)) / 1.25f : 18000.f;
            ui::setP(apvts, "mbX" + juce::String(dragXo + 1), std::clamp(f, lo, std::max(lo, hi)));
        } else if (dragThr >= 0) {
            ui::setP(apvts, "mbThr" + juce::String(dragThr + 1), std::clamp(-(e.position.y - pr.getY()) / pr.getHeight() * 60.f, -60.f, 0.f));
        }
    }
    void mouseUp(const juce::MouseEvent&) override {
        if (dragXo >= 0) ui::gesture(apvts, "mbX" + juce::String(dragXo + 1), false);
        if (dragThr >= 0) ui::gesture(apvts, "mbThr" + juce::String(dragThr + 1), false);
        dragXo = dragThr = -1;
    }
private:
    juce::Rectangle<float> plot() const { const float k = ui::scaleOf(*this); return { 40.f * k, 5.f * k, 1205.f * k, 250.f * k }; }
    float fx(float f) const { auto pr = plot(); return pr.getX() + std::log(f / 20.f) / std::log(1000.f) * pr.getWidth(); }
    float xToF(float x) const { auto pr = plot(); return 20.f * std::pow(1000.f, std::clamp((x - pr.getX()) / pr.getWidth(), 0.f, 1.f)); }
    std::array<float, 3> crossovers(int n) const {   // same ordering rule as the DSP so the display always matches what you hear
        std::array<float, 3> f{}; float prev = 20.f;
        for (int j = 0; j < n - 1; ++j) { f[(size_t) j] = std::min(std::max(ui::getP(apvts, "mbX" + juce::String(j + 1)), prev * 1.2f), 18000.f); prev = f[(size_t) j]; }
        return f;
    }
    int bandAt(float x, int n, const std::array<float, 3>& xo) const { int b = 0; for (int j = 0; j < n - 1; ++j) if (x > fx(xo[(size_t) j])) b = j + 1; return b; }
    RajvikProcessor& proc; juce::AudioProcessorValueTreeState& apvts; int& sel; std::function<void()> onSelect;
    Analyzer an; float grS[4] = {}, lvS[4] = { -100, -100, -100, -100 }; int dragXo = -1, dragThr = -1;
};

// ------------------------------------------------------------------ IN / OUT / GR meters
class MeterStrip : public juce::Component {
public:
    float in = -100, out = -100, gr = 0, inHold = -100, outHold = -100, grMax = 0;
    float mom = -100, sho = -100, integ = -100, tp = -100, rmsIn = -100, rmsOut = -100;
    int outMode = 0;   // click the OUT read-out to cycle: peak / true-peak / RMS / LUFS-M / LUFS-S / LUFS-I
    std::function<void()> onReset;
    void update(float i, float o, float g) {
        in = std::max(i, in - 1.4f); out = std::max(o, out - 1.4f); gr = (std::fabs(g) > std::fabs(gr) ? g : gr * 0.9f + g * 0.1f);
        inHold = std::max(i, inHold - 0.12f); outHold = std::max(o, outHold - 0.12f); if (g < grMax) grMax = g; repaint();
    }
    void mouseDown(const juce::MouseEvent& e) override {
        const float k = ui::scaleOf(*this);
        if (e.position.y > 318.f * k && e.position.x > 80.f * k && e.position.x < 126.f * k) { outMode = (outMode + 1) % 6; repaint(); return; }
        inHold = outHold = -100; grMax = 0; if (onReset) onReset();
    }
    juce::String outText() const {
        auto f = [](float v) { return v <= -99.f ? juce::String("--") : juce::String(v, 1); };
        switch (outMode) { case 1: return f(tp); case 2: return f(rmsOut); case 3: return f(mom); case 4: return f(sho); case 5: return f(integ); default: return f(outHold); }
    }
    juce::String outTag() const { static const char* t[] = { "", "TP", "RMS", "M", "S", "I" }; return t[outMode]; }
    void paint(juce::Graphics& g) override {
        const float k = ui::scaleOf(*this);
        auto Y = [&](float db) { return (308.f - (std::clamp(db, -60.f, 5.f) + 60.f) * (270.f / 65.f)) * k; };   // local y (design: bars 38..308)
        g.setFont(ui::font(14.f * k)); g.setColour(ui::text); g.drawText("IN", juce::Rectangle<float>(36.f * k, 4.f * k, 40.f * k, 20.f * k), juce::Justification::centred);
        g.drawText("OUT", juce::Rectangle<float>(82.f * k, 4.f * k, 44.f * k, 20.f * k), juce::Justification::centred);
        g.setColour(ui::orange); g.setFont(ui::font(14.f * k, true)); g.drawText("GR", juce::Rectangle<float>(128.f * k, 14.f * k, 44.f * k, 20.f * k), juce::Justification::centred);
        g.setFont(ui::font(13.f * k)); g.setColour(ui::dim);
        for (int db : { 0, -6, -12, -18, -24, -30, -36, -42, -48, -60 }) g.drawText(juce::String(db), juce::Rectangle<float>(0.f, Y((float) db) - 8.f * k, 34.f * k, 16.f * k), juce::Justification::centredRight);
        auto bar = [&](float x, float level, float hold) {
            auto b = juce::Rectangle<float>(x * k, 38.f * k, 23.f * k, 270.f * k);
            g.setColour(juce::Colour(0xff080b11)); g.fillRoundedRectangle(b, 2.f * k);
            const float top = Y(level);
            if (top < b.getBottom()) {
                { juce::ColourGradient cg(ui::orange, 0.f, Y(0.f), juce::Colour(0xff2fb84a), 0.f, Y(-30.f), false); cg.addColour(0.35, ui::accent); g.setGradientFill(cg); }
                g.fillRoundedRectangle(b.withTop(top), 2.f * k);
            }
            g.setColour(ui::text); g.fillRect(b.getX(), Y(hold), b.getWidth(), 1.5f * k);
            g.setColour(hold > 0.f ? ui::red : juce::Colour(0xff3a2226)); g.fillRect(b.getX() + 3.f * k, b.getY() + 1.f * k, b.getWidth() - 6.f * k, 2.f * k);
        };
        bar(45.f, in, inHold); bar(92.f, out, outHold);
        {   // gain reduction bar grows from the top
            auto b = juce::Rectangle<float>(140.f * k, 38.f * k, 23.f * k, 270.f * k); g.setColour(juce::Colour(0xff080b11)); g.fillRoundedRectangle(b, 2.f * k);
            const float h = std::min(std::fabs(gr), 60.f) / 60.f * b.getHeight();
            g.setGradientFill(juce::ColourGradient(ui::accent, 0.f, b.getY(), ui::orange, 0.f, b.getY() + std::max(h, 1.f), false)); g.fillRoundedRectangle(b.withHeight(h), 2.f * k);
        }
        g.setFont(ui::font(15.f * k)); g.setColour(ui::text);
        auto num = [&](float cx, const juce::String& t) { g.drawText(t, juce::Rectangle<float>((cx - 28.f) * k, 316.f * k, 56.f * k, 22.f * k), juce::Justification::centred); };
        num(57.f, inHold <= -99.f ? "--" : juce::String(inHold, 1)); num(104.f, outText());
        if (outMode > 0) { g.setFont(ui::font(10.f * k)); g.setColour(ui::accent); g.drawText(outTag(), juce::Rectangle<float>(82.f * k, 336.f * k, 44.f * k, 12.f * k), juce::Justification::centred); }
        auto box = [&](float y, const juce::String& t, bool hi) {
            auto r = juce::Rectangle<float>(141.f * k, y * k, 28.f * k, 19.f * k);
            g.setColour(ui::field); g.fillRoundedRectangle(r, 3.f * k); g.setColour(hi ? ui::accent : ui::fieldEdge); g.drawRoundedRectangle(r.reduced(0.5f), 3.f * k, 1.f);
            g.setFont(ui::font(12.5f * k)); g.setColour(ui::text); g.drawText(t, r, juce::Justification::centred);
        };
        box(316.f, juce::String(gr, 1), false); box(337.f, juce::String(grMax, 1), true);
    }
};

// ------------------------------------------------------------------ sidechain EQ: 6 draggable nodes, per-band coloured curves
class EqGraph : public juce::Component {
public:
    EqGraph(RajvikProcessor& p, int& selRef, std::function<void()> onSel) : proc(p), apvts(p.apvts), sel(selRef), onSelect(std::move(onSel)), anPre(p.anIn), anPost(p.anDet) {}
    void pull() { if (isVisible()) { anPre.update(proc.getSampleRate()); anPost.update(proc.getSampleRate()); repaint(); } }
    int numBands() const { return 2 + (int) ui::getP(apvts, "eqBands"); }
    void paint(juce::Graphics& g) override {
        const float k = ui::scaleOf(*this); auto pr = plot(); auto r = getLocalBounds().toFloat();
        g.setColour(ui::field); g.fillRoundedRectangle(r, 8.f * k); g.setColour(ui::panelEdge); g.drawRoundedRectangle(r.reduced(0.5f), 8.f * k, 1.f);
        g.setFont(ui::font(12.f * k));
        for (float d : { 12.f, 6.f, 0.f, -6.f, -12.f }) {
            g.setColour(d == 0.f ? ui::faint : ui::grid); g.drawHorizontalLine((int) gy(d), pr.getX(), pr.getRight());
            g.setColour(ui::dim); g.drawText((d > 0 ? "+" : "") + juce::String((int) d), juce::Rectangle<float>(2.f * k, gy(d) - 8.f * k, pr.getX() - 8.f * k, 16.f * k), juce::Justification::centredRight);
        }
        static const float fl[] = { 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000 };
        for (float f : fl) {
            g.setColour(ui::grid); g.drawVerticalLine((int) fx(f), pr.getY(), pr.getBottom());
            g.setColour(ui::dim); g.drawText(f >= 1000.f ? juce::String((int) (f / 1000.f)) + "k" : juce::String((int) f), juce::Rectangle<float>(fx(f) - 20.f * k, pr.getBottom() + 4.f * k, 40.f * k, 16.f * k), juce::Justification::centred);
        }
        {   // what the compressor hears: detector spectrum before (grey) and after (white) the sidechain EQ
            auto specY = [&](float dbv) { return pr.getBottom() - std::clamp((dbv + 100.f) / 80.f, 0.f, 1.f) * pr.getHeight(); };
            juce::Path l1, f1, l2, f2; anPre.build(pr, specY, l1, f1); anPost.build(pr, specY, l2, f2);
            g.setColour(ui::text.withAlpha(0.07f)); g.fillPath(f2);
            g.setColour(ui::dim.withAlpha(0.45f)); g.strokePath(l1, juce::PathStrokeType(1.f * k));
            g.setColour(ui::text.withAlpha(0.55f)); g.strokePath(l2, juce::PathStrokeType(1.2f * k));
        }
        const double sr = proc.getSampleRate() > 0 ? proc.getSampleRate() : 44100.0; const int nb = numBands();
        const bool powered = ui::getP(apvts, "scEqOn") > 0.5f && ui::getP(apvts, "scOn") > 0.5f; const float alpha = powered ? 1.f : 0.35f;
        for (int i = 0; i < nb; ++i) {
            const auto b = band(i); if (!rdp::eqBandActive(b)) continue;
            juce::Path curve, fill; const float y0 = gy(0.f);
            for (int s = 0; s <= 200; ++s) {
                const float f = 20.f * std::pow(1000.f, (float) s / 200.f), x = fx(f), y = gy(std::clamp(rdp::eqBandResponseDb(b, f, sr), -12.f, 12.f));
                if (s == 0) { curve.startNewSubPath(x, y); fill.startNewSubPath(x, y0); fill.lineTo(x, y); } else { curve.lineTo(x, y); fill.lineTo(x, y); }
            }
            fill.lineTo(pr.getRight(), y0); fill.closeSubPath();
            g.setColour(ui::eqCol[i].withAlpha(0.22f * alpha)); g.fillPath(fill);
            g.setColour(ui::eqCol[i].withAlpha(0.95f * alpha)); g.strokePath(curve, juce::PathStrokeType(1.6f * k));
        }
        for (int i = 0; i < nb; ++i) {
            const auto p = nodePos(i); const bool on = band(i).on; const float rr = 6.f * k;
            if (i == sel) { g.setColour(ui::text.withAlpha(0.9f)); g.drawEllipse(p.x - rr - 3.f * k, p.y - rr - 3.f * k, 2.f * (rr + 3.f * k), 2.f * (rr + 3.f * k), 1.3f * k); }
            g.setColour(on ? ui::eqCol[i] : ui::eqCol[i].withAlpha(0.3f)); g.fillEllipse(p.x - rr, p.y - rr, 2.f * rr, 2.f * rr);
            g.setColour(ui::bg); g.drawEllipse(p.x - rr, p.y - rr, 2.f * rr, 2.f * rr, 1.2f * k);
        }
    }
    void mouseDown(const juce::MouseEvent& e) override {
        const float k = ui::scaleOf(*this); drag = -1; float best = 16.f * k;
        for (int i = 0; i < numBands(); ++i) { const float d = nodePos(i).getDistanceFrom(e.position); if (d < best) { best = d; drag = i; } }
        if (drag < 0) return;
        sel = drag; if (onSelect) onSelect();
        ids.clear(); const auto n = juce::String(drag + 1); ids.add("eqFreq" + n); if (!rdp::eqIsCut(band(drag).type)) ids.add("eqGain" + n);
        for (auto& id : ids) ui::gesture(apvts, id, true);
        mouseDrag(e);
    }
    void mouseDrag(const juce::MouseEvent& e) override {
        if (drag < 0) return; const auto n = juce::String(drag + 1);
        ui::setP(apvts, "eqFreq" + n, xToF(e.position.x));
        if (!rdp::eqIsCut(band(drag).type)) ui::setP(apvts, "eqGain" + n, std::clamp(yToDb(e.position.y), -18.f, 18.f));
    }
    void mouseUp(const juce::MouseEvent&) override { for (auto& id : ids) ui::gesture(apvts, id, false); ids.clear(); drag = -1; }
    void mouseDoubleClick(const juce::MouseEvent& e) override {   // double-click a node = band on/off
        const float k = ui::scaleOf(*this);
        for (int i = 0; i < numBands(); ++i) if (nodePos(i).getDistanceFrom(e.position) < 16.f * k) {
            const auto id = "eqOn" + juce::String(i + 1); ui::gesture(apvts, id, true); ui::setP(apvts, id, ui::getP(apvts, id) > 0.5f ? 0.f : 1.f); ui::gesture(apvts, id, false); }
    }
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& w) override {   // wheel on a node = Q
        const float k = ui::scaleOf(*this);
        for (int i = 0; i < numBands(); ++i) if (!rdp::eqIsCut(band(i).type) && nodePos(i).getDistanceFrom(e.position) < 22.f * k) {
            const auto id = "eqQ" + juce::String(i + 1); ui::setP(apvts, id, std::clamp(ui::getP(apvts, id) * std::pow(1.15f, w.deltaY * 6.f), 0.1f, 10.f)); return; }
    }
private:
    rdp::EqBand band(int i) const {
        const auto n = juce::String(i + 1); rdp::EqBand b;
        b.on = ui::getP(apvts, "eqOn" + n) > 0.5f; b.type = (int) ui::getP(apvts, "eqType" + n); b.freq = ui::getP(apvts, "eqFreq" + n);
        b.gain = ui::getP(apvts, "eqGain" + n); b.q = ui::getP(apvts, "eqQ" + n); b.slope = (int) ui::getP(apvts, "eqSlope" + n); return b;
    }
    juce::Rectangle<float> plot() const { const float W = (float) getWidth(), H = (float) getHeight(); return { W * 37.f / 718.f, H * 9.f / 205.f, W * (680.f / 718.f), H * (168.f / 205.f) }; }
    float fx(float f) const { auto pr = plot(); return pr.getX() + std::log(f / 20.f) / std::log(1000.f) * pr.getWidth(); }
    float xToF(float x) const { auto pr = plot(); return 20.f * std::pow(1000.f, std::clamp((x - pr.getX()) / pr.getWidth(), 0.f, 1.f)); }
    float gy(float db) const { auto pr = plot(); return pr.getY() + (12.f - db) / 24.f * pr.getHeight(); }
    float yToDb(float y) const { auto pr = plot(); return 12.f - (y - pr.getY()) / pr.getHeight() * 24.f; }
    juce::Point<float> nodePos(int i) const { const auto b = band(i); return { fx(b.freq), gy(rdp::eqIsCut(b.type) ? 0.f : std::clamp(b.gain, -12.f, 12.f)) }; }
    RajvikProcessor& proc; juce::AudioProcessorValueTreeState& apvts; int& sel; std::function<void()> onSelect; Analyzer anPre, anPost; int drag = -1; juce::StringArray ids;
};

// ------------------------------------------------------------------ sidechain EQ band table (type / freq / gain-or-slope / on / next type)
class EqTable : public juce::Component {
public:
    EqTable(RajvikProcessor& p, int& selRef, std::function<void()> onSel) : apvts(p.apvts), sel(selRef), onSelect(std::move(onSel)) {}
    void paint(juce::Graphics& g) override {
        const float k = ui::scaleOf(*this), u = (float) getWidth() / 338.f; auto r = getLocalBounds().toFloat();
        g.setColour(ui::panel); g.fillRoundedRectangle(r, 8.f * k); g.setColour(ui::panelEdge); g.drawRoundedRectangle(r.reduced(0.5f), 8.f * k, 1.f);
        const int nb = 2 + (int) ui::getP(apvts, "eqBands"); static const char* tn[] = { "HPF", "Low Shelf", "Bell", "High Shelf", "LPF" };
        for (int i = 0; i < 6; ++i) {
            const auto n = juce::String(i + 1); const float cy = rowY(i); const bool inUse = i < nb, on = ui::getP(apvts, "eqOn" + n) > 0.5f; const int type = (int) ui::getP(apvts, "eqType" + n);
            const float a = (inUse && on) ? 1.f : 0.4f; const float rh = rowH();
            if (i == sel) { g.setColour(ui::accent.withAlpha(0.07f)); g.fillRoundedRectangle(6.f * u, cy - rh * 0.5f, r.getWidth() - 12.f * u, rh, 4.f * k); }
            g.setColour(inUse ? ui::eqCol[i].withAlpha(on ? 1.f : 0.4f) : ui::faint); g.fillEllipse(18.f * u - 5.f * k, cy - 5.f * k, 10.f * k, 10.f * k);
            g.setFont(ui::font(14.f * k)); g.setColour(ui::text.withAlpha(a));
            g.drawText(n, juce::Rectangle<float>(32.f * u, cy - 10.f * k, 18.f * u, 20.f * k), juce::Justification::centredLeft);
            g.drawText(tn[type], juce::Rectangle<float>(56.f * u, cy - 10.f * k, 80.f * u, 20.f * k), juce::Justification::centredLeft);
            g.drawText(rdp::freqText(ui::getP(apvts, "eqFreq" + n)), juce::Rectangle<float>(142.f * u, cy - 10.f * k, 68.f * u, 20.f * k), juce::Justification::centredLeft);
            const juce::String gt = rdp::eqIsCut(type) ? juce::String(12 << (int) ui::getP(apvts, "eqSlope" + n)) + " dB" : juce::String(ui::getP(apvts, "eqGain" + n), 1) + " dB";
            g.drawText(gt, juce::Rectangle<float>(213.f * u, cy - 10.f * k, 60.f * u, 20.f * k), juce::Justification::centredLeft);
            ui::drawIcon(g, "power", juce::Rectangle<float>(283.f * u - 9.f * k, cy - 9.f * k, 18.f * k, 18.f * k), on ? ui::accent : ui::faint, 1.5f * k);
            ui::drawIcon(g, type == rdp::EqBell ? "tBell" : (type == rdp::EqLowShelf || type == rdp::EqHighShelf ? "tShelf" : "tCut"),
                juce::Rectangle<float>(315.f * u - 10.f * k, cy - 10.f * k, 20.f * k, 20.f * k), ui::dim.withAlpha(a), 1.4f * k);
        }
    }
    void mouseDown(const juce::MouseEvent& e) override {
        const float u = (float) getWidth() / 338.f; row = std::clamp((int) ((e.position.y - margin()) / rowH()), 0, 5); const auto n = juce::String(row + 1); zone = 0; moved = false;
        sel = row; if (onSelect) onSelect();
        const float x = e.position.x / u;
        if (x > 268.f && x < 302.f) { const auto id = "eqOn" + n; ui::gesture(apvts, id, true); ui::setP(apvts, id, ui::getP(apvts, id) > 0.5f ? 0.f : 1.f); ui::gesture(apvts, id, false); }
        else if (x >= 302.f) { const auto id = "eqType" + n; ui::gesture(apvts, id, true); ui::setP(apvts, id, (float) (((int) ui::getP(apvts, id) + 1) % 5)); ui::gesture(apvts, id, false); }
        else if (x >= 135.f && x < 205.f) { zone = 1; startV = ui::getP(apvts, "eqFreq" + n); ui::gesture(apvts, "eqFreq" + n, true); }
        else if (x >= 205.f) { zone = 2; startV = ui::getP(apvts, "eqGain" + n); ui::gesture(apvts, "eqGain" + n, true); }
        repaint();
    }
    void mouseDrag(const juce::MouseEvent& e) override {
        if (zone == 0) return; const float u = (float) getWidth() / 338.f, dy = (float) e.getDistanceFromDragStartY(); const auto n = juce::String(row + 1);
        if (std::fabs(dy) > 2.f) moved = true;
        if (zone == 1) ui::setP(apvts, "eqFreq" + n, std::clamp(startV * std::pow(2.f, -dy / (30.f * u)), 20.f, 20000.f));
        else if (!rdp::eqIsCut((int) ui::getP(apvts, "eqType" + n))) ui::setP(apvts, "eqGain" + n, std::clamp(startV - dy * 0.1f / u, -18.f, 18.f));
    }
    void mouseUp(const juce::MouseEvent&) override {
        if (zone == 0) return; const auto n = juce::String(row + 1);
        ui::gesture(apvts, zone == 1 ? "eqFreq" + n : "eqGain" + n, false);
        if (zone == 2 && !moved && rdp::eqIsCut((int) ui::getP(apvts, "eqType" + n))) {   // click on a cut filter's slope = cycle 12/24/48
            const auto id = "eqSlope" + n; ui::gesture(apvts, id, true); ui::setP(apvts, id, (float) (((int) ui::getP(apvts, id) + 1) % 3)); ui::gesture(apvts, id, false); }
        zone = 0;
    }
private:
    float margin() const { return (float) getHeight() * 14.6f / 205.f; }
    float rowH() const { return ((float) getHeight() - 2.f * margin()) / 6.f; }
    float rowY(int i) const { return margin() + rowH() * ((float) i + 0.5f); }
    juce::AudioProcessorValueTreeState& apvts; int& sel; std::function<void()> onSelect; int row = 0, zone = 0; float startV = 0; bool moved = false;
};
