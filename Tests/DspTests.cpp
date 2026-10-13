// JUCE-free DSP unit tests.  Build: cmake --build build --target DspTests && ctest
#include "../Source/DSP/CompressorEngine.h"
#include "../Source/DSP/SidechainEQ.h"
#include "../Source/DSP/SaturationProcessor.h"
#include "../Source/DSP/MeteringEngine.h"
#include <cstdio>
using namespace rdp;
static int fails = 0;
#define CHECK(c, msg) do { if (!(c)) { std::printf("FAIL: %s\n", msg); ++fails; } else std::printf("ok:   %s\n", msg); } while (0)
static float runConst(CompressorEngine& e, float amp, int n) { float g[2], d[2] = {amp, amp}; for (int i = 0; i < n; ++i) e.process(d, 2, g); return e.getGainDb(); }

int main() {
    const double sr = 48000;
    { CompressorEngine e; EngineParams p; p.mode = 0; e.setParams(p); e.prepare(sr);
      float g[2], z[2] = {0, 0}; bool ok = true;
      for (int i = 0; i < 48000; ++i) { e.process(z, 2, g); if (!std::isfinite(g[0]) || std::fabs(g[0] - 1.f) > 1e-4f) ok = false; }
      CHECK(ok, "silence -> unity gain, no NaN"); }
    { CompressorEngine e; EngineParams p; p.thrDb = -20; p.ratio = 4; p.kneeDb = 0; p.atkMs = 1; p.mode = 0; e.setParams(p); e.prepare(sr);
      const float gr = runConst(e, 1.0f, 48000);
      CHECK(std::fabs(gr + 15.f) < 3.2f, "threshold -20, ratio 4:1, 0 dBFS in -> ~12-15 dB GR (mode knee adds softness)"); }
    { CompressorEngine e; EngineParams p; p.thrDb = -20; p.ratio = 100; p.kneeDb = 0; p.atkMs = 0.1f; p.mode = 0; e.setParams(p); e.prepare(sr);
      const float gr = runConst(e, 1.0f, 48000);
      CHECK(gr < -19.0f, "100:1 behaves like a limiter"); }
    { CompressorEngine e; EngineParams p; p.thrDb = -20; p.ratio = 4; p.kneeDb = 0; p.atkMs = 10; p.relMs = 100; p.mode = 0; e.setParams(p); e.prepare(sr);
      float g[2], d[2] = {1, 1}; int t50 = -1;
      for (int i = 0; i < 48000 && t50 < 0; ++i) { e.process(d, 2, g); if (e.getGainDb() < -6.0f) t50 = i; }
      const float ms = t50 * 1000.f / (float) sr;
      CHECK(ms > 2.f && ms < 30.f, "attack reaches ~50% of GR within expected time");
      runConst(e, 1.0f, 48000); int t = -1; d[0] = d[1] = 0;
      for (int i = 0; i < 96000 && t < 0; ++i) { e.process(d, 2, g); if (e.getGainDb() > -6.0f) t = i; }
      const float rms = t * 1000.f / (float) sr;
      CHECK(rms > 20.f && rms < 300.f, "release recovers ~50% within expected time"); }
    { CHECK(downwardGr(-26, -20, 4, 12) == 0.f && std::fabs(downwardGr(-8, -20, 4, 12) - 9.f) < 0.01f &&
            downwardGr(-20, -20, 4, 12) > 0.f && downwardGr(-20, -20, 4, 12) < 1.5f, "knee transitions (below/inside/above)"); }
    { CompressorEngine e; EngineParams p; p.mode = 10; p.thrDb = -40; p.ratio = 2; e.setParams(p); e.prepare(sr);
      const float g = runConst(e, 0.001f, 48000);
      CHECK(std::fabs(g - 10.f) < 0.5f, "upward compression boosts -60 dB by ~10 dB"); }
    { CompressorEngine e; EngineParams p; p.thrDb = -30; p.ratio = 4; p.link = 1; e.setParams(p); e.prepare(sr);
      float g[2], d[2] = {1.f, 0.01f}; for (int i = 0; i < 48000; ++i) e.process(d, 2, g);
      CHECK(std::fabs(g[0] - g[1]) < 1e-4f, "100% stereo link -> equal gains"); }
    { SidechainEQ eq; SidechainEqParams p; p.hpfHz = 1000; p.hpfStages = 2; eq.prepare(sr); eq.setParams(p);
      float lo = 0, hi = 0;
      for (int i = 0; i < 48000; ++i) { lo = std::max(lo, std::fabs(eq.process(0, std::sin(6.2831853f * 100.f * i / 48000.f)))); }
      eq.reset();
      for (int i = 0; i < 48000; ++i) { hi = std::max(hi, std::fabs(eq.process(0, std::sin(6.2831853f * 8000.f * i / 48000.f)))); }
      CHECK(linToDb(lo) < -30.f && linToDb(hi) > -1.f, "sidechain HPF 24 dB/oct attenuates 100 Hz, passes 8 kHz");
      CHECK(eq.magnitudeDb(100.f) < -30.f, "EQ magnitude response matches"); }
    { SaturationProcessor s; s.prepare(); s.setParams(1, 12.f);
      std::vector<float> b(48000); for (size_t i = 0; i < b.size(); ++i) b[i] = 0.5f * std::sin(6.2831853f * 220.f * i / 48000.f);
      s.processShaper(b.data(), (int) b.size(), 0); s.dcBlock(b.data(), (int) b.size(), 0);
      double m = 0; for (size_t i = 24000; i < b.size(); ++i) m += b[i]; m /= 24000.0;
      CHECK(std::fabs(m) < 2e-3, "tube saturation: DC offset removed"); }
    { MeteringEngine m; m.prepare(sr); const int N = 48000 * 12; std::vector<float> b(N);
      const float a = std::pow(10.f, -23.f / 20.f);
      for (int i = 0; i < N; ++i) b[(size_t) i] = a * std::sin(6.2831853f * 1000.f * i / 48000.f);
      m.process(b.data(), b.data(), N, 2);
      CHECK(std::fabs(m.integrated() + 23.f) < 0.4f && std::fabs(m.momentary() + 23.f) < 0.4f, "BS.1770: -23 dBFS 1 kHz stereo sine reads ~-23 LUFS");
      CHECK(m.truePeakDb() >= -23.1f && m.truePeakDb() < -22.5f, "true-peak estimate close to sine peak"); }
    std::printf(fails ? "\n%d FAILED\n" : "\nALL PASSED\n", fails);
    return fails ? 1 : 0;
}
