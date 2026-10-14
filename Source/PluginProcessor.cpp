#include "PluginProcessor.h"
#include "PluginEditor.h"
using namespace rdp;

RajvikProcessor::RajvikProcessor()
    : AudioProcessor(BusesProperties()
          .withInput("Input", juce::AudioChannelSet::stereo(), true)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)
          .withInput("Sidechain", juce::AudioChannelSet::stereo(), false)),
      apvts(*this, &undo, "STATE", createLayout()), presets(apvts)
{
    auto r = [&](const juce::String& id) { return apvts.getRawParameterValue(id); };
    prm.mode = r("mode"); prm.thr = r("threshold"); prm.ratio = r("ratio"); prm.atk = r("attack"); prm.rel = r("release"); prm.mix = r("mix");
    prm.knee = r("knee"); prm.range = r("range"); prm.hold = r("hold"); prm.look = r("lookahead"); prm.link = r("link");
    prm.inG = r("inGain"); prm.outG = r("outGain"); prm.autoGain = r("autoGain"); prm.autoRel = r("autoRelease"); prm.det = r("detector");
    prm.chr = r("character"); prm.drive = r("drive"); prm.os = r("oversampling"); prm.stMode = r("stereoMode");
    prm.procMid = r("procMid"); prm.procSide = r("procSide");
    prm.syncOn = r("syncOn"); prm.syncRate = r("syncRate"); prm.syncOff = r("syncOffset"); prm.syncPulse = r("syncPulse");
    prm.scOn = r("scOn"); prm.scExt = r("scExternal"); prm.scRoute = r("scRoute"); prm.scLevel = r("scLevel"); prm.scListen = r("scListen");
    prm.scInvert = r("scInvert"); prm.scEqOn = r("scEqOn"); prm.eqBands = r("eqBands");
    prm.mbOn = r("mbOn"); prm.mbBands = r("mbBands");
    for (int j = 0; j < 3; ++j) prm.mbX[j] = r("mbX" + juce::String(j + 1));
    for (int i = 0; i < kEqBands; ++i) {
        const auto n = juce::String(i + 1);
        eqP[i] = { r("eqOn" + n), r("eqType" + n), r("eqFreq" + n), r("eqGain" + n), r("eqQ" + n), r("eqSlope" + n) };
    }
    for (int b = 0; b < 4; ++b) {
        const auto n = juce::String(b + 1);
        mbP[b] = { r("mbThr" + n), r("mbRatio" + n), r("mbAtk" + n), r("mbRel" + n), r("mbGain" + n), r("mbSolo" + n), r("mbMute" + n), r("mbByp" + n) };
        mbGr[(size_t) b] = 0.f; mbLvl[(size_t) b] = -100.f;
    }
    for (auto& c : ccMap) c.store(nullptr);
}

bool RajvikProcessor::isBusesLayoutSupported(const BusesLayout& l) const
{
    const auto o = l.getMainOutputChannelSet();
    if (o != juce::AudioChannelSet::mono() && o != juce::AudioChannelSet::stereo()) return false;
    if (l.getMainInputChannelSet() != o) return false;
    const auto s = l.getChannelSet(true, 1);
    return s.isDisabled() || s == juce::AudioChannelSet::mono() || s == juce::AudioChannelSet::stereo();
}

void RajvikProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    sr = sampleRate; maxBlock = std::max(samplesPerBlock, 32);
    engine.prepare(sr); mb.prepare(sr); eq.prepare(sr); sat.prepare(); meter.prepare(sr);
    const int maxLook = (int) std::ceil(sr * 0.020) + 2;
    for (int c = 0; c < 2; ++c) { lookDelay[c].prepare(maxLook); dryDelay[c].prepare(2048); wetDelay[c].prepare(2048); }
    wet.setSize(2, maxBlock); dry.setSize(2, maxBlock);
    for (int i = 0; i < 5; ++i) {
        os[(size_t) i] = std::make_unique<juce::dsp::Oversampling<float>>(2, (size_t) (i + 1),
            juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true);
        os[(size_t) i]->initProcessing((size_t) maxBlock);
        osLat[(size_t) i] = (int) std::round(os[(size_t) i]->getLatencyInSamples());
    }
    for (auto* s : { &inSm, &outSm, &mixSm, &mkSm }) { s->reset(sr, 0.02); s->setCurrentAndTargetValue(1.f); }
    pIn = pOut = 0; lastOs = -2;
}

void RajvikProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    // ---- MIDI learn / CC mapping ----
    for (const auto meta : midi) {
        const auto msg = meta.getMessage();
        if (!msg.isController()) continue;
        const int cc = msg.getControllerNumber();
        if (auto* lp = learnParam.exchange(nullptr)) { ccMap[(size_t) cc].store(lp); mapDirty = true; }
        else if (auto* p = ccMap[(size_t) cc].load()) p->setValueNotifyingHost((float) msg.getControllerValue() / 127.f);
    }

    auto main = getBusBuffer(buffer, true, 0);
    const int nCh = std::min(2, main.getNumChannels()), nS = buffer.getNumSamples();

    const float* scPtr[2] = { nullptr, nullptr }; int scCh = 0;
    if (getBusCount(true) > 1 && getBus(true, 1)->isEnabled()) {
        auto sb = getBusBuffer(buffer, true, 1);
        scCh = std::min(2, sb.getNumChannels());
        for (int c = 0; c < scCh; ++c) scPtr[c] = sb.getReadPointer(c);
    }

    // ---- parameters -> DSP (only recomputes when changed) ----
    EngineParams ep; ep.thrDb = *prm.thr; ep.ratio = *prm.ratio; ep.atkMs = *prm.atk; ep.relMs = *prm.rel; ep.kneeDb = *prm.knee;
    ep.rangeDb = *prm.range; ep.holdMs = *prm.hold; ep.link = *prm.link * 0.01f; ep.mode = (int) *prm.mode;
    ep.detector = (int) *prm.det; ep.autoRelease = *prm.autoRel > 0.5f;
    engine.setParams(ep);
    SidechainEqParams sp; sp.enabled = *prm.scEqOn > 0.5f; sp.numBands = 2 + (int) *prm.eqBands;
    for (int i = 0; i < kEqBands; ++i) {
        auto& b = sp.band[i]; const auto& q = eqP[i];
        b.on = *q.on > 0.5f; b.type = (int) *q.type; b.freq = *q.freq; b.gain = *q.gain; b.q = *q.q; b.slope = (int) *q.slope;
    }
    eq.setParams(sp);

    chrIdx = (int) *prm.chr; sat.setParams(chrIdx, *prm.drive);
    osIdx = (int) *prm.os - 1; osLatNow = osIdx >= 0 ? osLat[(size_t) osIdx] : 0;
    if (osIdx != lastOs) { if (osIdx >= 0) os[(size_t) osIdx]->reset(); lastOs = osIdx; }
    lookS = (int) std::lround(*prm.look * 0.001 * sr);
    const int totalLat = lookS + osLatNow;
    if (totalLat != getLatencySamples()) setLatencySamples(totalLat);
    scOnNow = *prm.scOn > 0.5f;
    listen = *prm.scListen > 0.5f;
    extSc = scOnNow && *prm.scExt > 0.5f && scCh > 0;
    msOn = *prm.stMode > 0.5f && nCh == 2;
    routeMode = (scOnNow && !msOn && nCh == 2) ? (int) *prm.scRoute : 0;          // Mid / Side detector routing (stereo mode only)
    detGain = scOnNow ? dbToLin(*prm.scLevel) * (*prm.scInvert > 0.5f ? -1.f : 1.f) : 1.f;
    eqRun = scOnNow && eq.isActive();
    procCh[0] = !msOn || *prm.procMid > 0.5f; procCh[1] = !msOn || *prm.procSide > 0.5f;

    // ---- multiband setup (per band thr/ratio/atk/rel/gain/solo/mute/bypass; everything else is shared) ----
    mbActive = *prm.mbOn > 0.5f;
    if (mbActive) {
        const int nb = 2 + (int) *prm.mbBands;
        float xo[3] = { *prm.mbX[0], *prm.mbX[1], *prm.mbX[2] };
        rdp::BandSettings bs[4];
        for (int b = 0; b < 4; ++b) {
            const auto& q = mbP[b];
            bs[b] = { *q.thr, *q.ratio, *q.atk, *q.rel, *q.gain, *q.solo > 0.5f, *q.mute > 0.5f, *q.byp > 0.5f };
        }
        mb.setup(nb, xo, ep, bs);
    }
    mbShared = mbActive && lookS == 0 && !extSc && !eqRun && routeMode == 0 && detGain == 1.f;

    // ---- tempo sync: host transport -> rhythmic trigger ----
    syncActive = false;
    if (*prm.syncOn > 0.5f && !mbActive)   // tempo-sync pulse detector is a single-band feature
        if (auto* ph = getPlayHead())
            if (auto pos = ph->getPosition())
                if (pos->getIsPlaying())
                    if (auto bpm = pos->getBpm())
                        if (auto ppq = pos->getPpqPosition()) {
                            syncActive = true; syncPpq0 = *ppq; ppqPerSample = *bpm / 60.0 / sr;
                            syncBeats = 4.0 / (double) (1 << (int) *prm.syncRate);
                            syncOff = *prm.syncOff * 0.01; syncPulse = *prm.syncPulse * 0.01;
                        }

    inSm.setTargetValue(dbToLin(*prm.inG));
    outSm.setTargetValue(dbToLin(*prm.outG));
    mixSm.setTargetValue(*prm.mix * 0.01f);
    autoOn = *prm.autoGain > 0.5f;
    if (!autoOn) { mkSm.setTargetValue(1.f); pIn = pOut = 0; }

    for (int s = 0; s < nS; s += maxBlock)
        processChunk(main, s, std::min(maxBlock, nS - s), nCh, scPtr, scCh);
}

