#pragma once
#include <algorithm>
#include <cmath>
#include <vector>
namespace rdp {
inline float dbToLin(float db) noexcept { return std::exp(db * 0.11512925f); }
inline float linToDb(float x) noexcept { return 20.0f * std::log10(std::max(x, 1.0e-6f)); }
inline float tcCoef(float ms, double sr) noexcept { return std::exp(-1.0f / (std::max(ms, 0.001f) * 0.001f * (float) sr)); }
inline float flush(float x) noexcept { return std::fabs(x) < 1.0e-20f ? 0.0f : x; }

// Integer delay line, allocation-free after prepare().
class DelayLine {
public:
    void prepare(int maxDelay) { buf.assign((size_t) maxDelay + 1, 0.0f); w = 0; }
    void reset() { std::fill(buf.begin(), buf.end(), 0.0f); w = 0; }
    float process(float in, int d) noexcept {
        const int size = (int) buf.size();
        d = std::min(std::max(d, 0), size - 1);
        buf[(size_t) w] = in;
        int r = w - d; if (r < 0) r += size;
        if (++w >= size) w = 0;
        return buf[(size_t) r];
    }
private:
    std::vector<float> buf{0.0f};
    int w = 0;
};
} // namespace rdp
