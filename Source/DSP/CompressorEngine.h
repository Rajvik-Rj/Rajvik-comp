#pragma once
#include "CompressionAlgorithms.h"
namespace rdp {
struct EngineParams {
    float thrDb = -18, ratio = 4, atkMs = 10, relMs = 120, kneeDb = 6, rangeDb = 40, holdMs = 0, link = 1;
    int mode = 0;          // index into kModes
    int detector = 0;      // 0 = mode default, 1 = peak, 2 = RMS
    bool autoRelease = false;
    float clamp = 0;       // 0..1: how much of the transient that slipped through the attack stage is pressed down again
    bool operator==(const EngineParams&) const = default;
};

// Level detector -> static gain computer (soft knee) -> ballistics.
//
// Ballistics work on the "demand" d = -(gain in dB) (positive = reduction, negative = boost, so downward, upward and
// combined styles share one code path):
//   stage 1  peak-hold + release (fast stage, optional slow stage blended by sustained GR, optional hold / constant slope)
//   stage 2  attack smoothing, applied only while the demand is rising
// Because the peak is held *before* the attack smoothing the gain never follows the rectified waveform, so there is no
// low-frequency ripple / modulation distortion, and release is never smeared by a long attack.
class CompressorEngine {
public:
    void prepare(double sr) { sampleRate = sr; cachedSr = 0; reset(); recompute(); }
    void reset() { for (auto& c : det) c = Det{}; for (auto& g : gn) g = Gn{}; lastGain = 0; }
    void setParams(const EngineParams& p) {
        if (p == params && cachedSr == sampleRate) return;
        params = p; recompute();
    }
    // d: detector samples (already sidechain-filtered). out: linear gain per channel.
    void process(const float* in, int n, float* out) noexcept {
        float lvl[2] = { -120.f, -120.f };
        for (int c = 0; c < n; ++c) {
            auto& s = det[c];
            const float x = std::isfinite(in[c]) ? in[c] : 0.f;
            const float a = std::fabs(x);
            // Ripple guard: hold the peak for ~a half period of the lowest note (<= 25 ms) before it may fall, so the detector
            // does not follow the rectified waveform of bass notes (that ripple is what turns into low-frequency distortion).
            if (guardSamp == 0) s.hv = a;
            else if (a >= s.hv) { s.hv = a; s.hc = guardSamp; } else if (s.hc > 0) --s.hc; else s.hv = std::max(a, s.hv * guardDecay);
            float amp = s.hv;
            if (useRms) {
                s.pw = flush(rmsCoef * s.pw + (1.f - rmsCoef) * x * x);
                amp = rmsMix * std::sqrt(2.f * s.pw) + (1.f - rmsMix) * s.hv;   // sqrt(2 * mean-square): a sine reads its peak value
            }
            if (prof->adaptive) {   // crest-factor tracking (fast peak envelope vs slow RMS)
                s.pk = std::max(a, pkCoef * s.pk); s.cp = flush(crestCoef * s.cp + (1.f - crestCoef) * x * x);
            }
            float l = linToDb(amp);
            if (prof->feedback) l -= gn[c].y;                  // detector sees the already-processed level
            lvl[c] = l;
        }
        float g[2];
        if (n == 2 && params.link >= 0.999f) {
            const float l = std::max(lvl[0], lvl[1]);
            g[0] = g[1] = step(gn[0], l, adaptScale(det[0], det[1]));
            gn[1] = gn[0];                                       // identical gain state; detector state stays per channel
        } else if (n == 2) {
            const float mx = std::max(lvl[0], lvl[1]);
            for (int c = 0; c < 2; ++c) g[c] = step(gn[c], lvl[c] + params.link * (mx - lvl[c]), adaptScale(det[c], det[c]));
        } else g[0] = step(gn[0], lvl[0], adaptScale(det[0], det[0]));
        lastGain = g[0]; out[0] = dbToLin(g[0]);
        if (n == 2) { lastGain = std::min(g[0], g[1]); out[1] = dbToLin(g[1]); }
    }
    float getGainDb() const noexcept { return lastGain; }   // negative = reduction
    const ModeProfile& profile() const noexcept { return *prof; }
    float effectiveKnee() const noexcept { return knee; }
private:
    struct Det { float pw = 0, pk = 0, cp = 0, hv = 0; int hc = 0; };
    struct Gn  { float y1f = 0, y1s = 0, y = 0, avg = 0; int hold = 0; };

