#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

// Layout is designed on a 1100 px wide canvas and scaled with  k = width / 1100.
namespace ui {
inline const juce::Colour bg{ 0xff131316 }, bgTop{ 0xff1c1c20 }, panel{ 0xff18181b }, panelEdge{ 0xff2c2c32 }, field{ 0xff1b1b1f }, fieldEdge{ 0xff35353c },
                          text{ 0xffdcdce2 }, dim{ 0xff90909a }, faint{ 0xff55555d }, accent{ 0xfff4c430 }, orange{ 0xffee8a2a }, blue{ 0xff4a90e2 },
                          green{ 0xff86c93e }, red{ 0xffe9474c }, grid{ 0xff232328 };
inline const juce::Colour eqCol[6] = { juce::Colour(0xff8a93a3), juce::Colour(0xffffc21a), juce::Colour(0xff3fd35a), juce::Colour(0xff2f8cff), juce::Colour(0xffb04dff), juce::Colour(0xffff6a2a) };
inline const juce::Colour bandCol[4] = { juce::Colour(0xffff6a2a), juce::Colour(0xffffc21a), juce::Colour(0xff3fd35a), juce::Colour(0xff2f8cff) };

inline juce::Font font(float h, bool bold = false, float kern = 0.f) {
    auto o = juce::FontOptions(h, bold ? juce::Font::bold : juce::Font::plain);
    if (kern > 0.f) o = o.withKerningFactor(kern);
    return juce::Font(o);
}
inline float scaleOf(const juce::Component& c) {
    if (auto* t = c.getTopLevelComponent()) return std::max(0.2f, (float) t->getWidth() / 1160.f);
    return 0.9f; }
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
    } else if (n == "phones") {   // headphones (side-chain listen)
        p.addCentredArc(x + w * .5f, y + w * .56f, w * .36f, w * .36f, 0.f, -juce::MathConstants<float>::halfPi, juce::MathConstants<float>::halfPi, true);
        p.addRoundedRectangle(x + w * .10f, y + w * .54f, w * .20f, w * .32f, w * .06f); p.addRoundedRectangle(x + w * .70f, y + w * .54f, w * .20f, w * .32f, w * .06f); stroke();
    } else if (n == "trigInt") {   // internal sidechain
        p.addEllipse(x + w * .14f, y + w * .14f, w * .72f, w * .72f); p.addEllipse(x + w * .40f, y + w * .40f, w * .20f, w * .20f); stroke();
    } else if (n == "trigExt") {   // external sidechain: signal into a box
        p.startNewSubPath(P(.06f, .5f)); p.lineTo(P(.52f, .5f)); p.startNewSubPath(P(.38f, .32f)); p.lineTo(P(.54f, .5f)); p.lineTo(P(.38f, .68f));
        p.addRoundedRectangle(x + w * .64f, y + w * .2f, w * .28f, w * .6f, w * .05f); stroke();
    } else if (n == "trigMidi") {   // 5-pin DIN
        p.addEllipse(x + w * .10f, y + w * .10f, w * .80f, w * .80f);
        for (int i = 0; i < 5; ++i) { const float a = -0.9f + (float) i * 0.45f; p.addEllipse(x + w * (.5f + .27f * std::sin(a) - .045f), y + w * (.5f - .27f * std::cos(a) - .045f), w * .09f, w * .09f); } stroke();
    } else if (n == "trigTempo") {   // note
        p.addEllipse(x + w * .20f, y + w * .60f, w * .30f, w * .22f); p.startNewSubPath(P(.48f, .68f)); p.lineTo(P(.48f, .16f)); p.cubicTo(P(.52f, .34f), P(.80f, .32f), P(.74f, .58f)); stroke();
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
        const float k = ui::scaleOf(*this); auto pr = plot(); const juce::Colour gold(0xfff5c542), dimc(0xff8c8c9c);
        g.fillAll(juce::Colour(0xff06060b));
        auto Y = [&](float db) { return pr.getY() + pr.getHeight() * (-std::clamp(db, -60.f, 0.f) / 60.f); };
        g.setFont(ui::font(10.5f * k));
        for (int db = 0; db >= -60; db -= 12) {
            g.setColour(juce::Colour(0xff14141d)); g.fillRect(pr.getX(), Y((float) db), pr.getWidth(), 1.f);
            g.setColour(dimc); g.drawText(juce::String(db), juce::Rectangle<float>(0.f, Y((float) db) - 8.f * k, 40.f * k, 16.f * k), juce::Justification::centredRight);
        }
        static const float fl[] = { 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000 };
        for (float f : fl) {
            g.setColour(juce::Colour(0xff14141d)); g.fillRect(fx(f), pr.getY(), 1.f, pr.getHeight());
            g.setColour(dimc); g.drawText(f >= 1000.f ? juce::String((int) (f / 1000.f)) + "k" : juce::String((int) f), juce::Rectangle<float>(fx(f) - 20.f * k, pr.getBottom() + 5.f * k, 40.f * k, 14.f * k), juce::Justification::centred);
        }
        {   // input spectrum (grey, like the analyzer of the single band view)
            juce::Path l, f; an.build(pr, [&](float dbv) { return Y(dbv + 36.f); }, l, f);
            g.setColour(juce::Colour(0xff3b3b4a).withAlpha(0.55f)); g.fillPath(f); g.setColour(juce::Colours::white.withAlpha(0.8f)); g.strokePath(l, juce::PathStrokeType(1.2f * k));
        }
        const int n = numBands(); const auto xo = crossovers(n); const bool extSig = proc.extPk.load() > 1e-5f;
        for (int b = 0; b < n; ++b) {
            const float xl = b == 0 ? pr.getX() : fx(xo[b - 1]), xr = b == n - 1 ? pr.getRight() : fx(xo[b]);
            const auto col = ui::bandCol[b]; const bool selB = (b == sel); const juce::String id = juce::String(b + 1);
            const bool muted = ui::getP(apvts, "mbMute" + id) > 0.5f, solo = ui::getP(apvts, "mbSolo" + id) > 0.5f, byp = ui::getP(apvts, "mbByp" + id) > 0.5f, ext = ui::getP(apvts, "mbExt" + id) > 0.5f;
            auto reg = juce::Rectangle<float>(xl, pr.getY(), xr - xl, pr.getHeight());
            g.setColour(col.withAlpha(selB ? 0.11f : 0.045f)); g.fillRect(reg); g.setColour(col.withAlpha(muted ? 0.25f : 0.9f)); g.fillRect(reg.withHeight(3.f * k));
            // band input level (thin line)
            const float ly = Y(lvS[b]); g.setColour(col.withAlpha(muted ? 0.2f : 0.85f)); g.fillRect(juce::Rectangle<float>(reg.getX() + 3.f * k, ly, reg.getWidth() - 6.f * k, 1.6f * k));
            // threshold (draggable) + value chip
            const float thr = ui::getP(apvts, "mbThr" + id), ty = Y(thr); const float dash[] = { 6.f * k, 4.f * k };
            g.setColour(col.withAlpha(byp ? 0.35f : 1.f)); g.drawDashedLine(juce::Line<float>(reg.getX() + 3.f * k, ty, reg.getRight() - 3.f * k, ty), dash, 2, 1.5f * k);
            auto chip = juce::Rectangle<float>(reg.getX() + 8.f * k, ty - 20.f * k, 62.f * k, 16.f * k); g.setColour(juce::Colour(0xff0f0f17)); g.fillRoundedRectangle(chip, 4.f * k); g.setColour(col); g.drawRoundedRectangle(chip, 4.f * k, 1.f);
            g.setFont(ui::font(10.f * k, true)); g.drawText(juce::String(thr, 1) + " dB", chip, juce::Justification::centred);
            // gain reduction hanging from the top
            const float gh = std::min(std::fabs(grS[b]), 24.f) / 24.f * pr.getHeight() * 0.5f; auto bar = juce::Rectangle<float>(reg.getCentreX() - 14.f * k, pr.getY() + 3.f * k, 28.f * k, gh);
            g.setGradientFill(juce::ColourGradient(col.withAlpha(0.95f), 0.f, bar.getY(), col.withAlpha(0.35f), 0.f, bar.getBottom() + 1.f, false)); g.fillRoundedRectangle(bar, 3.f * k);
            g.setColour(juce::Colours::white); g.setFont(ui::font(12.f * k, true));
            g.drawText(juce::String(grS[b], 1), juce::Rectangle<float>(reg.getCentreX() - 40.f * k, bar.getBottom() + 3.f * k, 80.f * k, 16.f * k), juce::Justification::centred);
            // band tag: number, style, flags
            auto tag = juce::Rectangle<float>(reg.getX() + 8.f * k, pr.getBottom() - 28.f * k, 22.f * k, 22.f * k);
            g.setColour(col.withAlpha(selB ? 1.f : 0.65f)); g.fillEllipse(tag); g.setColour(juce::Colour(0xff06060b)); g.setFont(ui::font(12.f * k, true)); g.drawText(id, tag, juce::Justification::centred);
            const int mv = (int) ui::getP(apvts, "mbMode" + id); const int mi = mv == 0 ? (int) ui::getP(apvts, "mode") : mv - 1;
            juce::String txt = juce::String(rdp::kModes[mi].name); if (solo) txt << "  S"; if (muted) txt << "  M"; if (byp) txt << "  BYP";
            g.setColour(juce::Colours::white.withAlpha(0.8f)); g.setFont(ui::font(10.f * k)); g.drawText(txt, juce::Rectangle<float>(tag.getRight() + 5.f * k, tag.getY(), reg.getWidth() - 40.f * k, tag.getHeight()), juce::Justification::centredLeft);
            if (ext) { auto ec = juce::Rectangle<float>(reg.getRight() - 62.f * k, pr.getBottom() - 26.f * k, 54.f * k, 18.f * k); const auto c = extSig ? juce::Colour(0xff5ee08a) : juce::Colour(0xffff5d5d);
                g.setColour(c.withAlpha(0.18f)); g.fillRoundedRectangle(ec, 4.f * k); g.setColour(c); g.drawRoundedRectangle(ec, 4.f * k, 1.f); g.setFont(ui::font(9.5f * k, true)); g.drawText(extSig ? "EXT SC" : "EXT: none", ec, juce::Justification::centred); }
            if (selB) { g.setColour(col.withAlpha(0.7f)); g.drawRect(reg.reduced(0.5f), 1.f); }
        }
        for (int j = 0; j < n - 1; ++j) {   // crossover handles
            const float x = fx(xo[j]); g.setColour(juce::Colours::white.withAlpha(0.45f)); g.fillRect(x - 0.5f, pr.getY(), 1.f, pr.getHeight());
            auto h = juce::Rectangle<float>(x - 5.f * k, pr.getCentreY() - 14.f * k, 10.f * k, 28.f * k); g.setColour(juce::Colour(0xffe9e9f0)); g.fillRoundedRectangle(h, 5.f * k);
            auto fc = juce::Rectangle<float>(x - 34.f * k, pr.getY() - 21.f * k, 68.f * k, 16.f * k); g.setColour(juce::Colour(0xff0f0f17)); g.fillRoundedRectangle(fc, 4.f * k); g.setColour(juce::Colour(0xffe9e9f0)); g.setFont(ui::font(10.f * k, true)); g.drawText(rdp::freqText(xo[j]), fc, juce::Justification::centred);
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
    juce::Rectangle<float> plot() const { const float k = ui::scaleOf(*this); return { 46.f * k, 54.f * k, (float) getWidth() - 64.f * k, (float) getHeight() - 84.f * k }; }
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

// ------------------------------------------------------------------ sidechain EQ: 6 draggable nodes, per-band coloured curves
class EqGraph : public juce::Component {
public:
    EqGraph(RajvikProcessor& p, int& selRef, std::function<void()> onSel) : proc(p), apvts(p.apvts), sel(selRef), onSelect(std::move(onSel)), anPre(p.anIn), anPost(p.anDet) {}
    void pull() { if (isVisible()) { anPre.update(proc.getSampleRate()); anPost.update(proc.getSampleRate()); repaint(); } }
    int numBands() const { return 2 + (int) ui::getP(apvts, "eqBands"); }
    void paint(juce::Graphics& g) override {
        const float k = ui::scaleOf(*this); auto pr = plot(); auto r = getLocalBounds().toFloat();
        g.setColour(juce::Colour(0xff101013)); g.fillRoundedRectangle(r, 4.f * k); g.setColour(ui::panelEdge); g.drawRoundedRectangle(r.reduced(0.5f), 4.f * k, 1.f);
        g.setFont(ui::font(11.f * k));
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
        if (e.mods.isPopupMenu()) {   // right-click a node: filter type menu
            juce::PopupMenu m; const char* tn[] = { "High-pass", "Low shelf", "Bell", "High shelf", "Low-pass", "All-pass" }; const int cur = band(drag).type; const auto idn = "eqType" + juce::String(drag + 1);
            for (int t = 0; t < 6; ++t) m.addItem(t + 1, tn[t], true, t == cur);
            m.showMenuAsync(juce::PopupMenu::Options(), [this, idn](int r) { if (r > 0) { ui::gesture(apvts, idn, true); ui::setP(apvts, idn, (float) (r - 1)); ui::gesture(apvts, idn, false); if (onSelect) onSelect(); } });
            drag = -1; return;
        }
        ids.clear(); const auto n = juce::String(drag + 1); ids.add("eqFreq" + n); if (rdp::eqHasGain(band(drag).type)) ids.add("eqGain" + n);
        for (auto& id : ids) ui::gesture(apvts, id, true);
        mouseDrag(e);
    }
    void mouseDrag(const juce::MouseEvent& e) override {
        if (drag < 0) return; const auto n = juce::String(drag + 1);
        ui::setP(apvts, "eqFreq" + n, xToF(e.position.x));
        if (rdp::eqHasGain(band(drag).type)) ui::setP(apvts, "eqGain" + n, std::clamp(yToDb(e.position.y), -18.f, 18.f));
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
    juce::Point<float> nodePos(int i) const { const auto b = band(i); return { fx(b.freq), gy(rdp::eqHasGain(b.type) ? std::clamp(b.gain, -12.f, 12.f) : 0.f) }; }
    RajvikProcessor& proc; juce::AudioProcessorValueTreeState& apvts; int& sel; std::function<void()> onSelect; Analyzer anPre, anPost; int drag = -1; juce::StringArray ids;
};