void RajvikProcessor::processChunk(juce::AudioBuffer<float>& main, int start, int n, int nCh, const float* const* sc, int scCh)
{
    float* wp[2] = { wet.getWritePointer(0), wet.getWritePointer(1) };
    float* dp[2] = { dry.getWritePointer(0), dry.getWritePointer(1) };
    const bool mw = metersWanted.load(std::memory_order_relaxed);
    float inPk = 0.f, minG = 0.f, maxG = 0.f; double inSq = 0, outSq = 0, aIn = 0, aOut = 0;

    for (int i = 0; i < n; ++i) {
        const float ig = inSm.getNextValue();
        float x[2] = { 0, 0 }, src[2] = { 0, 0 }, det[2] = { 0, 0 }, g[2] = { 1, 1 }, d[2] = { 0, 0 }, w[2] = { 0, 0 };
        for (int c = 0; c < nCh; ++c) {
            x[c] = main.getReadPointer(c)[start + i] * ig;
            inPk = std::max(inPk, std::fabs(x[c]));
            src[c] = extSc ? sc[std::min(c, scCh - 1)][start + i] : x[c];
        }
        // sidechain chain: routing (Mid/Side) -> M/S matrix -> 6-band detector EQ -> level / invert
        if (routeMode == 1) src[0] = src[1] = (src[0] + src[1]) * 0.5f;
        else if (routeMode == 2) src[0] = src[1] = (src[0] - src[1]) * 0.5f;
        if (msOn) { const float m = (src[0] + src[1]) * 0.5f, s = (src[0] - src[1]) * 0.5f; src[0] = m; src[1] = s; }
        for (int c = 0; c < nCh; ++c) { det[c] = eqRun ? eq.process(c, src[c]) : src[c]; if (detGain != 1.f) det[c] *= detGain; }
        if (mw) {   // analyzer taps: input and post-EQ detector
            const int k2 = nCh > 1 ? 1 : 0; const unsigned ix = anWr & (AnalyzerFeed::N - 1);
            anIn.buf[ix].store((x[0] + x[k2]) * 0.5f, std::memory_order_relaxed); anDet.buf[ix].store((det[0] + det[k2]) * 0.5f, std::memory_order_relaxed); ++anWr;
        }
        if (syncActive) {   // host-tempo pulse replaces the detector: compression is triggered rhythmically
            double ph = std::fmod((syncPpq0 + (double) (start + i) * ppqPerSample) / syncBeats, 1.0) - syncOff;
            if (ph < 0) ph += 1.0;
            det[0] = det[1] = ph < syncPulse ? 1.f : 0.f;
        }
        for (int c = 0; c < nCh; ++c) d[c] = lookDelay[c].process(x[c], lookS);

        if (mbActive) {
            // ---- multiband: split (delayed audio) + split (detector) -> 1..4 compressors -> sum ----
            float xa[2] = { d[0], d[1] }, wm[2] = { 0, 0 }, lm[2] = { 0, 0 };
            if (msOn) { const float m = (xa[0] + xa[1]) * 0.5f, s = (xa[0] - xa[1]) * 0.5f; xa[0] = m; xa[1] = s; }
            mb.process(xa, det, nCh, mbShared, procCh, wm, lm);
            minG = std::min(minG, mb.grMin()); maxG = std::max(maxG, mb.grMax());
            if (listen) { if (msOn) { w[0] = lm[0] + lm[1]; w[1] = lm[0] - lm[1]; } else for (int c = 0; c < nCh; ++c) w[c] = lm[c]; }
            else if (msOn) { w[0] = wm[0] + wm[1]; w[1] = wm[0] - wm[1]; }
            else for (int c = 0; c < nCh; ++c) w[c] = wm[c];
        } else {
            engine.process(det, nCh, g);
            if (msOn) { if (!procCh[0]) g[0] = 1.f; if (!procCh[1]) g[1] = 1.f; }
            minG = std::min(minG, engine.getGainDb()); maxG = std::max(maxG, engine.getGainDb());
            if (listen) {
                if (msOn) { w[0] = det[0] + det[1]; w[1] = det[0] - det[1]; } else for (int c = 0; c < nCh; ++c) w[c] = det[c];
            } else if (msOn) {   // Mid/Side: gain[0] -> Mid, gain[1] -> Side
                const float m = (d[0] + d[1]) * 0.5f, s = (d[0] - d[1]) * 0.5f;
                w[0] = m * g[0] + s * g[1]; w[1] = m * g[0] - s * g[1];
            } else for (int c = 0; c < nCh; ++c) w[c] = d[c] * g[c];
        }
        for (int c = 0; c < nCh; ++c) {
            dp[c][i] = dryDelay[c].process(d[c], osLatNow); wp[c][i] = w[c];
            if (autoOn) { aIn += (double) d[c] * d[c]; aOut += (double) w[c] * w[c]; }
            if (mw) inSq += (double) x[c] * x[c];
        }
    }

    // loudness-matched auto gain: keep post-compression level equal to pre-compression level (1.5 s averaging)
    if (autoOn && !listen && n > 0) {
        const double a = std::exp(-(double) n / (sr * 1.5)), cnt = (double) n * nCh;
        pIn = a * pIn + (1.0 - a) * aIn / cnt; pOut = a * pOut + (1.0 - a) * aOut / cnt;
        if (pIn > 1e-10 && pOut > 1e-10)
            mkSm.setTargetValue(dbToLin(std::clamp((float) (10.0 * std::log10(pIn / pOut)), -12.f, 24.f)));
    }

    if (!listen) {
        if (chrIdx > 0) {
            if (osIdx >= 0) {
                float* chans[2] = { wp[0], wp[1] };
                juce::dsp::AudioBlock<float> blk(chans, (size_t) nCh, (size_t) n);
                auto up = os[(size_t) osIdx]->processSamplesUp(blk);
                for (int c = 0; c < nCh; ++c) sat.processShaper(up.getChannelPointer((size_t) c), (int) up.getNumSamples(), c);
                os[(size_t) osIdx]->processSamplesDown(blk);
            } else for (int c = 0; c < nCh; ++c) sat.processShaper(wp[c], n, c);
            for (int c = 0; c < nCh; ++c) sat.dcBlock(wp[c], n, c);
        } else if (osLatNow > 0) {
            for (int c = 0; c < nCh; ++c) for (int i = 0; i < n; ++i) wp[c][i] = wetDelay[c].process(wp[c][i], osLatNow);
        }
    }

    float outPk = 0.f;
    for (int i = 0; i < n; ++i) {
        const float m = listen ? 1.f : mixSm.getNextValue(), mk = listen ? 1.f : mkSm.getNextValue(), og = outSm.getNextValue();
        float ym = 0.f;
        for (int c = 0; c < nCh; ++c) {
            const float y = (dp[c][i] * (1.f - m) + wp[c][i] * mk * m) * og;
            main.getWritePointer(c)[start + i] = y; ym += y;
            outPk = std::max(outPk, std::fabs(y));
            if (mw) outSq += (double) y * y;
        }
        if (mw) { anOut.buf[anWrOut & (AnalyzerFeed::N - 1)].store(ym / (float) nCh, std::memory_order_relaxed); ++anWrOut; }
    }

    if (mw) {
        if (meterReset.exchange(false)) meter.reset();
        meter.process(main.getReadPointer(0) + start, nCh > 1 ? main.getReadPointer(1) + start : nullptr, n, nCh);
        mMom = meter.momentary(); mShort = meter.shortTerm(); mInt = meter.integrated(); mTp = meter.truePeakDb();
        const double cnt = (double) n * nCh;
        mRmsIn = linToDb((float) std::sqrt(inSq / cnt)); mRmsOut = linToDb((float) std::sqrt(outSq / cnt));
    }
    if (mw) { anIn.w.store(anWr, std::memory_order_release); anDet.w.store(anWr, std::memory_order_release); anOut.w.store(anWrOut, std::memory_order_release); }
    meters.push(inPk, outPk, minG < -0.01f ? minG : maxG);
    if (mbActive) for (int b = 0; b < 4; ++b) { mbGr[(size_t) b] = mb.grDb(b); mbLvl[(size_t) b] = linToDb(mb.level(b)); }
}

