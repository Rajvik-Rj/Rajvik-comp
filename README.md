# RAJVIK Dynamics Pro — GitHub Build Project

An original JUCE/C++ compressor plugin project with an automated GitHub Actions workflow. Upload this repository to GitHub and it will attempt to build a VST3 plugin for Windows, macOS and Linux, plus an AU component on macOS.

> **Important:** this is an original compressor inspired by common professional compressor workflows. It does not contain FabFilter's proprietary source code or reproduce its exact sound. The styles below are original approximations and should be auditioned and tuned against real audio before production use.

## What is implemented in this version

- 14 selectable compressor styles: Clean, Classic, Opto, Vocal, Mastering, Bus, Punch, Pumping, Versatile, Smooth, Upward, TTM-style upward/downward, Op-El-style and Vari-Mu-style.
- Real-time stereo compression DSP with threshold, ratio, attack, release, soft knee, range limit, hold, lookahead and parallel mix.
- Internal or host-routed external sidechain input. The DAW must enable and route the sidechain bus for External to work.
- Peak and RMS detector choices, stereo-link amount, input and output gain.
- Auto-release, adaptive auto-threshold, approximate auto makeup gain, and selectable Tube/Diode/Bright nonlinear coloration.
- Sidechain detector shaping with HPF, LPF and six adjustable parametric bell bands. Drag each coloured node in the sidechain EQ graph to change its frequency and gain; each band has a Q parameter available for host automation.
- Real signal-scope traces, threshold/transfer curve, input/output peak meters and a gain-reduction readout.
- Factory starting presets, parameter save/restore, A/B snapshot save/compare and Reset.
- GitHub Actions build jobs for Windows VST3, macOS VST3 + AU, and Linux VST3.

## Not implemented yet

This is a more functional first DSP version, **not a full Pro-C 3 replacement**. It does not yet implement multi-band compression, TTM cross-band dynamics, per-band Mid/Side EQ, a true-peak/loudness meter suite, 32x oversampling, tempo-synced triggering, Dolby Atmos / 9.1.6, full MIDI Learn, undo/redo history, or commercial plug-in validation. Auto Gain is a simplified makeup estimate, not a loudness-matching system. Saturation is not yet oversampled, so use moderate Drive and check high-frequency artifacts.

## Build it on GitHub without installing a compiler on your PC

1. Download and extract this ZIP.
2. Create an empty repository at [github.com/new](https://github.com/new), for example `rajvik-dynamics-pro`.
3. Upload the **contents** of the extracted folder to the repository root. Include the hidden `.github/workflows/build.yml` file.
4. Commit to `main` (or `master`). GitHub Actions should start automatically. If prompted, enable Actions.
5. Open the **Actions** tab, select **Build RAJVIK Dynamics Pro**, and open the latest run.
6. Wait for each platform job to succeed. Download the artifact ZIP shown at the bottom of that completed run.

The workflow downloads JUCE during CI, so GitHub runners need network access. This workspace has no network access to GitHub, so the actual CI build must be validated by running the workflow on GitHub; the ZIP itself is source code, not precompiled plugin binaries.

## Install the resulting bundles

- **Windows VST3:** copy `RAJVIK Dynamics Pro.vst3` into `C:\Program Files\Common Files\VST3` (or the VST3 folder your DAW scans).
- **macOS VST3:** copy the `.vst3` bundle into `~/Library/Audio/Plug-Ins/VST3`.
- **macOS AU:** copy the `.component` bundle into `~/Library/Audio/Plug-Ins/Components`.
- **Linux VST3:** copy the `.vst3` bundle into `~/.vst3` or your DAW's configured VST3 folder.

Restart/rescan plugins in your DAW. The macOS workflow builds for the architecture provided by its runner; a universal Intel + Apple Silicon package is not currently configured. Unsigned macOS plugin bundles may require local security approval.

## Local build (optional)

Requirements: a C++17 compiler, CMake 3.22+, platform audio/UI development dependencies, and Internet access for JUCE download.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target RajvikDynamicsPro_VST3
```

On macOS, also build `RajvikDynamicsPro_AU`.

## Source layout

```text
.github/workflows/build.yml   # cross-platform GitHub Actions
CMakeLists.txt                # JUCE plugin and formats
Source/PluginProcessor.h      # processor interface and DSP state
Source/PluginProcessor.cpp    # compressor, detector, sidechain EQ, coloration
Source/PluginEditor.h         # controls and interactive editor interface
Source/PluginEditor.cpp       # dark GUI, compressor view, EQ graph, presets
```

## License note

JUCE has its own licensing terms. Review them before distributing a commercial product. This repository's license applies to the project code written for this scaffold, not JUCE or third-party dependencies.
