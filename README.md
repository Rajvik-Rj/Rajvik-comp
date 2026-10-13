# RAJVIK DYNAMICS PRO

Original JUCE/C++ compressor plugin (VST3 + Standalone) with 14 compression styles, sidechain EQ, analog character and a live GR display.
Original design inspired by modern dynamics-plugin workflow; no third-party artwork or branding.

## Build (Windows, VST3)
Requires Visual Studio 2022 (Desktop C++) and CMake >= 3.22. JUCE 8.0.6 is downloaded automatically.
```
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```
Copy `build/RajvikDynamicsPro_artefacts/Release/VST3/RAJVIK DYNAMICS PRO.vst3` to `C:\Program Files\Common Files\VST3\`.
Tests only: `cmake -B build -DRDP_BUILD_PLUGIN=OFF && cmake --build build && ctest --test-dir build`.
CLAP builds automatically (output in `RajvikDynamicsPro_artefacts/Release/CLAP`); AU builds on macOS.

## Architecture
`Source/DSP/` is JUCE-free and unit-tested: `CompressorEngine` (detector, gain computer, smoothing, linking), `CompressionAlgorithms` (14 mode profiles),
`GainComputer`, `SidechainEQ` (RBJ biquads, detector only), `SaturationProcessor`. `PluginProcessor` (APVTS, latency, oversampling, meters via lock-free ring),
`PresetManager`, `PluginEditor` + `EditorComponents` (knobs, display, meters, interactive EQ graph).

## CPU notes
Headers are inline, no allocation/locks in the audio thread, coefficients recomputed only on parameter change, single shared gain path when link = 100%,
saturation/oversampling only run when Character != Clean, auto-release coefficient updated every 16 samples.

## Mode differences
Clean/Versatile (feed-forward RMS/peak) · Classic/Bus/Op-El/Vari-Mu (feedback detection) · Opto/Op-El/Vocal/Mastering (program-dependent release) ·
Punch (3x attack stretch) · Pumping (constant-slope release + hold) · Upward / TTM (upward & combined gain curves) · Vari-Mu (ratio rises with GR) · Smooth (very soft knee).

## Feature checklist
**Done:** 14 modes; threshold/ratio(1-100)/attack/release/knee(<=72)/range/hold/lookahead (latency reported); peak/RMS/style detector; auto release;
stereo link 0-100% **and Mid/Side mode** (detector and gain applied in M/S); internal/external sidechain; sidechain listen; 5-band sidechain EQ with draggable graph;
Tube/Diode/Bright saturation with drive and DC blocking; **oversampling 2x-32x** (IIR half-band, latency reported); **loudness-matched Auto Gain** (1.5 s power matching, replaces the static estimate);
mix; in/out gain; live input/output/GR history + transfer curve from real DSP; **tempo sync** (host BPM/PPQ pulse trigger, note rate 1/1-1/32, offset, pulse width; ignored when transport is stopped);
**MIDI Learn** (toggle MIDI Learn, touch a knob, move a CC; map saved in plugin state; Clear MIDI); **meters:** peak hold, RMS in/out, BS.1770 momentary/short-term/gated integrated LUFS, 4x true-peak estimate (click strip to reset);
13+ factory presets, user presets (`%APPDATA%/RAJVIK/DynamicsPro/Presets`); undo/redo; A/B; copy/paste; reset; state save/restore; DSP unit tests (incl. LUFS accuracy).
**Formats:** VST3 + Standalone everywhere, **CLAP** via clap-juce-extensions (`-DRDP_BUILD_CLAP=OFF` to skip), **AU** automatically on macOS.

**Partial:** sidechain EQ has 5 bands (no per-band M/S); no Mid/Side *metering*; loudness meters are computed only while the editor is open (saves CPU); sync mode uses a pulse as the detector (it does not follow the audio while active).

**Planned / not implemented:** pre-compression saturation routing, fullscreen/help buttons, surround layouts, polarity invert, host-level state/automation tests.

## Honest status
The DSP core (including the loudness meter) compiles and its tests pass. The JUCE plugin/GUI layer was written without being compiled or loaded in a DAW in the authoring environment: expect to fix a few compile errors on first build, then validate in your DAW.

## Troubleshooting
CMake can't fetch JUCE -> check git/network. MSVC C++20 errors -> use VS 2022 17.4+. Plugin not found -> rescan VST3 folder in your DAW.
