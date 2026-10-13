#pragma once
#include "GainComputer.h"
namespace rdp {
struct ModeProfile {
    const char* name; const char* description;
    float attackMul, releaseMul, kneeAdd, rmsMs;   // rmsMs 0 = peak detector
    bool feedback;      // detector sees the already-compressed level
    float progDep;      // program-dependent release depth
    bool slewRelease;   // constant dB/s release (pumping)
    bool variMu;        // ratio rises with gain reduction
    Kind kind; float holdMs;
    float defThr, defRatio, defAtk, defRel, defKnee;
};
inline constexpr int kNumModes = 14;
inline const ModeProfile kModes[kNumModes] = {
 {"Clean",     "Transparent, low-distortion feed-forward",        1.0f,1.0f, 3,  5, false,0.3f,false,false,Kind::Downward,  0, -18,3,  10,120, 6},
 {"Classic",   "Peak, feedback topology, general purpose",         1.0f,1.0f, 0,  0, true, 0.0f,false,false,Kind::Downward,  0, -20,4,   5,100, 2},
 {"Opto",      "Smooth, strongly program-dependent release",       2.0f,1.0f,12, 25, false,1.5f,false,false,Kind::Downward,  0, -24,3,  20,300,18},
 {"Vocal",     "Controlled dynamics, smooth recovery",             0.8f,1.0f, 8, 10, false,1.0f,false,false,Kind::Downward,  0, -22,4,   8,150,10},
 {"Mastering", "Slow, transparent, low artifact",                  1.5f,1.2f,12, 30, false,0.8f,false,false,Kind::Downward,  0, -14,2,  30,250,16},
 {"Bus",       "Cohesive RMS feedback glue",                       1.2f,1.0f, 6, 20, true, 0.6f,false,false,Kind::Downward,  0, -16,2,  30,200, 8},
 {"Punch",     "Delayed attack lets transients through",           3.0f,0.7f, 2,  0, false,0.0f,false,false,Kind::Downward,  0, -20,5,  15, 80, 4},
 {"Pumping",   "Constant-slope release with hold, rhythmic",       0.5f,1.0f, 0,  0, false,0.0f,true, false,Kind::Downward, 30, -26,8,   2,180, 0},
 {"Versatile", "Adaptive general purpose",                         1.0f,1.0f, 6,  8, false,1.0f,false,false,Kind::Downward,  0, -18,3,  12,150, 8},
 {"Smooth",    "Very soft knee, slow and gentle",                  1.5f,1.3f,24, 40, false,0.5f,false,false,Kind::Downward,  0, -18,2.5f,25,250,24},
 {"Upward",    "Raises material below threshold",                  1.0f,1.5f,12, 15, false,0.0f,false,false,Kind::Upward,    0, -40,2,  20,200,12},
 {"TTM",       "Downward above threshold + upward 24 dB below",    1.0f,1.2f, 8, 10, false,0.5f,false,false,Kind::Combined,  0, -20,3,  15,180, 8},
 {"Op-El",     "Opto/electro feedback hybrid",                     1.5f,1.0f,14, 20, true, 1.2f,false,false,Kind::Downward,  0, -22,3,  15,250,14},
 {"Vari-Mu",   "Ratio increases with gain reduction",              1.8f,1.4f,20, 15, true, 0.8f,false,true, Kind::Downward,  0, -20,3,  25,300,20},
};
} // namespace rdp
