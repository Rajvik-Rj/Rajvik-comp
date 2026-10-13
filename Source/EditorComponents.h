#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

namespace ui {
inline const juce::Colour bg{ 0xff131418 }, panel{ 0xff1c1e24 }, accent{ 0xffffc83a }, text{ 0xffd7dae0 }, dim{ 0xff8b909a },
                          cyan{ 0xff35d0e8 }, green{ 0xff4cd964 }, orange{ 0xffff8a3d }, blue{ 0xff4d8dff };
}

struct RajvikLnF : juce::LookAndFeel_V4 {
    RajvikLnF() {
        setColour(juce::Slider::textBoxTextColourId, ui::text); setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour(juce::ComboBox::backgroundColourId, ui::panel); setColour(juce::ComboBox::textColourId, ui::text);
        setColour(juce::TextButton::buttonColourId, ui::panel); setColour(juce::TextButton::textColourOffId, ui::text);
        setColour(juce::ToggleButton::textColourId, ui::text); setColour(juce::ToggleButton::tickColourId, ui::accent);
        setColour(juce::PopupMenu::backgroundColourId, ui::panel);
    }
    void drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider&) override {
        auto b = juce::Rectangle<float>((float) x, (float) y, (float) w, (float) h).reduced(5.f);
        const float r = std::min(b.getWidth(), b.getHeight()) * 0.5f; const auto c = b.getCentre(); const float ang = a0 + pos * (a1 - a0);
        g.setColour(ui::panel.brighter(0.08f)); g.fillEllipse(c.x - r, c.y - r, 2 * r, 2 * r);
        juce::Path t; t.addCentredArc(c.x, c.y, r - 3, r - 3, 0, a0, a1, true);
        g.setColour(juce::Colour(0xff2c2f38)); g.strokePath(t, juce::PathStrokeType(4.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        juce::Path v; v.addCentredArc(c.x, c.y, r - 3, r - 3, 0, a0, ang, true);
        g.setColour(ui::accent); g.strokePath(v, juce::PathStrokeType(4.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        juce::Path p; p.addRoundedRectangle(-1.5f, -r + 9.f, 3.f, r * 0.35f, 1.5f);
        g.fillPath(p, juce::AffineTransform::rotation(ang).translated(c.x, c.y));
    }
};

struct Knob {
    juce::Slider s{ juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow };
    juce::Label label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
    void init(juce::Component& parent, juce::AudioProcessorValueTreeState& a, const juce::String& id, const juce::String& name) {
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 74, 16);
        label.setText(name, juce::dontSendNotification); label.setJustificationType(juce::Justification::centred);
        label.setColour(juce::Label::textColourId, ui::dim);
        parent.addAndMakeVisible(s); parent.addAndMakeVisible(label);
        att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(a, id, s);
    }
    void place(juce::Rectangle<int> r) { label.setBounds(r.removeFromTop(16)); s.setBounds(r); }
};

// Real-time history of input level / output level / gain change, fed from the DSP's per-block meter frames
class DisplayComponent : public juce::Component {
public:
    explicit DisplayComponent(RajvikProcessor& p) : proc(p) { lastRead = proc.meters.w.load(); }
    void pull() {
        const unsigned w = proc.meters.w.load(std::memory_order_acquire);
        unsigned from = lastRead; if (w - from > MeterRing::N) from = w - MeterRing::N;
        for (unsigned i = from; i != w; ++i) {
            const auto k = i % MeterRing::N;
            hist.push_back({ rdp::linToDb(proc.meters.in[k].load()), rdp::linToDb(proc.meters.out[k].load()), proc.meters.gr[k].load() });
        }
        lastRead = w;
        while (hist.size() > 600) hist.erase(hist.begin());
        if (!hist.empty()) { curIn = hist.back().in; curOut = hist.back().out; curGr = hist.back().gr; }
        repaint();
    }
    float curIn = -100, curOut = -100, curGr = 0;
    void paint(juce::Graphics& g) override {
        auto r = getLocalBounds().toFloat();
        g.setColour(ui::panel); g.fillRoundedRectangle(r, 6.f);
        auto curve = r.removeFromRight(std::min(r.getHeight(), r.getWidth() * 0.3f)).reduced(8.f);
        auto h = r.reduced(8.f);
        auto Y = [&](float db) { return h.getY() + h.getHeight() * (1.f - (std::clamp(db, -60.f, 0.f) + 60.f) / 60.f); };
        g.setFont(10.f);
        for (int db = -60; db <= 0; db += 12) {
            g.setColour(juce::Colour(0xff2a2d35)); g.drawHorizontalLine((int) Y((float) db), h.getX(), h.getRight());
            g.setColour(ui::dim); g.drawText(juce::String(db), (int) h.getX(), (int) Y((float) db) - 10, 28, 10, juce::Justification::left);
        }
        const int n = (int) hist.size();
        if (n > 1) {
            const float dx = h.getWidth() / 600.f; const float x0 = h.getRight() - n * dx;
            juce::Path in, out, gr;
            in.startNewSubPath(x0, h.getBottom());
            for (int i = 0; i < n; ++i) { in.lineTo(x0 + i * dx, Y(hist[(size_t) i].in)); }
            in.lineTo(h.getRight(), h.getBottom()); in.closeSubPath();
            g.setColour(juce::Colour(0x40808898)); g.fillPath(in);
            for (int i = 0; i < n; ++i) { const float x = x0 + i * dx, y = Y(hist[(size_t) i].out); if (i == 0) out.startNewSubPath(x, y); else out.lineTo(x, y); }
            g.setColour(ui::accent); g.strokePath(out, juce::PathStrokeType(1.6f));
            for (int i = 0; i < n; ++i) {
                const float gdb = hist[(size_t) i].gr, hh = std::min(std::fabs(gdb), 30.f) / 30.f * h.getHeight() * 0.5f;
                g.setColour((gdb < 0 ? ui::orange : ui::cyan).withAlpha(0.55f));
                g.fillRect(x0 + i * dx, h.getY(), std::max(dx, 1.f), hh);
            }
        }
        // transfer curve with knee/threshold/ratio from the live parameters
        g.setColour(juce::Colour(0xff15171c)); g.fillRoundedRectangle(curve, 4.f);
        auto& a = proc.apvts;
        const auto& m = rdp::kModes[(int) *a.getRawParameterValue("mode")];
        const float thr = *a.getRawParameterValue("threshold"), ratio = *a.getRawParameterValue("ratio"), rng = *a.getRawParameterValue("range");
        const float knee = std::clamp(*a.getRawParameterValue("knee") + m.kneeAdd, 0.f, 72.f);
        auto tx = [&](float db) { return curve.getX() + curve.getWidth() * (db + 60.f) / 60.f; };
        auto ty = [&](float db) { return curve.getBottom() - curve.getHeight() * (std::clamp(db, -60.f, 0.f) + 60.f) / 60.f; };
        g.setColour(juce::Colour(0xff2a2d35)); g.drawLine(tx(-60), ty(-60), tx(0), ty(0));
        juce::Path tc;
        for (int db = -60; db <= 0; ++db) {
            const float o = (float) db + rdp::staticGainDb(m.kind, (float) db, thr, ratio, knee, rng);
            if (db == -60) tc.startNewSubPath(tx((float) db), ty(o)); else tc.lineTo(tx((float) db), ty(o));
        }
        g.setColour(ui::accent); g.strokePath(tc, juce::PathStrokeType(2.f));
        g.setColour(ui::cyan); g.drawVerticalLine((int) tx(thr), curve.getY(), curve.getBottom());
        g.setColour(ui::text); g.fillEllipse(tx(curIn) - 3.f, ty(curOut) - 3.f, 6.f, 6.f);
    }
private:
    struct Frame { float in, out, gr; };
    RajvikProcessor& proc; std::vector<Frame> hist; unsigned lastRead = 0;
};

class MeterStrip : public juce::Component {
public:
    float in = -100, out = -100, gr = 0, inHold = -100, outHold = -100;
    float mom = -100, sho = -100, integ = -100, tp = -100, rmsIn = -100, rmsOut = -100;
    std::function<void()> onReset;
    void update(float i, float o, float g) {
        in = std::max(i, in - 1.2f); out = std::max(o, out - 1.2f); gr = (std::fabs(g) > std::fabs(gr) ? g : gr * 0.9f + g * 0.1f);
        inHold = std::max(i, inHold - 0.15f); outHold = std::max(o, outHold - 0.15f); repaint();
    }
    void mouseDown(const juce::MouseEvent&) override { inHold = outHold = -100; if (onReset) onReset(); }   // click = reset peaks + loudness
    void paint(juce::Graphics& g) override {
        auto r = getLocalBounds().toFloat().reduced(6.f);
        g.setColour(ui::panel); g.fillRoundedRectangle(getLocalBounds().toFloat(), 6.f);
        auto bar = [&](juce::Rectangle<float> b, float db, float hold, juce::Colour c, const char* t) {
            g.setColour(juce::Colour(0xff0f1014)); g.fillRect(b);
            const float f = (std::clamp(db, -60.f, 6.f) + 60.f) / 66.f, hy = b.getBottom() - b.getHeight() * (std::clamp(hold, -60.f, 6.f) + 60.f) / 66.f;
            g.setColour(db > 0.f ? juce::Colours::red : c); g.fillRect(b.withTop(b.getBottom() - b.getHeight() * f));
            g.setColour(ui::text); g.drawHorizontalLine((int) hy, b.getX(), b.getRight());
            g.setColour(ui::dim); g.setFont(10.f); g.drawText(t, b.withY(b.getBottom() + 2).withHeight(12), juce::Justification::centred);
        };
        const float w = r.getWidth() / 3.f - 6.f; auto body = r.withTrimmedBottom(16 + 92);
        {   // numeric readouts: momentary / short-term / integrated LUFS, true peak, RMS in/out
            auto f = [](float v) { return v <= -99.f ? juce::String("--") : juce::String(v, 1); };
            g.setColour(ui::text); g.setFont(11.f);
            auto t = r.withTop(r.getBottom() - 90).toNearestInt();
            g.drawText("M   " + f(mom), t.removeFromTop(15), juce::Justification::left);
            g.drawText("S   " + f(sho), t.removeFromTop(15), juce::Justification::left);
            g.drawText("I   " + f(integ) + " LU", t.removeFromTop(15), juce::Justification::left);
            g.drawText("TP  " + f(tp) + " dB", t.removeFromTop(15), juce::Justification::left);
            g.drawText("RMS " + f(rmsIn) + "/" + f(rmsOut), t.removeFromTop(15), juce::Justification::left);
            g.setColour(ui::dim); g.setFont(9.f); g.drawText("click = reset", t, juce::Justification::left);
        }
        bar({ r.getX(), body.getY(), w, body.getHeight() }, in, inHold, ui::green, "IN");
        bar({ r.getX() + w + 9, body.getY(), w, body.getHeight() }, out, outHold, ui::green, "OUT");
        const float gx = r.getX() + 2 * (w + 9), gh = std::min(std::fabs(gr), 30.f) / 30.f * body.getHeight();
        g.setColour(juce::Colour(0xff0f1014)); g.fillRect(gx, body.getY(), w, body.getHeight());
        g.setColour(gr < 0 ? ui::orange : ui::cyan); g.fillRect(gx, body.getY(), w, gh);
        g.setColour(ui::dim); g.setFont(10.f); g.drawText("GR", (int) gx, (int) body.getBottom() + 2, (int) w, 12, juce::Justification::centred);
    }
};

// Interactive sidechain EQ: drag nodes (freq = x, gain = y), mouse-wheel on a node = Q (bell)
class EqGraph : public juce::Component {
public:
    EqGraph(RajvikProcessor& p) : proc(p), apvts(p.apvts) {}
    void paint(juce::Graphics& g) override {
        auto r = getLocalBounds().toFloat();
        g.setColour(ui::panel); g.fillRoundedRectangle(r, 6.f);
        g.setColour(juce::Colour(0xff2a2d35));
        for (float f : { 50.f, 100.f, 500.f, 1000.f, 5000.f, 10000.f }) g.drawVerticalLine((int) fx(f), 0.f, r.getHeight());
        for (float d : { -12.f, 0.f, 12.f }) g.drawHorizontalLine((int) gy(d), 0.f, r.getWidth());
        rdp::SidechainEQ eq; eq.prepare(proc.getSampleRate() > 0 ? proc.getSampleRate() : 44100.0); eq.setParams(readParams());
        juce::Path c;
        for (int i = 0; i <= 240; ++i) {
            const float f = 20.f * std::pow(1000.f, (float) i / 240.f), y = gy(std::clamp(eq.magnitudeDb(f), -24.f, 24.f));
            if (i == 0) c.startNewSubPath(fx(f), y); else c.lineTo(fx(f), y);
        }
        g.setColour(ui::accent); g.strokePath(c, juce::PathStrokeType(2.f));
        for (int i = 0; i < 5; ++i) {
            auto p = nodePos(i);
            g.setColour(nodeColour(i)); g.fillEllipse(p.x - 7, p.y - 7, 14, 14);
            g.setColour(ui::bg); g.setFont(10.f); g.drawText(juce::String(i + 1), (int) p.x - 7, (int) p.y - 7, 14, 14, juce::Justification::centred);
        }
    }
    void mouseDown(const juce::MouseEvent& e) override {
        sel = -1; float best = 18.f;
        for (int i = 0; i < 5; ++i) { const float d = nodePos(i).getDistanceFrom(e.position); if (d < best) { best = d; sel = i; } }
        if (sel >= 0) { gesture(true); mouseDrag(e); }
    }
    void mouseDrag(const juce::MouseEvent& e) override {
        if (sel < 0) return;
        const float f = xf(e.position.x), g = std::clamp(yg(e.position.y), -18.f, 18.f);
        static const char* fid[] = { "scHpf", "scLowFreq", "scBellFreq", "scHighFreq", "scLpf" };
        static const char* gid[] = { nullptr, "scLowGain", "scBellGain", "scHighGain", nullptr };
        set(fid[sel], f); if (gid[sel]) set(gid[sel], g);
    }
    void mouseUp(const juce::MouseEvent&) override { if (sel >= 0) gesture(false); sel = -1; }
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& w) override {
        for (int i = 0; i < 5; ++i) if (i == 2 && nodePos(i).getDistanceFrom(e.position) < 24.f) {
            const float q = *apvts.getRawParameterValue("scBellQ"); set("scBellQ", q * std::pow(1.15f, w.deltaY * 6.f)); }
    }
private:
    rdp::SidechainEqParams readParams() const {
        auto v = [&](const char* id) { return (float) *apvts.getRawParameterValue(id); };
        rdp::SidechainEqParams p; p.hpfHz = v("scHpf"); p.hpfStages = 1 << (int) v("scHpfSlope"); p.lpfHz = v("scLpf");
        p.lowHz = v("scLowFreq"); p.lowDb = v("scLowGain"); p.bellHz = v("scBellFreq"); p.bellDb = v("scBellGain"); p.bellQ = v("scBellQ");
        p.highHz = v("scHighFreq"); p.highDb = v("scHighGain"); return p;
    }
    float fx(float f) const { return std::log(f / 20.f) / std::log(1000.f) * (float) getWidth(); }
    float xf(float x) const { return 20.f * std::pow(1000.f, std::clamp(x / (float) getWidth(), 0.f, 1.f)); }
    float gy(float db) const { return (float) getHeight() * (0.5f - db / 48.f); }
    float yg(float y) const { return (0.5f - y / (float) getHeight()) * 48.f; }
    juce::Point<float> nodePos(int i) const {
        auto v = [&](const char* id) { return (float) *apvts.getRawParameterValue(id); };
        switch (i) { case 0: return { fx(v("scHpf")), gy(0) }; case 1: return { fx(v("scLowFreq")), gy(v("scLowGain")) };
                     case 2: return { fx(v("scBellFreq")), gy(v("scBellGain")) }; case 3: return { fx(v("scHighFreq")), gy(v("scHighGain")) };
                     default: return { fx(v("scLpf")), gy(0) }; }
    }
    juce::Colour nodeColour(int i) const { static const juce::Colour c[] = { ui::orange, ui::green, ui::cyan, ui::blue, ui::accent }; return c[i]; }
    void set(const char* id, float v) { if (auto* p = apvts.getParameter(id)) p->setValueNotifyingHost(p->convertTo0to1(v)); }
    void gesture(bool begin) {
        static const char* fid[] = { "scHpf", "scLowFreq", "scBellFreq", "scHighFreq", "scLpf" };
        static const char* gid[] = { nullptr, "scLowGain", "scBellGain", "scHighGain", nullptr };
        for (const char* id : { fid[sel], gid[sel] }) if (id) { auto* p = apvts.getParameter(id); begin ? p->beginChangeGesture() : p->endChangeGesture(); }
    }
    RajvikProcessor& proc; juce::AudioProcessorValueTreeState& apvts; int sel = -1;
};
