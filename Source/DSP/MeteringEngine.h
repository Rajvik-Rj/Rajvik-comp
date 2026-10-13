#pragma once
#include "Common.h"
#include <array>
namespace rdp {
// BS.1770-4 / EBU R128 loudness (K-weighting, 400 ms momentary, 3 s short-term, gated integrated) + 4x true-peak estimate
class MeteringEngine {
public:
    void prepare(double sr) { fs = sr; blockLen = std::max(1, (int) (sr * 0.1)); design(); reset(); }
    void reset() {
        for (auto& c : st) c = {};
        for (auto& h : hist) h = {};
        ring.fill(0.0); ringPos = 0; blocks = 0; sub = 0; subCount = 0;
        for (auto& c : tpHist) c = {};
        tpMax = 0; mom = sho = integ = -100.f; histCount.fill(0); histEnergy.fill(0.0);
    }
    void process(const float* l, const float* r, int n, int nCh) noexcept {
        for (int i = 0; i < n; ++i) {
            const float* in[2] = { l, r };
            for (int c = 0; c < nCh; ++c) {
                const float x = in[c][i];
                float y = x;
                for (int s = 0; s < 2; ++s) {              // two K-weighting biquads (TDF2)
                    const auto& q = co[s]; auto& z = st[c][s];
                    const float o = q.b0 * y + z.z1; z.z1 = q.b1 * y - q.a1 * o + z.z2; z.z2 = q.b2 * y - q.a2 * o; y = o;
                }
                sub += (double) y * y;
                auto& h = tpHist[c];                          // true-peak: cubic (Catmull-Rom) interpolation, 3 extra points
                h[0] = h[1]; h[1] = h[2]; h[2] = h[3]; h[3] = x;
                tpMax = std::max(tpMax, std::fabs(x));
                for (int k = 1; k <= 3; ++k) {
                    const float t = 0.25f * (float) k;
                    const float v = 0.5f * (2 * h[1] + (-h[0] + h[2]) * t + (2 * h[0] - 5 * h[1] + 4 * h[2] - h[3]) * t * t + (-h[0] + 3 * h[1] - 3 * h[2] + h[3]) * t * t * t);
                    tpMax = std::max(tpMax, std::fabs(v));
                }
            }
            if (++subCount >= blockLen) { endBlock(); }
        }
    }
    float momentary() const { return mom; } float shortTerm() const { return sho; } float integrated() const { return integ; }
    float truePeakDb() const { return linToDb(tpMax); }
private:
    struct Co { float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0; }; struct Z { float z1 = 0, z2 = 0; };
    static float lufs(double e) { return (float) (-0.691 + 10.0 * std::log10(std::max(e, 1e-12))); }
    void design() {
        { const double f0 = 1681.974450955533, G = 3.999843853973347, Q = 0.7071752369554196, K = std::tan(3.14159265358979 * f0 / fs);
          const double Vh = std::pow(10.0, G / 20.0), Vb = std::pow(Vh, 0.4996667741545416), a0 = 1.0 + K / Q + K * K;
          co[0] = { (float) ((Vh + Vb * K / Q + K * K) / a0), (float) (2.0 * (K * K - Vh) / a0), (float) ((Vh - Vb * K / Q + K * K) / a0),
                    (float) (2.0 * (K * K - 1.0) / a0), (float) ((1.0 - K / Q + K * K) / a0) }; }
        { const double f0 = 38.13547087602444, Q = 0.5003270373238773, K = std::tan(3.14159265358979 * f0 / fs), a0 = 1.0 + K / Q + K * K;
          co[1] = { 1.f, -2.f, 1.f, (float) (2.0 * (K * K - 1.0) / a0), (float) ((1.0 - K / Q + K * K) / a0) }; }
    }
    void endBlock() {
        ring[(size_t) ringPos] = sub / blockLen; ringPos = (ringPos + 1) % 30; ++blocks; sub = 0; subCount = 0;
        auto avg = [&](int cnt) { double s = 0; for (int k = 1; k <= cnt; ++k) s += ring[(size_t) ((ringPos - k + 60) % 30)]; return s / cnt; };
        const int a = std::min(blocks, 4), b = std::min(blocks, 30);
        mom = lufs(avg(a)); sho = lufs(avg(b));
        if (blocks >= 4) {
            const double e4 = avg(4); const float l4 = lufs(e4);
            if (l4 > -70.f) {
                const int bin = std::clamp((int) ((l4 + 70.f) * 10.f), 0, 799);
                histCount[(size_t) bin]++; histEnergy[(size_t) bin] += e4;
            }
            double sum = 0; long cnt = 0;
            for (size_t i = 0; i < 800; ++i) { sum += histEnergy[i]; cnt += histCount[i]; }
            if (cnt > 0) {
                const float gate = lufs(sum / (double) cnt) - 10.f; double s2 = 0; long c2 = 0;
                for (size_t i = 0; i < 800; ++i) if (-70.f + 0.1f * (float) i >= gate) { s2 += histEnergy[i]; c2 += histCount[i]; }
                if (c2 > 0) integ = lufs(s2 / (double) c2);
            }
        }
    }
    double fs = 48000; int blockLen = 4800, ringPos = 0, blocks = 0, subCount = 0; double sub = 0;
    Co co[2]; std::array<std::array<Z, 2>, 2> st{}; std::array<std::array<float, 4>, 2> tpHist{}, hist{};
    std::array<double, 30> ring{}; std::array<long, 800> histCount{}; std::array<double, 800> histEnergy{};
    float tpMax = 0, mom = -100, sho = -100, integ = -100;
};
} // namespace rdp