    // 1.0 = nominal. Versatile: transient material -> faster, dense material -> slower.
    float adaptScale(const Det& a, const Det& b) const noexcept {
        if (!prof->adaptive) return 1.f;
        const float pk = std::max(a.pk, b.pk), rms = std::sqrt(2.f * std::max(a.cp, b.cp));
        const float crest = std::clamp(linToDb(pk) - linToDb(rms), 0.f, 24.f);
        return std::clamp(1.55f - crest / 13.f, 0.45f, 1.55f);
    }
    float step(Gn& s, float L, float ad) noexcept {
        float R = params.ratio;
        if (prof->variMu) R = 1.f + (R - 1.f) * (0.35f + 0.65f * std::clamp(s.y / 18.f, 0.f, 1.f));
        const float d = -staticGainDb(prof->kind, L, params.thrDb, R, knee, params.rangeDb);

        const bool rising = d >= s.y1f;
        if (rising) s.hold = holdSamp;
        const bool holding = !rising && s.hold > 0;
        if (holding) --s.hold;

        const float rcF = ad == 1.f ? relF : tcCoef(relBaseF * ad, sampleRate);
        auto stage = [&](float& y1, float rc) noexcept {
            if (d >= y1) y1 = d;
            else if (holding) {}
            else if (prof->slewRelease) y1 = std::max(d, y1 - slewStep);
            else y1 = rc * y1 + (1.f - rc) * d;
        };
        stage(s.y1f, rcF);
        float y1 = s.y1f;
        if (twoStage) {
            stage(s.y1s, ad == 1.f ? relS : tcCoef(relBaseS * ad, sampleRate));
            const float w = slowAmt * std::clamp(s.avg / 8.f, 0.f, 1.f);   // more slow-stage as compression is sustained
            y1 += (s.y1s - s.y1f) * w;
        }
        if (y1 > s.y) { const float aC = ad == 1.f ? atkC : tcCoef(atkBase * ad, sampleRate); s.y = aC * s.y + (1.f - aC) * y1; }
        else s.y = y1;

        s.avg = avgC * s.avg + (1.f - avgC) * std::fabs(s.y);
        s.y = flush(s.y); s.y1f = flush(s.y1f); s.y1s = flush(s.y1s); s.avg = flush(s.avg);
        float out = s.y;
        if (params.clamp > 0.f && y1 > out) out += params.clamp * (y1 - out);          // Clamp: press down what the attack stage let through
        return -out;
    }
    void recompute() {
        cachedSr = sampleRate;
        prof = &kModes[std::clamp(params.mode, 0, kNumModes - 1)];
        atkBase = std::max(params.atkMs * prof->attackMul, 0.02f);   // floor: sub-20 us attacks only add distortion
        atkC = tcCoef(atkBase, sampleRate);
        relBaseF = params.relMs * prof->releaseMul; relF = tcCoef(relBaseF, sampleRate);
        float sm = prof->slowMul, sa = prof->slowAmt;
        if (params.autoRelease) { sm = std::max(sm, 4.f); sa = std::max(sa, 0.8f); }
        twoStage = sm > 0.f && sa > 0.f && !prof->slewRelease; slowAmt = sa;
        relBaseS = std::max(params.relMs * sm, relBaseF); relS = tcCoef(relBaseS, sampleRate);
        float rmsMs = prof->rmsMs; rmsMix = prof->rmsMix;
        if (params.detector == 1) rmsMs = 0; else if (params.detector == 2 && rmsMs <= 0) { rmsMs = 10; rmsMix = 1.f; }
        if (rmsMs > 0.f) rmsMs = std::max(rmsMs, 12.f);   // shorter windows follow the ripple of bass notes
        useRms = rmsMs > 0.f; rmsCoef = useRms ? tcCoef(rmsMs, sampleRate) : 0.f;
        knee = std::clamp(params.kneeDb + prof->kneeAdd, 0.f, 72.f);
        holdSamp = (int) ((params.holdMs + prof->holdMs) * 0.001 * sampleRate);
        avgC = tcCoef(prof->slowAmt > 0.9f ? 600.f : 350.f, sampleRate);
        guardSamp = prof->slewRelease ? 0 : (int) (std::clamp(relBaseF * 0.25f, 3.f, 25.f) * 0.001 * sampleRate);   // slew styles already hold via their Hold time guardDecay = tcCoef(2.5f, sampleRate);
        pkCoef = tcCoef(60.f, sampleRate); crestCoef = tcCoef(150.f, sampleRate);
        slewStep = 24.f / std::max(1.f, relBaseF * 0.001f * (float) sampleRate);
    }
    EngineParams params; const ModeProfile* prof = &kModes[0];
    double sampleRate = 44100, cachedSr = 0;
    Det det[2]; Gn gn[2];
    float atkBase = 10, atkC = 0, relBaseF = 100, relBaseS = 300, relF = 0, relS = 0, slowAmt = 0, rmsCoef = 0, rmsMix = 0, knee = 6, avgC = 0, slewStep = 0,
          pkCoef = 0, crestCoef = 0, lastGain = 0;
    int holdSamp = 0, guardSamp = 0; float guardDecay = 0; bool useRms = false, twoStage = false;
};
} // namespace rdp
