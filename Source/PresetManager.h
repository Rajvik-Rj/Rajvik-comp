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
        add("Smooth Vocal", "Vocals", {{"mode",3},{"threshold",-24},{"ratio",3},{"attack",8},{"release",150},{"knee",12},{"autoGain",1}});
        add("Aggressive Vocal", "Vocals", {{"mode",1},{"threshold",-28},{"ratio",8},{"attack",2},{"release",80},{"knee",3},{"character",2},{"drive",6}});
        add("Tight Snare", "Drums", {{"mode",1},{"threshold",-18},{"ratio",6},{"attack",3},{"release",60},{"knee",2}});
        add("Punchy Kick", "Kick and Bass", {{"mode",6},{"threshold",-20},{"ratio",5},{"attack",15},{"release",80},{"knee",4},{"scHpf",60}});
        add("EDM Pump", "EDM Pumping", {{"mode",7},{"threshold",-30},{"ratio",10},{"attack",1},{"release",250},{"scHpf",40}});
        add("EDM Pump Sync", "EDM Pumping", {{"mode",7},{"threshold",-30},{"ratio",10},{"attack",1},{"release",250},{"syncOn",1},{"syncRate",2},{"syncPulse",20}});
        add("Drum Bus", "Drum Bus", {{"mode",6},{"threshold",-20},{"ratio",4},{"attack",20},{"release",100}});
        add("Glue Bus", "Mix Bus", {{"mode",5},{"threshold",-16},{"ratio",2},{"attack",30},{"release",200},{"knee",8}});
        add("Transparent Master", "Mastering", {{"mode",4},{"threshold",-14},{"ratio",1.8f},{"attack",30},{"release",250},{"knee",16}});
        add("Fast Limiter", "Mastering", {{"mode",0},{"threshold",-6},{"ratio",100},{"attack",0.05f},{"release",40},{"knee",0},{"lookahead",5}});
        add("Warm Opto", "Acoustic Instruments", {{"mode",2},{"threshold",-22},{"ratio",3},{"attack",20},{"release",300},{"character",1},{"drive",6}});
        add("Pick Control", "Guitar and Bass", {{"mode",8},{"threshold",-20},{"ratio",4},{"attack",10},{"release",120}});
        add("Rise Up", "Creative Effects", {{"mode",10},{"threshold",-45},{"ratio",3},{"attack",20},{"release",300}});
        add("Parallel Drums", "Parallel Compression", {{"mode",1},{"threshold",-34},{"ratio",8},{"attack",5},{"release",100},{"mix",40}});
    }
    juce::AudioProcessorValueTreeState& apvts;
    std::vector<FactoryPreset> presets;
};
} // namespace rdp
