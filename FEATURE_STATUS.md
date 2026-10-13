# Feature status

| Feature | Status | Notes |
|---|---|---|
| Stereo compressor | Implemented | Feed-forward envelope compressor |
| 14 styles | Implemented as original approximations | Style-dependent ratio/timing and upward modes; not proprietary algorithms |
| Threshold / ratio / attack / release | Implemented | Host-automatable |
| Knee / range / hold | Implemented | Soft knee and gain-change limit |
| Lookahead | Implemented | Delay line plus reported plugin latency |
| Parallel mix | Implemented | 0–100% blend |
| Auto release | Implemented | Program-dependent release estimate |
| Auto threshold | Implemented | Slow level adaptation |
| Auto gain | Partial | Simplified static makeup estimate; not loudness matching |
| Peak / RMS detector | Implemented | Detector choices |
| Stereo link | Implemented | Continuous 0–100% blend |
| External sidechain | Implemented in code | DAW must expose/connect sidechain input |
| Sidechain HPF / LPF | Implemented | Detector path only |
| Six-band sidechain EQ | Implemented | Six draggable bell nodes, fixed 6 bands with adjustable frequency/gain; Q automatable |
| Tube / Diode / Bright | Basic implementation | Nonlinear waveshaping; oversampling is not implemented |
| Input/output meters | Implemented | Peak dBFS, block-updated |
| Gain-reduction readout | Implemented | Maximum reduction/change seen in the block |
| Factory presets | Basic implementation | Actual parameter values are written |
| A/B snapshots | Implemented | Save A, save B, toggle active snapshot |
| MIDI Learn | Not implemented | Planned |
| Undo/redo history | Not implemented | Planned |
| Multiband compressor | Not implemented | Planned |
| Tempo sync | Not implemented | Planned |
| True peak / EBU loudness | Not implemented | Planned |
| 32x oversampling | Not implemented | Planned |
| Dolby Atmos / 9.1.6 | Not implemented | Planned |
