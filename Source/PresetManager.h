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
        add("Default Preset", "Default", {});
        add("Smooth Vocal", "Vocals", {{"mode",3},{"threshold",-24},{"ratio",3},{"attack",8},{"release",150},{"knee",12},{"autoGain",1}});
        add("Aggressive Vocal", "Vocals", {{"mode",1},{"threshold",-28},{"ratio",8},{"attack",2},{"release",80},{"knee",3},{"character",2},{"drive",6}});
        add("Tight Snare", "Drums", {{"mode",1},{"threshold",-18},{"ratio",6},{"attack",3},{"release",60},{"knee",2}});
        add("Punchy Kick", "Kick and Bass", {{"mode",6},{"threshold",-20},{"ratio",5},{"attack",15},{"release",80},{"knee",4},{"eqFreq1",60}});
        add("EDM Pump", "EDM Pumping", {{"mode",7},{"threshold",-30},{"ratio",10},{"attack",1},{"release",250},{"eqFreq1",40}});
        add("EDM Pump Sync", "EDM Pumping", {{"mode",7},{"threshold",-30},{"ratio",10},{"attack",1},{"release",250},{"syncOn",1},{"syncRate",2},{"syncPulse",20}});
        add("Drum Bus", "Drum Bus", {{"mode",6},{"threshold",-20},{"ratio",4},{"attack",20},{"release",100}});
        add("Glue Bus", "Mix Bus", {{"mode",5},{"threshold",-16},{"ratio",2},{"attack",30},{"release",200},{"knee",8}});
        add("Transparent Master", "Mastering", {{"mode",4},{"threshold",-14},{"ratio",1.8f},{"attack",30},{"release",250},{"knee",16}});
        add("Fast Limiter", "Mastering", {{"mode",0},{"threshold",-6},{"ratio",100},{"attack",0.05f},{"release",40},{"knee",0},{"lookahead",5}});
        add("Warm Opto", "Acoustic Instruments", {{"mode",2},{"threshold",-22},{"ratio",3},{"attack",20},{"release",300},{"character",1},{"drive",6}});
        add("Pick Control", "Guitar and Bass", {{"mode",8},{"threshold",-20},{"ratio",4},{"attack",10},{"release",120}});
        add("Rise Up", "Creative Effects", {{"mode",10},{"threshold",-45},{"ratio",3},{"attack",20},{"release",300}});
        add("Parallel Drums", "Parallel Compression", {{"mode",1},{"threshold",-34},{"ratio",8},{"attack",5},{"release",100},{"dry",-6}});
        // ---- multiband: mbOn=1; mbBands index 0/1/2 = 2/3/4 bands ----
        add("Master 4-Band Glue", "Multiband", {{"mode",4},{"mbOn",1},{"mbBands",2},{"mbX1",120},{"mbX2",1200},{"mbX3",6000},
            {"mbThr1",-22},{"mbRatio1",2.5f},{"mbAtk1",40},{"mbRel1",250},{"mbThr2",-20},{"mbRatio2",2},{"mbAtk2",25},{"mbRel2",200},
            {"mbThr3",-20},{"mbRatio3",2},{"mbAtk3",15},{"mbRel3",150},{"mbThr4",-24},{"mbRatio4",2.5f},{"mbAtk4",8},{"mbRel4",120},{"knee",12}});
        add("Tame The Lows", "Multiband", {{"mode",0},{"mbOn",1},{"mbBands",1},{"mbX1",100},{"mbX2",3000},
            {"mbThr1",-28},{"mbRatio1",5},{"mbAtk1",20},{"mbRel1",180},{"mbThr2",0},{"mbRatio2",1},{"mbThr3",0},{"mbRatio3",1}});
        add("De-Ess Multiband", "Multiband", {{"mode",1},{"mbOn",1},{"mbBands",1},{"mbX1",250},{"mbX2",5500},
            {"mbThr1",0},{"mbRatio1",1},{"mbThr2",0},{"mbRatio2",1},{"mbThr3",-30},{"mbRatio3",6},{"mbAtk3",0.5f},{"mbRel3",60},{"knee",4}});
        add("Vocal 3-Band Leveler", "Multiband", {{"mode",3},{"mbOn",1},{"mbBands",1},{"mbX1",200},{"mbX2",4000},
            {"mbThr1",-26},{"mbRatio1",3},{"mbAtk1",15},{"mbRel1",160},{"mbThr2",-24},{"mbRatio2",3},{"mbAtk2",8},{"mbRel2",140},
            {"mbThr3",-26},{"mbRatio3",2.5f},{"mbAtk3",3},{"mbRel3",90}});
        add("Mix Bus 2-Band", "Multiband", {{"mode",5},{"mbOn",1},{"mbBands",0},{"mbX1",180},{"mbThr1",-18},{"mbRatio1",2},{"mbAtk1",35},{"mbRel1",200},
            {"mbThr2",-16},{"mbRatio2",1.8f},{"mbAtk2",20},{"mbRel2",150}});
        // ---- sidechain EQ look from the reference design (gentle bass cut, presence lift) ----
        add("Sidechain EQ Showcase", "Sidechain", {{"eqFreq1",20},{"eqType2",1},{"eqFreq2",120},{"eqGain2",3},{"eqFreq3",500},{"eqGain3",-4},
            {"eqFreq4",2000},{"eqGain4",3},{"eqFreq5",5000},{"eqGain5",-3},{"eqType6",3},{"eqFreq6",12000},{"eqGain6",4}});
    }
    juce::AudioProcessorValueTreeState& apvts;
    std::vector<FactoryPreset> presets;
};
} // namespace rdp
