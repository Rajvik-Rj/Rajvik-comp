// Measures distortion added by the gain computer / ballistics: sine -> compressor -> harmonics (dB below the fundamental)
#include "../Source/DSP/CompressorEngine.h"
#include <cstdio>
#include <vector>
#include <complex>
using namespace rdp;
static double bin(const std::vector<float>& x, double f, double sr) {   // Hann-windowed single-bin DFT magnitude
    std::complex<double> s = 0; const int n = (int) x.size();
    for (int i = 0; i < n; ++i) { const double w = 0.5 - 0.5 * std::cos(6.283185307 * i / (n - 1)); s += w * x[i] * std::polar(1.0, -6.283185307 * f * i / sr); }
    return std::abs(s) / n;
}
double thd(int mode, double f0, float thr, float ratio, float atk, float rel, double sr = 48000) {
    CompressorEngine e; e.prepare(sr); EngineParams p; p.mode = mode; p.thrDb = thr; p.ratio = ratio; p.atkMs = atk; p.relMs = rel; e.setParams(p);
    const int N = (int) (sr * 4); std::vector<float> y; const float A = std::pow(10.f, -8.f / 20.f);
    for (int i = 0; i < N; ++i) { float x = A * (float) std::sin(6.283185307 * f0 * i / sr), g; e.process(&x, 1, &g); if (i > N / 2) y.push_back(x * g); }
    // snap the analysis window to an integer number of periods
    int per = (int) (sr / f0 + 0.5), cyc = (int) (y.size() / per); y.resize((size_t) cyc * per);
    const double fund = bin(y, f0, sr); double h = 0; for (int k = 2; k <= 9; ++k) { const double b = bin(y, f0 * k, sr); h += b * b; }
    return 20 * std::log10(std::sqrt(h) / fund + 1e-12);
}
int main() {
    printf("harmonic distortion in dB re fundamental (lower = cleaner). thr -30, ratio 4, atk 10, rel 100\n%-12s", "style");
    const double fr[] = { 40, 60, 100, 250, 1000 }; for (double f : fr) printf("%8.0fHz", f); printf("\n");
    for (int m = 0; m < kNumModes; ++m) { printf("%-12s", kModes[m].name); for (double f : fr) printf("%10.1f", thd(m, f, -30, 4, 10, 100)); printf("\n"); }
}
