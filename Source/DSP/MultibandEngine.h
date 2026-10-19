#pragma once
#include "CompressorEngine.h"
#include "Crossover.h"
namespace rdp {

struct BandSettings {
    float thr = -18, ratio = 4, atk = 10, rel = 120, gainDb = 0;
    bool solo = false, mute = false, bypass = false;
    int mode = 0;          // 0 = follow the main style, otherwise kModes index + 1
    bool ext = false;      // this band listens to the external side chain input (split into the same bands)
};

// Up to 4 independent compressors (one CompressorEngine per band) behind an LR4 crossover network.
// Style / knee / range / hold / detector / stereo-link / auto-release are shared; threshold, ratio, attack,
// release, make-up gain, solo, mute and bypass are per band.
class MultibandEngine {
public:
    static constexpr int kMax = BandSplitter::kMax;
    void prepare(double sr) {
        sampleRate = sr; audio.prepare(sr); det.prepare(sr); extSplit.prepare(sr);
        for (auto& e : eng) e.prepare(sr);
        smooth = 1.0f - tcCoef(8.0f, sr); lvlDecay = tcCoef(150.0f, sr);
        for (int b = 0; b < kMax; ++b) { mk[b] = 1.f; gate[b] = 1.f; }
        reset();
    }
    void reset() {
        audio.reset(); det.reset(); extSplit.reset();
        for (auto& e : eng) e.reset();
        for (int b = 0; b < kMax; ++b) { gr[b] = 0.f; lvl[b] = 0.f; }
    }
    // xo: crossover frequencies (n-1 values). They are forced into ascending order with a minimum spacing.
    void setup(int numBands, const float* xo, const EngineParams& base, const BandSettings* bs) {
        n = std::min(std::max(numBands, 2), kMax);
        float f[kMax - 1], prev = 20.f; const float top = (float) sampleRate * 0.4f;
        for (int j = 0; j < n - 1; ++j) { f[j] = std::min(std::max(xo[j], prev * 1.2f), top); prev = f[j]; }
        audio.setup(n, f); det.setup(n, f); extSplit.setup(n, f);
        anySolo = false;
        for (int b = 0; b < n; ++b) {
            set[b] = bs[b];
            EngineParams ep = base; ep.thrDb = bs[b].thr; ep.ratio = bs[b].ratio; ep.atkMs = bs[b].atk; ep.relMs = bs[b].rel; if (bs[b].mode > 0) ep.mode = bs[b].mode - 1;
            eng[b].setParams(ep);
            mkT[b] = dbToLin(bs[b].gainDb);
            anySolo = anySolo || bs[b].solo;
        }
        for (int b = 0; b < n; ++b) gateT[b] = set[b].mute ? 0.f : (anySolo ? (set[b].solo ? 1.f : 0.f) : 1.f);
    }
    void setMasterScale(float s) noexcept { scale = s; }   // master Mix: scales the gain change of every band (0 .. 2)
    int bands() const noexcept { return n; }
    // x: audio samples (already lookahead-delayed / M-S encoded); d: detector source (same encoding).
    // shared == true when d is identical to x (saves the second split). nCh = 1 or 2.
    // active[c] == false forces the gain of channel c to unity (used for "process Mid/Side only").
    // wet = summed compressed bands; listen = summed detector bands (gated by solo/mute like the audio).
    // e: external side chain samples in the same encoding (or nullptr). Bands with ext == true use e as their detector.
    void process(const float* x, const float* d, const float* e, int nCh, bool shared, const bool* active, float* wet, float* listen) noexcept {
        float A[2][kMax] = {}, D[2][kMax] = {}, E[2][kMax] = {};
        if (e != nullptr) for (int c = 0; c < nCh; ++c) extSplit.process(c, e[c], E[c]);
        for (int c = 0; c < nCh; ++c) { audio.process(c, x[c], A[c]); if (!shared) det.process(c, d[c], D[c]); }
        const float(*Dp)[kMax] = shared ? A : D;
        for (int c = 0; c < nCh; ++c) wet[c] = listen[c] = 0.f;
        for (int b = 0; b < n; ++b) {
            const float(*Sp)[kMax] = (set[b].ext && e != nullptr) ? E : Dp;
            float dd[2] = { Sp[0][b], nCh > 1 ? Sp[1][b] : 0.f }, g[2] = { 1.f, 1.f };
            if (set[b].ext && e == nullptr) dd[0] = dd[1] = 0.f;   // external selected but no side chain input connected: nothing triggers
            if (!set[b].bypass) { eng[b].process(dd, nCh, g); if (scale != 1.f) for (int c = 0; c < nCh; ++c) g[c] = dbToLin(linToDb(g[c]) * scale); gr[b] = eng[b].getGainDb() * scale; } else gr[b] = 0.f;
            if (nCh > 1) { if (!active[1]) g[1] = 1.f; }
            if (!active[0]) g[0] = 1.f;
            mk[b] += (mkT[b] - mk[b]) * smooth; gate[b] += (gateT[b] - gate[b]) * smooth;
            const float k = mk[b] * gate[b];
            float pk = 0.f;
            for (int c = 0; c < nCh; ++c) { wet[c] += A[c][b] * g[c] * k; listen[c] += Sp[c][b] * gate[b]; pk = std::max(pk, std::fabs(A[c][b])); }
            lvl[b] = std::max(pk, lvl[b] * lvlDecay);
        }
    }
    float grDb(int b) const noexcept { return gr[b]; }
    float level(int b) const noexcept { return lvl[b]; }
    float grMin() const noexcept { float m = 0.f; for (int b = 0; b < n; ++b) m = std::min(m, gr[b]); return m; }
    float grMax() const noexcept { float m = 0.f; for (int b = 0; b < n; ++b) m = std::max(m, gr[b]); return m; }
private:
    double sampleRate = 44100; int n = 4; bool anySolo = false;
    BandSplitter audio, det, extSplit; CompressorEngine eng[kMax]; BandSettings set[kMax];
    float mk[kMax] = {}, mkT[kMax] = {}, gate[kMax] = {}, gateT[kMax] = {}, gr[kMax] = {}, lvl[kMax] = {};
    float smooth = 0.01f, lvlDecay = 0.999f, scale = 1.f;
};
} // namespace rdp
