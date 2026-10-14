# RAJVIK Dynamics Pro — build fixes in this package

This version fixes a cross-platform source compilation error and improves CI diagnostics.

- The CMake project enables both C and C++, which JUCE and its helper projects require in this configuration.
- Fixed MIDI-learn state serialization: the mapping stores `RangedAudioParameter*`, so code now safely casts to `AudioProcessorParameterWithID` before obtaining the stable parameter ID. The old `p->paramID` access was invalid through the base pointer and could stop compilation on Windows, macOS, and Linux.
- Updated `actions/checkout` to v5 for current GitHub-hosted runners, and enabled verbose build output so any remaining compiler error is visible in the failed job log.
- CI keeps the platform jobs independent, installs the required Linux GUI/audio development packages, runs the DSP unit tests, and uploads built VST3/CLAP/AU bundles as artifacts.

## Correct upload method

Extract the ZIP and upload the *contents inside the `RajvikDynamicsPro` folder* to the repository root, preserving the `Source/`, `Tests/`, and `.github/workflows/` directory structure. Do not upload the outer folder as a nested directory and do not add another `RajvikDynamicsPro/` level inside the GitHub repository.

## Verification

The DSP-only build and its unit test pass in the available local environment. The full JUCE plugin compilation must be confirmed by GitHub Actions, because this environment cannot download JUCE dependencies directly.
