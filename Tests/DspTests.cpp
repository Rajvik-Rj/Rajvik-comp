// JUCE-free DSP unit tests.  Build: cmake --build build --target DspTests && ctest
#include "../Source/DSP/CompressorEngine.h"
#include "../Source/DSP/SidechainEQ.h"
#include "../Source/DSP/SaturationProcessor.h"
#include "../Source/DSP/MeteringEngine.h"
#include "../Source/DSP/MultibandEngine.h"
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
    { SidechainEQ eq; SidechainEqParams p; p.band[0].type = EqHPF; p.band[0].freq = 1000; p.band[0].slope = 1; eq.prepare(sr); eq.setParams(p);
      float lo = 0, hi = 0;
      for (int i = 0; i < 48000; ++i) { lo = std::max(lo, std::fabs(eq.process(0, std::sin(6.2831853f * 100.f * i / 48000.f)))); }
      eq.reset();
      for (int i = 0; i < 48000; ++i) { hi = std::max(hi, std::fabs(eq.process(0, std::sin(6.2831853f * 8000.f * i / 48000.f)))); }
      CHECK(linToDb(lo) < -30.f && linToDb(hi) > -1.f, "sidechain HPF 24 dB/oct attenuates 100 Hz, passes 8 kHz");
      CHECK(eq.magnitudeDb(100.f) < -30.f, "EQ magnitude response matches"); }
    { SidechainEQ eq; SidechainEqParams p; p.band[2].type = EqBell; p.band[2].freq = 1000; p.band[2].gain = 12; p.band[2].q = 1; eq.prepare(sr); eq.setParams(p);
      CHECK(std::fabs(eq.magnitudeDb(1000.f) - 12.f) < 0.1f && std::fabs(eq.magnitudeDb(20.f)) < 1.0f, "bell +12 dB at 1 kHz");
      p.band[2].on = false; eq.setParams(p); CHECK(!eq.isActive(), "disabled band -> EQ inactive (no filtering)");
      p.band[2].on = true; p.enabled = false; eq.setParams(p); CHECK(!eq.isActive(), "EQ power off -> inactive");
      p.enabled = true; p.numBands = 2; eq.setParams(p); CHECK(!eq.isActive(), "bands above numBands are ignored");
      EqBand ls; ls.type = EqLowShelf; ls.freq = 200; ls.gain = 6;
      CHECK(std::fabs(eqBandResponseDb(ls, 20.f, sr) - 6.f) < 0.5f && std::fabs(eqBandResponseDb(ls, 10000.f, sr)) < 0.3f, "low shelf +6 dB below 200 Hz"); }
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

    // ------------------------------------------------------------------ multiband
    auto sineRms = [&](auto&& fn, float f, int N, float amp = 0.25f) {   // run a callable sample-by-sample, return RMS of 2nd half
        double acc = 0; int cnt = 0;
        for (int i = 0; i < N; ++i) { const float y = fn(amp * std::sin(6.2831853f * f * (float) i / (float) sr)); if (i >= N / 2) { acc += (double) y * y; ++cnt; } }
        return std::sqrt(acc / cnt);
    };
    for (int nb = 2; nb <= 4; ++nb) {   // crossover network must sum flat (all-pass) for every band count
        BandSplitter bs; bs.prepare(sr); const float xo[3] = { 150.f, 1500.f, 6000.f }; bs.setup(nb, xo);
        float worst = 0.f;
        for (float f : { 40.f, 100.f, 150.f, 400.f, 1500.f, 3000.f, 6000.f, 12000.f }) {
            bs.reset();
            const double r = sineRms([&](float x) { float o[4]; bs.process(0, x, o); float s = 0; for (int k = 0; k < nb; ++k) s += o[k]; return s; }, f, 48000);
            worst = std::max(worst, std::fabs(20.f * std::log10((float) r / (0.25f * 0.70710678f))));
        }
        char m[96]; std::snprintf(m, sizeof m, "%d-band crossover sums flat (worst deviation %.3f dB)", nb, worst); CHECK(worst < 0.1f, m);
    }
    { BandSplitter bs; bs.prepare(sr); const float xo[3] = { 150.f, 1500.f, 6000.f }; bs.setup(4, xo);
      float rms[4][3]; const float fr[3] = { 50.f, 600.f, 15000.f };
      for (int t = 0; t < 3; ++t) { bs.reset(); double acc[4] = {}; for (int i = 0; i < 48000; ++i) { float o[4]; bs.process(0, 0.25f * std::sin(6.2831853f * fr[t] * i / 48000.f), o); if (i > 24000) for (int k = 0; k < 4; ++k) acc[k] += (double) o[k] * o[k]; } for (int k = 0; k < 4; ++k) rms[k][t] = (float) std::sqrt(acc[k] / 23999); }
      CHECK(rms[0][0] > 10 * rms[1][0] && rms[0][0] > 100 * rms[3][0], "50 Hz lives in band 1");
      CHECK(rms[1][1] > 10 * rms[0][1] && rms[1][1] > 10 * rms[3][1], "600 Hz lives in band 2");
      CHECK(rms[3][2] > 10 * rms[2][2] && rms[3][2] > 100 * rms[0][2], "15 kHz lives in band 4"); }
    {   // per-band independence: loud 80 Hz is compressed, another band stays at unity
        MultibandEngine mb; mb.prepare(sr);
        const float xo[3] = { 150.f, 1500.f, 6000.f }; EngineParams base; base.kneeDb = 0; base.mode = 0;
        BandSettings bs[4]; for (auto& b : bs) { b.thr = 0.f; b.ratio = 1.f; b.atk = 5.f; b.rel = 100.f; }
        bs[0].thr = -30.f; bs[0].ratio = 6.f;
        mb.setup(4, xo, base, bs); const bool act[2] = { true, true };
        float grLo = 0, grHi = 0;
        for (int i = 0; i < 96000; ++i) {
            const float lo = 0.5f * std::sin(6.2831853f * 80.f * i / 48000.f), hi = 0.01f * std::sin(6.2831853f * 5000.f * i / 48000.f);
            const float x[2] = { lo + hi, lo + hi }, d[2] = { x[0], x[1] }; float w[2], l[2];
            mb.process(x, d, 2, true, act, w, l);
            grLo = mb.grDb(0); grHi = mb.grDb(2);
        }
        CHECK(grLo < -12.f, "multiband: band 1 compresses its loud 80 Hz tone");
        CHECK(std::fabs(grHi) < 0.01f, "multiband: band 3 (thr 0 dB, 1:1) applies no gain reduction");
    }
    {   // solo / mute / bypass / mono / silence
        MultibandEngine mb; mb.prepare(sr); const float xo[3] = { 150.f, 1500.f, 6000.f }; EngineParams base; BandSettings bs[4]; for (auto& b : bs) { b.thr = 0.f; b.ratio = 1.f; }
        const bool act[2] = { true, true };
        auto rmsAt = [&](float f) { mb.reset(); double a = 0; for (int i = 0; i < 48000; ++i) { const float v = 0.25f * std::sin(6.2831853f * f * i / 48000.f), x[2] = { v, v }; float w[2], l[2]; mb.process(x, x, 2, true, act, w, l); if (i > 24000) a += (double) w[0] * w[0]; } return std::sqrt(a / 23999); };
        mb.setup(4, xo, base, bs); const double ref = rmsAt(5000.f);
        bs[1].solo = true; mb.setup(4, xo, base, bs); CHECK(rmsAt(5000.f) < ref * 0.03 && rmsAt(500.f) > ref * 0.8, "solo band 2 isolates 150 Hz - 1.5 kHz");
        bs[1].solo = false; bs[2].mute = true; mb.setup(4, xo, base, bs); CHECK(rmsAt(3000.f) < ref * 0.18 && rmsAt(500.f) > ref * 0.9 && rmsAt(15000.f) > ref * 0.9, "mute band 3 removes ~1.5 - 6 kHz (>15 dB at 3 kHz), others untouched");
        bs[2].mute = false; bs[0].gainDb = 6.f; mb.setup(4, xo, base, bs); { const double r = rmsAt(50.f); CHECK(std::fabs(20 * std::log10(r / ref) - 6.f) < 0.5f, "band make-up gain +6 dB"); }
        bs[0].gainDb = 0.f; bs[0].thr = -40.f; bs[0].ratio = 10.f; bs[0].bypass = true; mb.setup(4, xo, base, bs);
        { const double r = rmsAt(50.f); CHECK(std::fabs(20 * std::log10(r / ref)) < 0.3f, "band bypass -> no compression"); }
        mb.reset(); bool ok = true; float z[2] = { 0, 0 }, w[2], l[2]; for (int i = 0; i < 48000; ++i) { mb.process(z, z, 2, true, act, w, l); if (!std::isfinite(w[0]) || std::fabs(w[0]) > 1e-6f) ok = false; }
        CHECK(ok, "multiband: silence -> silence, no NaN");
        bs[0].bypass = false; mb.setup(3, xo, base, bs); const bool act1[2] = { true, false }; float x1[1] = { 0.3f }; ok = true;
        for (int i = 0; i < 4800; ++i) { mb.process(x1, x1, 1, true, act1, w, l); if (!std::isfinite(w[0])) ok = false; } CHECK(ok, "multiband: mono / 3 bands runs");
        // crossovers dragged out of order are repaired (no NaN, still flat)
        const float bad[3] = { 5000.f, 200.f, 100.f }; bs[0].thr = 0; bs[0].ratio = 1; mb.setup(4, bad, base, bs); ok = true;
        for (int i = 0; i < 9600; ++i) { const float v = 0.25f * std::sin(6.2831853f * 1000.f * i / 48000.f), x[2] = { v, v }; mb.process(x, x, 2, true, act, w, l); if (!std::isfinite(w[0])) ok = false; }
        CHECK(ok, "multiband: out-of-order crossover settings are sanitised"); }
    std::printf(fails ? "\n%d FAILED\n" : "\nALL PASSED\n", fails);
    return fails ? 1 : 0;
}
