#pragma once
#include "Common.h"
namespace rdp {
class SaturationProcessor {
public:
    void prepare() { reset(); }
    void reset() { for (int c = 0; c < 2; ++c) { lp[c] = 0; dcX[c] = 0; dcY[c] = 0; } }
    // type: 0 clean, 1 tube, 2 diode, 3 bright
    void setParams(int t, float driveDb) { type = t; k = dbToLin(driveDb); comp = 1.0f / std::sqrt(k); }
    void processShaper(float* d, int n, int c) noexcept {
        switch (type) {
            case 1: for (int i = 0; i < n; ++i) { const float v = k * d[i]; d[i] = std::tanh(v + 0.2f * v * v) * comp; } break;
            case 2: for (int i = 0; i < n; ++i) { const float v = k * d[i];
                        d[i] = (v >= 0.f ? 1.f - std::exp(-v) : -0.7f * (1.f - std::exp(0.9f * v))) * comp; } break;
            case 3: for (int i = 0; i < n; ++i) { const float v = k * d[i]; lp[c] += 0.25f * (v - lp[c]);
                        d[i] = std::tanh(v + 0.6f * (v - lp[c])) * comp; } break;
            default: break;
        }
    }
    // removes DC created by asymmetric curves (~35 Hz one-pole HP)
    void dcBlock(float* d, int n, int c) noexcept {
        for (int i = 0; i < n; ++i) {
            const float x = d[i], y = x - dcX[c] + 0.995f * dcY[c];
            dcX[c] = x; dcY[c] = flush(y); d[i] = y;
        }
    }
private:
    int type = 0; float k = 1, comp = 1; float lp[2]{}, dcX[2]{}, dcY[2]{};
};
} // namespace rdp
