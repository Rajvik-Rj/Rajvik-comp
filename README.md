# RAJVIK DYNAMICS PRO

Cross-platform JUCE audio-plugin starter project for GitHub Actions.

## What this repository builds

- Windows: VST3
- macOS: VST3 and Audio Unit (AU)
- Linux: VST3
- A basic working stereo compressor with Threshold, Ratio, Attack, Release, and Mix controls.
- A simple custom dark UI.
- GitHub Actions automatically builds artifacts for all three operating systems.

This is a reliable starting foundation, not the complete 14-mode feature set from the design brief yet. Add features incrementally and test each one.

## Important requirements

- GitHub account and a repository created from these files.
- No local C++ compiler is required if you use GitHub Actions.
- JUCE is downloaded by CMake during the build, so the runner needs internet access.
- This starter uses JUCE's current public CMake workflow. Check JUCE licensing before distributing a commercial product.

## Upload to GitHub

1. Extract this ZIP.
2. Create a new empty GitHub repository, for example `rajvik-dynamics-pro`.
3. Upload the *contents* of this folder to the repository root (the `.github` folder must also be included).
4. Commit the files to the `main` branch.
5. Open **Actions** and enable workflows if GitHub asks.
6. Open the **Build Audio Plugins** workflow and choose **Run workflow**, or push a new commit.
7. Wait for Windows, macOS, and Linux jobs to finish.
8. Open the completed workflow run and download the artifacts at the bottom.

## Installing the builds

- **Windows:** extract the Windows artifact and copy `RAJVIK Dynamics Pro.vst3` to `%COMMONPROGRAMFILES%\\VST3` (normally `C:\\Program Files\\Common Files\\VST3`). Some DAWs can scan a custom folder.
- **macOS:** extract the macOS artifact. Copy the `.vst3` bundle to `~/Library/Audio/Plug-Ins/VST3` or `/Library/Audio/Plug-Ins/VST3`. The `.component` bundle is the AU build for compatible DAWs. Unsigned builds may require additional macOS security steps.
- **Linux:** extract the Linux artifact and copy the `.vst3` bundle to `~/.vst3` or `~/.vst3/` as appropriate for your DAW.

Restart/rescan plugins in your DAW after installing. The host and plugin architecture must match (for example, Apple Silicon vs Intel).

## Build locally (optional)

Requires CMake, a C++17 compiler, and platform dependencies described by JUCE.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target "RAJVIK Dynamics Pro_VST3"
```

On macOS, the AU target is also generated.

## Current implementation

- Stereo audio processing.
- Threshold, Ratio, Attack, Release, Mix.
- Input/output gain-reduction metering is intentionally minimal in this first scaffold.
- Preset, sidechain EQ, 14 compression styles, oversampling, surround, MIDI learn, and advanced metering are not implemented yet.

## Repository structure

```text
.
├── .github/workflows/build.yml
├── CMakeLists.txt
├── README.md
├── Source/
│   ├── PluginProcessor.h
│   ├── PluginProcessor.cpp
│   ├── PluginEditor.h
│   └── PluginEditor.cpp
└── LICENSE
```