void RajvikProcessor::syncMidiMap()
{
    if (mapDirty.exchange(false)) {
        juce::String s;
        for (int cc = 0; cc < 128; ++cc) if (auto* p = ccMap[(size_t) cc].load()) s << cc << "=" << p->paramID << ";";
        midiMapStr = s; apvts.state.setProperty("midiMap", s, nullptr);
    } else if (auto stored = apvts.state.getProperty("midiMap").toString(); stored != midiMapStr) {
        midiMapStr = stored;
        for (auto& c : ccMap) c.store(nullptr);
        for (auto& item : juce::StringArray::fromTokens(stored, ";", ""))
            if (item.contains("=")) if (auto* p = apvts.getParameter(item.fromFirstOccurrenceOf("=", false, false)))
                ccMap[(size_t) juce::jlimit(0, 127, item.upToFirstOccurrenceOf("=", false, false).getIntValue())].store(p);
    }
}

void RajvikProcessor::applyModeDefaults()
{
    const auto& m = kModes[std::clamp((int) *prm.mode, 0, kNumModes - 1)];
    auto set = [&](const char* id, float v) { if (auto* p = apvts.getParameter(id)) p->setValueNotifyingHost(p->convertTo0to1(v)); };
    undo.beginNewTransaction("Mode defaults");
    set("threshold", m.defThr); set("ratio", m.defRatio); set("attack", m.defAtk); set("release", m.defRel); set("knee", m.defKnee);
}

void RajvikProcessor::toggleAB()
{
    abState[abCurrent] = apvts.copyState(); abCurrent ^= 1;
    if (abState[abCurrent].isValid()) apvts.replaceState(abState[abCurrent].createCopy());
}

void RajvikProcessor::getStateInformation(juce::MemoryBlock& dest)
{
    syncMidiMap();
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary(*xml, dest);
}
void RajvikProcessor::setStateInformation(const void* data, int size)
{
    if (auto xml = getXmlFromBinary(data, size))
        if (xml->hasTagName(apvts.state.getType())) { apvts.replaceState(juce::ValueTree::fromXml(*xml)); midiMapStr = "\x01"; syncMidiMap(); }
}
juce::AudioProcessorEditor* RajvikProcessor::createEditor() { return new RajvikEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new RajvikProcessor(); }
