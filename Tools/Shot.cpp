// Dev tool: renders the editor to PNG files (needs xvfb on Linux).  Usage: RdpShot <outdir>
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"
#include <cmath>
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
int main(int argc, char** argv)
{
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
    ed->setSize(1000, 600); set("mbOn", 1); feed(p, 40, ph); save(*ed, out.getChildFile("multiband.png"));
    ed.reset(); return 0;
}
