# RAJVIK DYNAMICS PRO

Original JUCE/C++ compressor plugin (VST3 + Standalone) with 14 compression styles, sidechain EQ, analog character and a live GR display.
Original design inspired by modern dynamics-plugin workflow; no third-party artwork or branding.

## Build locally
Requires CMake >= 3.22, Git, and a C++20 compiler. On Windows, install Visual Studio with **Desktop development with C++**. JUCE 8.0.6 and CLAP extensions are fetched by CMake. Both C and C++ are enabled because recent JUCE/CMake toolchains validate the C compiler during configuration.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel 2
ctest --test-dir build -C Release --output-on-failure
```

The VST3 bundle is under `build/RajvikDynamicsPro_artefacts/Release/VST3/`. Copy it to `C:\Program Files\Common Files\VST3\` on Windows, `~/Library/Audio/Plug-Ins/VST3/` on macOS, or `~/.vst3/` on Linux. CLAP also builds by default; AU is added on macOS. Set `-DRDP_BUILD_CLAP=OFF` to skip CLAP, or `-DRDP_BUILD_PLUGIN=OFF` to build only the DSP tests.

## GitHub Actions builds
Push the project to GitHub and open **Actions**. The workflow builds Windows, macOS, and Linux plugin artifacts separately and runs the DSP tests. `fail-fast: false` lets the other platforms continue even if one build fails. Download the desired plugin bundle from the successful workflow run's **Artifacts** section.

## Architecture
`Source/DSP/` is JUCE-free and unit-tested: `CompressorEngine` (detector, gain computer, smoothing, linking), `CompressionAlgorithms` (14 mode profiles),
`GainComputer`, `SidechainEQ` (6-band RBJ biquads, detector only), `Crossover` (LR4 band splitter), `MultibandEngine` (up to 4 band compressors), `SaturationProcessor`. `PluginProcessor` (APVTS, latency, oversampling, meters via lock-free ring),
`PresetManager`, `PluginEditor` + `EditorComponents` (knobs, display, meters, interactive EQ graph).

## CPU notes
Headers are inline, no allocation/locks in the audio thread, coefficients recomputed only on parameter change, single shared gain path when link = 100%,
saturation/oversampling only run when Character != Clean, auto-release coefficient updated every 16 samples.

## Mode differences
Clean/Versatile (feed-forward RMS/peak) · Classic/Bus/Op-El/Vari-Mu (feedback detection) · Opto/Op-El/Vocal/Mastering (program-dependent release) ·
Punch (3x attack stretch) · Pumping (constant-slope release + hold) · Upward / TTM (upward & combined gain curves) · Vari-Mu (ratio rises with GR) · Smooth (very soft knee).

## Feature checklist
**Done:** 14 modes; threshold/ratio(1-100)/attack/release/knee(<=72)/range/hold/lookahead (latency reported); peak/RMS/style detector; auto release;
stereo link 0-100% **and Mid/Side mode** (detector and gain applied in M/S); internal/external sidechain; sidechain listen; 6-band sidechain EQ (HPF/shelves/bells/LPF per band, draggable graph + table), detector routing Stereo/Mid/Side, sidechain level, Process-Mid/Side toggles;
Tube/Diode/Bright saturation with drive and DC blocking; **oversampling 2x-32x** (IIR half-band, latency reported); **loudness-matched Auto Gain** (1.5 s power matching, replaces the static estimate);
mix; in/out gain; live input/output/GR history + transfer curve from real DSP; **tempo sync** (host BPM/PPQ pulse trigger, note rate 1/1-1/32, offset, pulse width; ignored when transport is stopped);
**MIDI Learn** (toggle MIDI Learn, touch a knob, move a CC; map saved in plugin state; Clear MIDI); **meters:** peak hold, RMS in/out, BS.1770 momentary/short-term/gated integrated LUFS, 4x true-peak estimate (click strip to reset);
13+ factory presets, user presets (`%APPDATA%/RAJVIK/DynamicsPro/Presets`); undo/redo; A/B; copy/paste; reset; state save/restore; DSP unit tests (incl. LUFS accuracy).
**Formats:** VST3 + Standalone everywhere, **CLAP** via clap-juce-extensions (`-DRDP_BUILD_CLAP=OFF` to skip), **AU** automatically on macOS.

**Partial:** sidechain EQ has no per-band M/S; no Mid/Side *metering*; loudness meters are computed only while the editor is open (saves CPU); sync mode uses a pulse as the detector (it does not follow the audio while active).

**Planned / not implemented:** pre-compression saturation routing, fullscreen/help buttons, surround layouts, polarity invert, host-level state/automation tests.

## Honest status
The DSP tests run as part of the GitHub Actions workflow. A green CI build confirms compilation for that runner and plugin format; still load-test the plugin in your own DAW before relying on it in a production session.

## Troubleshooting
CMake can't fetch JUCE -> check git/network. MSVC C++20 errors -> use VS 2022 17.4+. Plugin not found -> rescan VST3 folder in your DAW.

## Multiband (2-4 bands)

Switch **Single / Multiband** next to the Style menu. Linkwitz-Riley 4th-order crossovers (phase-aligned, sums flat to 0.001 dB), one compressor per band.
Per band: threshold, ratio, attack, release, make-up gain, solo, mute, bypass. Shared by all bands: style, knee, range, hold, lookahead, detector,
stereo link / Mid-Side, sidechain section (external sidechain is split into the same bands), mix, in/out gain, auto gain, saturation.
In multiband mode the Threshold/Ratio/Attack/Release knobs edit the **selected band**. Tempo-sync is single-band only.
Display: drag white handles = crossovers, drag a band's dashed line = threshold, orange bars = live gain reduction per band.

## UI

Layout is a 1:1 recreation of the reference design on a 1536x1024 canvas, scaled (resizable, fixed aspect, Scale menu 75-200 %).
Things the reference has no room for live behind the gear icon (Character, Drive, Tempo Sync, actions); LUFS / true-peak: click the OUT read-out.
Character lives in the gear menu; the Invert button flips detector polarity (no effect on compression).

**Build note:** `DspTests` can be built without downloading JUCE by configuring with `-DRDP_BUILD_PLUGIN=OFF`. The complete plugin is built by the GitHub Actions workflow on Windows, macOS, and Linux.

## Visualizers (real-time FFT)

4096-point Hann FFT analyzers run in the GUI from sample rings filled by the audio thread (only while the editor is open):
- main display: **Input** (grey) and **Output** (blue) spectrum + gain-reduction history + transfer curve with knee/threshold;
- sidechain EQ graph: detector spectrum **before** (grey line) and **after** (white) the EQ, plus the 6 band curves - so you can see the EQ acting on what the compressor hears;
- multiband display: input spectrum behind the band regions.
