#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

namespace rdp {
struct FactoryPreset { juce::String name, category; std::vector<std::pair<juce::String, float>> values; };

class PresetManager {
public:
    explicit PresetManager(juce::AudioProcessorValueTreeState& a) : apvts(a) { build(); }
    const std::vector<FactoryPreset>& factory() const { return presets; }

    void resetToDefaults() {
        for (auto* p : apvts.processor.getParameters())
            p->setValueNotifyingHost(p->getDefaultValue());
    }
    void loadFactory(int i) {
        if (i < 0 || i >= (int) presets.size()) return;
        if (apvts.undoManager) apvts.undoManager->beginNewTransaction("Load preset");
        resetToDefaults();
        for (auto& [id, v] : presets[(size_t) i].values)
            if (auto* p = apvts.getParameter(id)) p->setValueNotifyingHost(p->convertTo0to1(v));
    }
    static juce::File userDir() {
        auto d = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                     .getChildFile("RAJVIK").getChildFile("DynamicsPro").getChildFile("Presets");
        d.createDirectory(); return d;
    }
    juce::Array<juce::File> userPresets() const { return userDir().findChildFiles(juce::File::findFiles, false, "*.rdpreset"); }
    bool saveUser(const juce::String& name) const {
        auto xml = apvts.copyState().createXml();
        return xml != nullptr && xml->writeTo(userDir().getChildFile(juce::File::createLegalFileName(name) + ".rdpreset"));
    }
    bool loadUser(const juce::File& f) {
        if (auto xml = juce::XmlDocument::parse(f))
            if (xml->hasTagName(apvts.state.getType())) { apvts.replaceState(juce::ValueTree::fromXml(*xml)); return true; }
        return false;
    }
private:
    void add(const char* name, const char* cat, std::vector<std::pair<juce::String, float>> v) { presets.push_back({ name, cat, std::move(v) }); }
    void build() {
        // mode: 0 Clean 1 Classic 2 Opto 3 Vocal 4 Mastering 5 Bus 6 Punch 7 Pumping 8 Versatile 9 Smooth 10 Upward 11 TTM 12 Op-El 13 Vari-Mu
        add("Smooth Vocal", "Vocals", {{"mode",3.0f},{"threshold",-24.0f},{"ratio",3.0f},{"attack",8.0f},{"release",150.0f},{"knee",12.0f},{"autoGain",1.0f}});
        add("Aggressive Vocal", "Vocals", {{"mode",1.0f},{"threshold",-28.0f},{"ratio",8.0f},{"attack",2.0f},{"release",80.0f},{"knee",3.0f},{"character",2.0f},{"drive",6.0f}});
        add("Tight Snare", "Drums", {{"mode",1.0f},{"threshold",-18.0f},{"ratio",6.0f},{"attack",3.0f},{"release",60.0f},{"knee",2.0f}});
        add("Punchy Kick", "Kick and Bass", {{"mode",6.0f},{"threshold",-20.0f},{"ratio",5.0f},{"attack",15.0f},{"release",80.0f},{"knee",4.0f},{"scHpf",60.0f}});
        add("EDM Pump", "EDM Pumping", {{"mode",7.0f},{"threshold",-30.0f},{"ratio",10.0f},{"attack",1.0f},{"release",250.0f},{"scHpf",40.0f}});
        add("EDM Pump Sync", "EDM Pumping", {{"mode",7.0f},{"threshold",-30.0f},{"ratio",10.0f},{"attack",1.0f},{"release",250.0f},{"syncOn",1.0f},{"syncRate",2.0f},{"syncPulse",20.0f}});
        add("Drum Bus", "Drum Bus", {{"mode",6.0f},{"threshold",-20.0f},{"ratio",4.0f},{"attack",20.0f},{"release",100.0f}});
        add("Glue Bus", "Mix Bus", {{"mode",5.0f},{"threshold",-16.0f},{"ratio",2.0f},{"attack",30.0f},{"release",200.0f},{"knee",8.0f}});
        add("Transparent Master", "Mastering", {{"mode",4.0f},{"threshold",-14.0f},{"ratio",1.8f},{"attack",30.0f},{"release",250.0f},{"knee",16.0f}});
        add("Fast Limiter", "Mastering", {{"mode",0.0f},{"threshold",-6.0f},{"ratio",100.0f},{"attack",0.05f},{"release",40.0f},{"knee",0.0f},{"lookahead",5.0f}});
        add("Warm Opto", "Acoustic Instruments", {{"mode",2.0f},{"threshold",-22.0f},{"ratio",3.0f},{"attack",20.0f},{"release",300.0f},{"character",1.0f},{"drive",6.0f}});
        add("Pick Control", "Guitar and Bass", {{"mode",8.0f},{"threshold",-20.0f},{"ratio",4.0f},{"attack",10.0f},{"release",120.0f}});
        add("Rise Up", "Creative Effects", {{"mode",10.0f},{"threshold",-45.0f},{"ratio",3.0f},{"attack",20.0f},{"release",300.0f}});
        add("Parallel Drums", "Parallel Compression", {{"mode",1.0f},{"threshold",-34.0f},{"ratio",8.0f},{"attack",5.0f},{"release",100.0f},{"mix",40.0f}});
    }
    juce::AudioProcessorValueTreeState& apvts;
    std::vector<FactoryPreset> presets;
};
} // namespace rdp
