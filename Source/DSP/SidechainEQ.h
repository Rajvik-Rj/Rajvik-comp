#pragma once
#include "Common.h"
#include <array>
#include <complex>
namespace rdp {

enum EqType { EqHPF = 0, EqLowShelf, EqBell, EqHighShelf, EqLPF, EqAllPass };
inline constexpr int kEqBands = 6;

struct EqBand {
    bool on = true; int type = EqBell;
    float freq = 1000, gain = 0, q = 1;
    int slope = 1;                     // 0/1/2/3 = 12/24/48/96 dB per octave (HPF / LPF only; 96 = brickwall)
    int ms = 0;                        // 0 = stereo (L/R), 1 = mid only, 2 = side only
    bool operator==(const EqBand&) const = default;
};
struct SidechainEqParams {
    int numBands = kEqBands; bool enabled = true; EqBand band[kEqBands];
    bool operator==(const SidechainEqParams&) const = default;
};
struct BiquadCoef { float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0; };

inline bool eqIsCut(int t) noexcept { return t == EqHPF || t == EqLPF; }
inline bool eqHasGain(int t) noexcept { return t == EqLowShelf || t == EqBell || t == EqHighShelf; }
inline bool eqBandActive(const EqBand& b) noexcept {
    if (!b.on) return false;
    switch (b.type) { case EqHPF: return b.freq > 20.5f; case EqLPF: return b.freq < 19500.f; case EqAllPass: return true; default: return std::fabs(b.gain) > 0.05f; }
}
inline BiquadCoef eqDesign(int type, float f, float Q, float gdb, double sr) {
    f = std::min(f, (float) sr * 0.45f);
    const double w0 = 2.0 * 3.14159265358979 * f / sr, cw = std::cos(w0), sw = std::sin(w0);
    const double alpha = sw / (2.0 * Q), A = std::pow(10.0, gdb / 40.0), beta = 2.0 * std::sqrt(A) * alpha;
    double b0, b1, b2, a0, a1, a2;
    switch (type) {
        case EqHPF: b0 = (1 + cw) / 2; b1 = -(1 + cw); b2 = b0; a0 = 1 + alpha; a1 = -2 * cw; a2 = 1 - alpha; break;
        case EqLPF: b0 = (1 - cw) / 2; b1 = 1 - cw; b2 = b0; a0 = 1 + alpha; a1 = -2 * cw; a2 = 1 - alpha; break;
        case EqAllPass: b0 = 1 - alpha; b1 = -2 * cw; b2 = 1 + alpha; a0 = 1 + alpha; a1 = -2 * cw; a2 = 1 - alpha; break;
        case EqBell: b0 = 1 + alpha * A; b1 = -2 * cw; b2 = 1 - alpha * A; a0 = 1 + alpha / A; a1 = -2 * cw; a2 = 1 - alpha / A; break;
        case EqLowShelf:
            b0 = A * ((A + 1) - (A - 1) * cw + beta); b1 = 2 * A * ((A - 1) - (A + 1) * cw); b2 = A * ((A + 1) - (A - 1) * cw - beta);
            a0 = (A + 1) + (A - 1) * cw + beta; a1 = -2 * ((A - 1) + (A + 1) * cw); a2 = (A + 1) + (A - 1) * cw - beta; break;
        default:
            b0 = A * ((A + 1) + (A - 1) * cw + beta); b1 = -2 * A * ((A - 1) + (A + 1) * cw); b2 = A * ((A + 1) + (A - 1) * cw - beta);
            a0 = (A + 1) - (A - 1) * cw + beta; a1 = 2 * ((A - 1) - (A + 1) * cw); a2 = (A + 1) - (A - 1) * cw - beta; break;
    }
    return { (float) (b0 / a0), (float) (b1 / a0), (float) (b2 / a0), (float) (a1 / a0), (float) (a2 / a0) };
}
// Biquad stages for one band (HPF/LPF: 1, 2 or 4 Butterworth sections). Returns 0 if the band does nothing.
inline int eqBandStages(const EqBand& b, double sr, BiquadCoef* out) {
    if (!eqBandActive(b)) return 0;
    if (!eqIsCut(b.type)) { out[0] = eqDesign(b.type, b.freq, b.type == EqAllPass ? 0.707f : std::max(b.q, 0.1f), b.gain, sr); return 1; }
    static const float q1[] = { 0.7071f }, q2[] = { 0.5412f, 1.3066f }, q4[] = { 0.5098f, 0.6013f, 0.8999f, 2.5629f };
    if (b.slope >= 3) {   // 96 dB/oct "brickwall": two cascaded 8th-order Butterworth sections
        for (int i = 0; i < 8; ++i) out[i] = eqDesign(b.type, b.freq, q4[i & 3], 0.f, sr);
        return 8;
    }
    const int st = b.slope >= 2 ? 4 : (b.slope == 1 ? 2 : 1);
    const float* q = st == 4 ? q4 : (st == 2 ? q2 : q1);
    for (int i = 0; i < st; ++i) out[i] = eqDesign(b.type, b.freq, q[i], 0.f, sr);
    return st;
}
inline double biquadMagnitude(const BiquadCoef& q, double w) {
    const std::complex<double> z1 = std::polar(1.0, -w), z2 = std::polar(1.0, -2.0 * w);
    return std::abs((double) q.b0 + (double) q.b1 * z1 + (double) q.b2 * z2) / std::abs(1.0 + (double) q.a1 * z1 + (double) q.a2 * z2);
}
// Response of a single band in dB (0 when inactive) - used for the per-band curves in the GUI
inline float eqBandResponseDb(const EqBand& b, float f, double sr) {
    BiquadCoef c[8]; const int n = eqBandStages(b, sr, c);
    const double w = 2.0 * 3.14159265358979 * f / sr; double m = 1.0;
    for (int i = 0; i < n; ++i) m *= biquadMagnitude(c[i], w);
    return (float) (20.0 * std::log10(std::max(m, 1e-9)));
}

// 6-band detector EQ (HPF / shelves / bells / LPF). Affects the detector only. Each band owns 4 filter slots,
// so changing one band never disturbs the filter state of the others.
class SidechainEQ {
public:
    static constexpr int kSlots = 8;
    void prepare(double sr) { sampleRate = sr; cached = false; reset(); setParams(params); }
    void reset() { for (auto& c : z) for (auto& s : c) s = {}; }
    void setParams(const SidechainEqParams& p) {
        if (cached && p == params) return;
        params = p; cached = true; any = false;
        for (int b = 0; b < kEqBands; ++b) {
            nSt[b] = 0;
            if (p.enabled && b < p.numBands) nSt[b] = eqBandStages(p.band[b], sampleRate, &co[b * kSlots]);
            any = any || nSt[b] > 0;
        }
    }
    bool isActive() const noexcept { return any; }
    float runBand(int b, int c, float x) noexcept {
        for (int k = 0; k < nSt[b]; ++k) {
            const int i = b * kSlots + k; const auto& q = co[i]; auto& s = z[c][i];
            const float y = q.b0 * x + s.z1;
            s.z1 = flush(q.b1 * x - q.a1 * y + s.z2);
            s.z2 = flush(q.b2 * x - q.a2 * y);
            x = y;
        }
        return x;
    }
    float process(int c, float x) noexcept {   // mono / single channel
        for (int b = 0; b < kEqBands; ++b) if (nSt[b]) x = runBand(b, c, x);
        return x;
    }
    // Stereo pair with per-band channel routing. msIn = the pair already is (mid, side) instead of (left, right).
    void processStereo(float& a, float& b, bool msIn) noexcept {
        for (int k = 0; k < kEqBands; ++k) {
            if (!nSt[k]) continue;
            const int m = params.band[k].ms;
            if (m == 0) { a = runBand(k, 0, a); b = runBand(k, 1, b); }
            else if (msIn) { if (m == 1) a = runBand(k, 0, a); else b = runBand(k, 1, b); }
            else if (m == 1) { const float mid = (a + b) * 0.5f, d = runBand(k, 0, mid) - mid; a += d; b += d; }
            else { const float sd = (a - b) * 0.5f, d = runBand(k, 1, sd) - sd; a += d; b -= d; }
        }
    }
    float magnitudeDb(float f) const {
        const double w = 2.0 * 3.14159265358979 * f / sampleRate; double m = 1.0;
        for (int b = 0; b < kEqBands; ++b) for (int k = 0; k < nSt[b]; ++k) m *= biquadMagnitude(co[b * kSlots + k], w);
        return (float) (20.0 * std::log10(std::max(m, 1e-9)));
    }
private:
    struct St { float z1 = 0, z2 = 0; };
    SidechainEqParams params; bool cached = false, any = false; double sampleRate = 44100;
    std::array<BiquadCoef, kEqBands * kSlots> co{}; std::array<std::array<St, kEqBands * kSlots>, 2> z{}; int nSt[kEqBands] = {};
};
} // namespace rdp
