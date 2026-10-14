#pragma once
#include "Common.h"
namespace rdp {

// Linkwitz-Riley 4th order (two cascaded Butterworth biquads) crossover point, 2 channels.
// lo + hi sums to an all-pass (flat magnitude), so un-compressed bands recombine transparently.
class LR4 {
public:
    void set(float f, double sr) {
        f = std::min(std::max(f, 10.0f), (float) sr * 0.45f);
        const double w0 = 2.0 * 3.14159265358979323846 * f / sr, cw = std::cos(w0), sw = std::sin(w0), alpha = sw / (2.0 * 0.70710678118);
        const double a0 = 1.0 + alpha, a1 = -2.0 * cw / a0, a2 = (1.0 - alpha) / a0;
        lp = { (float) ((1.0 - cw) * 0.5 / a0), (float) ((1.0 - cw) / a0), (float) ((1.0 - cw) * 0.5 / a0), (float) a1, (float) a2 };
        hp = { (float) ((1.0 + cw) * 0.5 / a0), (float) (-(1.0 + cw) / a0), (float) ((1.0 + cw) * 0.5 / a0), (float) a1, (float) a2 };
    }
    void reset() { for (auto& c : st) for (auto& s : c) s = {}; }
    void split(int c, float x, float& lo, float& hi) noexcept {
        lo = run(st[c][1], lp, run(st[c][0], lp, x));
        hi = run(st[c][3], hp, run(st[c][2], hp, x));
    }
    float allpass(int c, float x) noexcept { float lo, hi; split(c, x, lo, hi); return lo + hi; }
private:
    struct Co { float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0; };
    struct St { float z1 = 0, z2 = 0; };
    static float run(St& s, const Co& q, float x) noexcept {
        const float y = q.b0 * x + s.z1;
        s.z1 = flush(q.b1 * x - q.a1 * y + s.z2);
        s.z2 = flush(q.b2 * x - q.a2 * y);
        return y;
    }
    Co lp, hp; St st[2][4];
};

// Splits a signal into 2..4 phase-aligned bands (3 crossovers max).
// Band k = LP_k(HP_{k-1}(...HP_0(x))). Lower bands are run through all-passes of the higher crossovers so the sum is flat.
class BandSplitter {
public:
    static constexpr int kMax = 4;
    void prepare(double sr) { sampleRate = sr; cachedN = 0; reset(); setup(n, freq); }
    void reset() { for (auto& x : xo) x.reset(); for (auto& r : comp) for (auto& x : r) x.reset(); }
    void setup(int bands, const float* f) {
        bands = std::min(std::max(bands, 2), kMax);
        bool same = (bands == cachedN);
        for (int j = 0; j < bands - 1; ++j) same = same && (f[j] == freq[j]);
        if (same) return;
        n = bands; cachedN = bands;
        for (int j = 0; j < n - 1; ++j) {
            freq[j] = f[j]; xo[j].set(f[j], sampleRate);
            for (int k = 0; k < j; ++k) comp[k][j].set(f[j], sampleRate);
        }
    }
    int bands() const noexcept { return n; }
    void process(int c, float x, float* out) noexcept {
        float rest = x;
        for (int k = 0; k < n - 1; ++k) { float lo, hi; xo[k].split(c, rest, lo, hi); out[k] = lo; rest = hi; }
        out[n - 1] = rest;
        for (int k = 0; k < n - 2; ++k) for (int j = k + 1; j <= n - 2; ++j) out[k] = comp[k][j].allpass(c, out[k]);
    }
private:
    double sampleRate = 44100; int n = 4, cachedN = 0; float freq[kMax - 1] = { 150.f, 1500.f, 6000.f };
    LR4 xo[kMax - 1]; LR4 comp[kMax - 1][kMax - 1];
};
} // namespace rdp
