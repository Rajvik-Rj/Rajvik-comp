#pragma once
#include "Common.h"
#include <array>
#include <complex>
namespace rdp {
struct SidechainEqParams {
    float hpfHz = 20; int hpfStages = 1;          // 1/2/4 stages = 12/24/48 dB/oct
    float lpfHz = 20000;                           // 12 dB/oct
    float lowHz = 100, lowDb = 0;                  // low shelf
    float bellHz = 1000, bellDb = 0, bellQ = 1;    // bell
    float highHz = 6000, highDb = 0;               // high shelf
    bool operator==(const SidechainEqParams&) const = default;
};

class SidechainEQ {
public:
    void prepare(double sr) { sampleRate = sr; cached = false; reset(); setParams(params); }
    void reset() { for (auto& c : z) for (auto& s : c) s = {0, 0}; }
    void setParams(const SidechainEqParams& p) {
        if (cached && p == params) return;
        params = p; cached = true; n = 0;
        static const float q1[] = {0.7071f}, q2[] = {0.5412f, 1.3066f}, q4[] = {0.5098f, 0.6013f, 0.8999f, 2.5629f};
        if (p.hpfHz > 20.5f) {
            const int st = p.hpfStages >= 4 ? 4 : (p.hpfStages >= 2 ? 2 : 1);
            const float* q = st == 4 ? q4 : (st == 2 ? q2 : q1);
            for (int i = 0; i < st; ++i) add(Type::HPF, p.hpfHz, q[i], 0);
        }
        if (p.lpfHz < 19500.f) add(Type::LPF, p.lpfHz, 0.7071f, 0);
        if (std::fabs(p.lowDb) > 0.05f)  add(Type::LowShelf, p.lowHz, 0.7071f, p.lowDb);
        if (std::fabs(p.bellDb) > 0.05f) add(Type::Peak, p.bellHz, p.bellQ, p.bellDb);
        if (std::fabs(p.highDb) > 0.05f) add(Type::HighShelf, p.highHz, 0.7071f, p.highDb);
    }
    bool isActive() const noexcept { return n > 0; }
    float process(int c, float x) noexcept {
        for (int k = 0; k < n; ++k) {
            const auto& q = co[k]; auto& s = z[c][k];
            const float y = q.b0 * x + s.z1;
            s.z1 = q.b1 * x - q.a1 * y + s.z2;
            s.z2 = q.b2 * x - q.a2 * y;
            x = y;
        }
        return x;
    }
    float magnitudeDb(float f) const {
        const double w = 2.0 * 3.14159265358979 * f / sampleRate;
        const std::complex<double> z1 = std::polar(1.0, -w), z2 = std::polar(1.0, -2.0 * w);
        double mag = 1.0;
        for (int k = 0; k < n; ++k) {
            const auto& q = co[k];
            mag *= std::abs((double) q.b0 + (double) q.b1 * z1 + (double) q.b2 * z2) / std::abs(1.0 + (double) q.a1 * z1 + (double) q.a2 * z2);
        }
        return (float) (20.0 * std::log10(std::max(mag, 1e-9)));
    }
private:
    enum class Type { HPF, LPF, Peak, LowShelf, HighShelf };
    struct Co { float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0; };
    struct St { float z1 = 0, z2 = 0; };
    void add(Type t, float f, float Q, float gdb) {
        if (n >= 8) return;
        f = std::min(f, (float) sampleRate * 0.45f);
        const double w0 = 2.0 * 3.14159265358979 * f / sampleRate, cw = std::cos(w0), sw = std::sin(w0);
        const double alpha = sw / (2.0 * Q), A = std::pow(10.0, gdb / 40.0), beta = 2.0 * std::sqrt(A) * alpha;
        double b0, b1, b2, a0, a1, a2;
        switch (t) {
            case Type::HPF: b0 = (1 + cw) / 2; b1 = -(1 + cw); b2 = b0; a0 = 1 + alpha; a1 = -2 * cw; a2 = 1 - alpha; break;
            case Type::LPF: b0 = (1 - cw) / 2; b1 = 1 - cw; b2 = b0; a0 = 1 + alpha; a1 = -2 * cw; a2 = 1 - alpha; break;
            case Type::Peak: b0 = 1 + alpha * A; b1 = -2 * cw; b2 = 1 - alpha * A; a0 = 1 + alpha / A; a1 = -2 * cw; a2 = 1 - alpha / A; break;
            case Type::LowShelf:
                b0 = A * ((A + 1) - (A - 1) * cw + beta); b1 = 2 * A * ((A - 1) - (A + 1) * cw); b2 = A * ((A + 1) - (A - 1) * cw - beta);
                a0 = (A + 1) + (A - 1) * cw + beta; a1 = -2 * ((A - 1) + (A + 1) * cw); a2 = (A + 1) + (A - 1) * cw - beta; break;
            default:
                b0 = A * ((A + 1) + (A - 1) * cw + beta); b1 = -2 * A * ((A - 1) + (A + 1) * cw); b2 = A * ((A + 1) + (A - 1) * cw - beta);
                a0 = (A + 1) - (A - 1) * cw + beta; a1 = 2 * ((A - 1) - (A + 1) * cw); a2 = (A + 1) - (A - 1) * cw - beta; break;
        }
        co[n++] = { (float) (b0 / a0), (float) (b1 / a0), (float) (b2 / a0), (float) (a1 / a0), (float) (a2 / a0) };
    }
    SidechainEqParams params; bool cached = false; double sampleRate = 44100;
    std::array<Co, 8> co{}; std::array<std::array<St, 8>, 2> z{}; int n = 0;
};
} // namespace rdp
