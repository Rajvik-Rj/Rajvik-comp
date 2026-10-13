#pragma once
#include "Common.h"
namespace rdp {
enum class Kind { Downward, Upward, Combined };

// Positive result = dB of reduction above threshold (soft knee, quadratic interpolation)
inline float downwardGr(float L, float T, float R, float W) noexcept {
    const float over = L - T, slope = 1.0f - 1.0f / R;
    if (W < 1.0e-3f) return over > 0.0f ? over * slope : 0.0f;
    if (2.0f * over < -W) return 0.0f;
    if (2.0f * over > W) return over * slope;
    const float x = over + W * 0.5f;
    return slope * x * x / (2.0f * W);
}
// Positive result = dB of boost below threshold
inline float upwardGain(float L, float T, float R, float W) noexcept {
    return downwardGr(-L, -T, R, W);
}
// Static transfer: gain change in dB for input level L
inline float staticGainDb(Kind kind, float L, float thr, float R, float W, float range) noexcept {
    float t = 0.0f;
    if (kind != Kind::Upward) t -= std::min(downwardGr(L, thr, R, W), range);
    if (kind != Kind::Downward) {
        const float upThr = (kind == Kind::Combined) ? thr - 24.0f : thr;
        float u = std::min(upwardGain(L, upThr, std::min(R, 8.0f), W), range);
        if (L < -80.0f) u = 0.0f; else if (L < -70.0f) u *= (L + 80.0f) * 0.1f; // don't boost the noise floor
        t += u;
    }
    return t;
}
} // namespace rdp
