// Dev tool: renders the editor to PNG files (needs xvfb on Linux).  Usage: RdpShot <outdir>
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"
#include <cmath>
#include <complex>
#include <vector>
#include <cstdio>
static void feed(RajvikProcessor& p, int blocks, float& ph)
{
    juce::AudioBuffer<float> b(2, 512); juce::MidiBuffer m;
    for (int i = 0; i < blocks; ++i) {
        for (int s = 0; s < 512; ++s) {
            const float env = std::pow(std::fabs(std::sin(ph * 0.00035f)), 6.f) * 0.9f + 0.05f; ph += 1.f;
            const float v = env * (0.6f * std::sin(ph * 0.04f) + 0.3f * std::sin(ph * 0.13f) + 0.1f * std::sin(ph * 0.71f));
            b.setSample(0, s, v); b.setSample(1, s, v * 0.9f);
        }
        p.processBlock(b, m);
        juce::MessageManager::getInstance()->runDispatchLoopUntil(12);
    }
}
static void save(juce::Component& c, const juce::File& f)
{
    auto img = c.createComponentSnapshot(c.getLocalBounds(), true, 1.f); juce::PNGImageFormat png; f.deleteFile();
    juce::FileOutputStream os(f); png.writeImageToStream(img, os);
}
static double thdProc(RajvikProcessor& p, double f0, double sr = 48000)
{
    juce::AudioBuffer<float> b(2, 512); juce::MidiBuffer m; std::vector<float> y; long t = 0;
    for (int blk = 0; blk < (int) (sr * 4 / 512); ++blk) {
        for (int s = 0; s < 512; ++s, ++t) { const float v = 0.4f * (float) std::sin(6.283185307 * f0 * (double) t / sr); b.setSample(0, s, v); b.setSample(1, s, v); }
        p.processBlock(b, m); if (blk > (int) (sr * 2 / 512)) for (int s = 0; s < 512; ++s) y.push_back(b.getSample(0, s));
    }
    const int per = (int) (sr / f0 + 0.5), cyc = (int) (y.size() / per); y.resize((size_t) cyc * per);
    auto bin = [&](double f) { std::complex<double> a = 0; for (size_t i = 0; i < y.size(); ++i) { const double w = 0.5 - 0.5 * std::cos(6.283185307 * (double) i / (double) (y.size() - 1)); a += w * y[i] * std::polar(1.0, -6.283185307 * f * (double) i / sr); } return std::abs(a); };
    double h = 0; for (int k = 2; k <= 9; ++k) { const double v = bin(f0 * k); h += v * v; }
    return 20 * std::log10(std::sqrt(h) / bin(f0) + 1e-12);
}
int main(int argc, char** argv)
{
    if (argc > 2 && juce::String(argv[2]) == "thd") {   // whole-plug-in distortion check
        juce::ScopedJuceInitialiser_GUI init2;
        std::printf("full plug-in chain, 0.4 FS sine, thr -30 ratio 4 (harmonics dB re fundamental)\n%-22s %9s %9s\n", "setting", "60 Hz", "1 kHz");
        struct Case { const char* name; int mode; int os; float look; bool autoG; int mb; };
        for (auto c : { Case{ "Clean", 0, 0, 0, false, 0 }, Case{ "Classic", 1, 0, 0, false, 0 }, Case{ "Bus", 5, 0, 0, false, 0 }, Case{ "FET", 15, 0, 0, false, 0 }, Case{ "SSL Bus", 16, 0, 0, false, 0 }, Case{ "VCA", 17, 0, 0, false, 0 },
                        Case{ "Clean + 4x oversample", 0, 2, 0, false, 0 }, Case{ "Clean + lookahead 5ms", 0, 0, 5, false, 0 }, Case{ "Clean + auto gain", 0, 0, 0, true, 0 }, Case{ "Clean multiband", 0, 0, 0, false, 1 } }) {
            RajvikProcessor p; p.setPlayConfigDetails(2, 2, 48000, 512); p.prepareToPlay(48000, 512);
            auto set = [&](const char* id, float v) { ui::setP(p.apvts, id, v); };
            set("mode", (float) c.mode); set("threshold", -30); set("ratio", 4); set("oversampling", (float) c.os); set("lookahead", c.look); set("autoGain", c.autoG ? 1.f : 0.f); set("mbOn", (float) c.mb);
            if (c.mb) for (int b = 1; b <= 4; ++b) { set(("mbThr" + juce::String(b)).toRawUTF8(), -30); set(("mbRatio" + juce::String(b)).toRawUTF8(), 4); }
            const double a = thdProc(p, 60.0); RajvikProcessor q; q.setPlayConfigDetails(2, 2, 48000, 512); q.prepareToPlay(48000, 512);
            auto set2 = [&](const char* id, float v) { ui::setP(q.apvts, id, v); };
            set2("mode", (float) c.mode); set2("threshold", -30); set2("ratio", 4); set2("oversampling", (float) c.os); set2("lookahead", c.look); set2("autoGain", c.autoG ? 1.f : 0.f); set2("mbOn", (float) c.mb);
            if (c.mb) for (int b = 1; b <= 4; ++b) { set2(("mbThr" + juce::String(b)).toRawUTF8(), -30); set2(("mbRatio" + juce::String(b)).toRawUTF8(), 4); }
            std::printf("%-22s %9.1f %9.1f\n", c.name, a, thdProc(q, 1000.0));
        }
        return 0;
    }
    juce::ScopedJuceInitialiser_GUI init; juce::File out(argc > 1 ? argv[1] : "/tmp/shots"); out.createDirectory();
    RajvikProcessor p; p.setPlayConfigDetails(2, 2, 48000, 512); p.prepareToPlay(48000, 512);
    p.setRateAndBufferSizeDetails(48000, 512);
    std::unique_ptr<juce::AudioProcessorEditor> ed(p.createEditor()); auto* e = dynamic_cast<RajvikEditor*>(ed.get());
    auto set = [&](const char* id, float v) { ui::setP(p.apvts, id, v); };
    set("mode", 7); set("threshold", -24); set("ratio", 8); set("character", 1); set("drive", 6); set("autoGain", 1);
    float ph = 0; ed->setSize(1000, 600); feed(p, 140, ph);
    save(*ed, out.getChildFile("main.png"));
    e->showCharacterPanel(true); feed(p, 4, ph); save(*ed, out.getChildFile("character.png")); e->showCharacterPanel(false);
    e->setSidechainOpen(true); feed(p, 30, ph); save(*ed, out.getChildFile("sidechain.png"));
    e->setSidechainOpen(false); e->setHelpOn(true); feed(p, 4, ph); save(*ed, out.getChildFile("help.png"));
    e->setHelpOn(false); ed->setSize(440, 264); feed(p, 4, ph); save(*ed, out.getChildFile("small.png"));
    ed->setSize(1000, 600); set("mbOn", 1); set("mbExt2", 1); feed(p, 40, ph); save(*ed, out.getChildFile("multiband.png"));
    ed.reset(); return 0;
}
