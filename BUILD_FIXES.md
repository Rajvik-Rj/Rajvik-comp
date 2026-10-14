# Build fixes in this package

- Enabled both C and C++ in the root CMake project. This addresses the JUCE/CMake configure failure where `CMAKE_C_COMPILE_OBJECT` is missing with newer CMake releases.
- Kept CLAP optional through `RDP_BUILD_CLAP` and enabled it in CI.
- Updated GitHub Actions to build Windows, macOS, and Linux independently (`fail-fast: false`), run the DSP tests, and upload the discovered VST3/CLAP/AU bundles as artifacts.
- Added the Linux packages required by JUCE's GUI/audio modules.

## Run the builds

Push the extracted project contents to the root of the GitHub repository, then open **Actions**. Each successful platform job will publish a separate artifact. This local verification environment could run the DSP tests, but it could not fetch JUCE from GitHub because outbound DNS/network access was unavailable; the full plugin build still needs to be confirmed by GitHub Actions.
