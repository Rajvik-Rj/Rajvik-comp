#pragma once
#include "GainComputer.h"
namespace rdp {

// One entry per Style. Every field changes how the sound behaves, not just a label:
//   attackMul / releaseMul   scale the user's Attack / Release (fast stage of the release)
//   kneeAdd                  extra soft knee added on top of the Knee control
//   rmsMs, rmsMix            detector: RMS window (0 = pure peak) and RMS share of the detector signal (rest = peak)
//   feedback                 detector listens to the already-compressed signal (gentler, program-dependent behaviour)
//   slewRelease              constant dB/s release (rhythmic pumping)
//   variMu                   ratio rises as gain reduction deepens (soft, "tube-like" onset)
//   adaptive                 attack / release follow the crest factor of the material (fast on transients, slow on dense parts)
//   slowMul, slowAmt         two-stage release: slow stage = Release x slowMul, blended in (up to slowAmt) as sustained
//                            gain reduction builds up (short peaks recover fast, long compression recovers slowly)
struct ModeProfile {
    const char* name; const char* description;
    float attackMul, releaseMul, kneeAdd;
    float rmsMs, rmsMix;
    bool feedback, slewRelease, variMu, adaptive;
    float slowMul, slowAmt;
    Kind kind; float holdMs;
    float defThr, defRatio, defAtk, defRel, defKnee;
};
inline constexpr int kNumModes = 14;
inline const ModeProfile kModes[kNumModes] = {
 //  name        description                                         atk  rel  knee  rms  mix   fb     slew   vari   adapt  slowM slowA kind              hold   thr  ratio atk  rel  knee
 {"Clean",     "Transparent, low-distortion feed-forward",         1.0f,1.0f, 3.f,  5.f,0.5f, false,false,false,false, 3.0f,0.25f,Kind::Downward,  0.f, -18.f,3.f,  10.f,120.f, 4.f},
 {"Classic",   "Peak, feedback topology, general purpose",         1.0f,1.0f, 0.f,  0.f,0.0f, true, false,false,false, 0.0f,0.00f,Kind::Downward,  0.f, -20.f,4.f,   5.f,100.f, 2.f},
 {"Opto",      "Smooth, strongly program-dependent release",       2.0f,0.3f,12.f, 25.f,1.0f, false,false,false,false, 6.0f,1.00f,Kind::Downward,  0.f, -24.f,3.f,  20.f,300.f, 6.f},
 {"Vocal",     "Controlled dynamics, smooth recovery",             0.8f,1.0f, 8.f, 10.f,0.5f, false,false,false,false, 3.5f,0.55f,Kind::Downward,  0.f, -22.f,4.f,   8.f,150.f, 6.f},
 {"Mastering", "Slow, transparent, low artifact",                  1.5f,1.2f,10.f, 30.f,0.7f, false,false,false,false, 3.0f,0.50f,Kind::Downward,  0.f, -14.f,2.f,  30.f,250.f, 6.f},
 {"Bus",       "Cohesive RMS feedback glue",                       1.2f,1.0f, 6.f, 20.f,1.0f, true, false,false,false, 3.0f,0.50f,Kind::Downward,  0.f, -16.f,2.f,  30.f,200.f, 4.f},
 {"Punch",     "Delayed attack lets transients through",           3.0f,0.7f, 2.f,  0.f,0.0f, false,false,false,false, 0.0f,0.00f,Kind::Downward,  0.f, -20.f,5.f,  15.f, 80.f, 2.f},
 {"Pumping",   "Constant-slope release with hold, rhythmic",       0.5f,1.0f, 0.f,  0.f,0.0f, false,true, false,false, 0.0f,0.00f,Kind::Downward, 30.f, -26.f,8.f,   2.f,180.f, 0.f},
 {"Versatile", "Adaptive general purpose",                         1.0f,1.0f, 4.f,  8.f,0.5f, false,false,false,true,  2.5f,0.40f,Kind::Downward,  0.f, -18.f,3.f,  12.f,150.f, 6.f},
 {"Smooth",    "Very soft knee, slow and gentle",                  1.5f,1.3f,16.f, 40.f,1.0f, false,false,false,false, 2.5f,0.40f,Kind::Downward,  0.f, -18.f,2.5f, 25.f,250.f,12.f},
 {"Upward",    "Raises material below threshold",                  1.0f,1.5f, 8.f, 15.f,1.0f, false,false,false,false, 2.0f,0.30f,Kind::Upward,    0.f, -40.f,2.f,  20.f,200.f, 8.f},
 {"TTM",       "Downward above threshold + upward 24 dB below",    1.0f,1.2f, 6.f, 10.f,0.6f, false,false,false,false, 2.5f,0.40f,Kind::Combined,  0.f, -20.f,3.f,  15.f,180.f, 6.f},
 {"Op-El",     "Opto/electro feedback hybrid",                     1.5f,0.6f,10.f, 20.f,0.8f, true, false,false,false, 4.0f,0.70f,Kind::Downward,  0.f, -22.f,3.f,  15.f,250.f, 6.f},
 {"Vari-Mu",   "Ratio increases with gain reduction",              1.8f,1.0f,12.f, 15.f,0.6f, true, false,true, false, 3.0f,0.50f,Kind::Downward,  0.f, -20.f,3.f,  25.f,300.f, 8.f},
};
} // namespace rdp
