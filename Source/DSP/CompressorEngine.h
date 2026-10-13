#pragma once
#include "CompressionAlgorithms.h"
namespace rdp {
struct EngineParams {
    float thrDb = -18, ratio = 4, atkMs = 10, relMs = 120, kneeDb = 6, rangeDb = 40, holdMs = 0, link = 1;
    int mode = 0;          // index into kModes
    int detector = 0;      // 0 = mode default, 1 = peak, 2 = RMS
    bool autoRelease = false;
    bool operator==(const EngineParams&) const = default;
};

class CompressorEngine {
public:
    void prepare(double sr) { sampleRate = sr; cachedSr = 0; reset(); recompute(); }
    void reset() { for (auto& c : ch) c = ChState{}; lastGain = 0; }
    void setParams(const EngineParams& p) {
        if (p == params && cachedSr == sampleRate) return;
        params = p; recompute();
    }
    // det: detector samples (already sidechain-filtered). out: linear gain per channel.
    void process(const float* det, int n, float* out) noexcept {
        float lvl[2] = {-120.f, -120.f};
        for (int c = 0; c < n; ++c) {
            auto& s = ch[c];
            float d;
            if (useRms) { s.rms = rmsCoef * s.rms + (1.f - rmsCoef) * det[c] * det[c]; if (s.rms < 1e-18f) s.rms = 0; d = std::sqrt(s.rms); }
            else d = std::fabs(det[c]);
            float l = linToDb(d);
            if (prof->feedback) l += s.g;
            lvl[c] = l;
        }
        float g[2];
        if (n == 2 && params.link >= 0.999f) {
            g[0] = g[1] = step(ch[0], std::max(lvl[0], lvl[1]));
            ch[1] = ch[0];
        } else if (n == 2) {
            const float mx = std::max(lvl[0], lvl[1]);
            for (int c = 0; c < 2; ++c) g[c] = step(ch[c], lvl[c] + params.link * (mx - lvl[c]));
        } else g[0] = step(ch[0], lvl[0]);
        lastGain = g[0]; out[0] = dbToLin(g[0]);
        if (n == 2) { lastGain = std::min(g[0], g[1]); out[1] = dbToLin(g[1]); }
    }
    float getGainDb() const noexcept { return lastGain; }   // negative = reduction
    const ModeProfile& profile() const noexcept { return *prof; }
    float effectiveKnee() const noexcept { return knee; }
private:
    struct ChState { float rms = 0, g = 0, avg = 0, relC = 0; int hold = 0, cnt = 0; };
    float step(ChState& s, float L) noexcept {
        float R = params.ratio;
        if (prof->variMu) R = 1.f + (R - 1.f) * (0.35f + 0.65f * std::clamp(-s.g / 18.f, 0.f, 1.f));
        const float t = staticGainDb(prof->kind, L, params.thrDb, R, knee, params.rangeDb);
        if (t < s.g) { s.g = aC * s.g + (1.f - aC) * t; s.hold = holdSamp; }
        else if (s.hold > 0) --s.hold;
        else if (prof->slewRelease) s.g = std::min(s.g + slewStep, t);
        else {
            if (progDep > 0.f) {
                if ((s.cnt++ & 15) == 0) s.relC = tcCoef(relBase * (0.5f + progDep * std::clamp(s.avg / 12.f, 0.f, 1.f) * 2.f), sampleRate);
                s.g = s.relC * s.g + (1.f - s.relC) * t;
            } else s.g = relC * s.g + (1.f - relC) * t;
        }
        s.avg = avgC * s.avg + (1.f - avgC) * std::max(-s.g, 0.f);
        s.g = flush(s.g);
        return s.g;
    }
    void recompute() {
        cachedSr = sampleRate;
        prof = &kModes[std::clamp(params.mode, 0, kNumModes - 1)];
        aC = tcCoef(params.atkMs * prof->attackMul, sampleRate);
        relBase = params.relMs * prof->releaseMul;
        relC = tcCoef(relBase, sampleRate);
        float rmsMs = prof->rmsMs;
        if (params.detector == 1) rmsMs = 0; else if (params.detector == 2 && rmsMs <= 0) rmsMs = 10;
        useRms = rmsMs > 0; rmsCoef = useRms ? tcCoef(rmsMs, sampleRate) : 0.f;
        knee = std::clamp(params.kneeDb + prof->kneeAdd, 0.f, 72.f);
        holdSamp = (int) ((params.holdMs + prof->holdMs) * 0.001 * sampleRate);
        progDep = std::max(prof->progDep, params.autoRelease ? 1.0f : 0.0f);
        avgC = tcCoef(250.f, sampleRate);
        slewStep = 24.f / std::max(1.f, relBase * 0.001f * (float) sampleRate);
    }
    EngineParams params; const ModeProfile* prof = &kModes[0];
    double sampleRate = 44100, cachedSr = 0;
    ChState ch[2];
    float aC = 0, relC = 0, relBase = 100, rmsCoef = 0, knee = 6, progDep = 0, avgC = 0, slewStep = 0, lastGain = 0;
    int holdSamp = 0; bool useRms = false;
};
} // namespace rdp
