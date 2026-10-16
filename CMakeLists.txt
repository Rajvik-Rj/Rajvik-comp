cmake_minimum_required(VERSION 3.22)
project(RajvikDynamicsPro VERSION 1.0.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

# --- DSP unit tests (no JUCE needed) ---
enable_testing()
add_executable(DspTests Tests/DspTests.cpp)
target_include_directories(DspTests PRIVATE Source/DSP)
add_test(NAME DspTests COMMAND DspTests)

# --- JUCE plugin (disable with -DRDP_BUILD_PLUGIN=OFF to build tests only) ---
option(RDP_BUILD_PLUGIN "Build the JUCE plugin" ON)
if(RDP_BUILD_PLUGIN)
  include(FetchContent)
  FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.6 GIT_SHALLOW ON)
  FetchContent_MakeAvailable(JUCE)

  set(RDP_FORMATS VST3)   # VST3 only (Windows, macOS, Linux)

  juce_add_plugin(RajvikDynamicsPro
    COMPANY_NAME "RAJVIK"
    PRODUCT_NAME "RAJVIK DYNAMICS PRO"
    PLUGIN_MANUFACTURER_CODE Rajv
    PLUGIN_CODE Rdp1
    FORMATS ${RDP_FORMATS}
    VST3_CATEGORIES Fx Dynamics
    IS_SYNTH FALSE NEEDS_MIDI_INPUT TRUE NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)

  target_sources(RajvikDynamicsPro PRIVATE Source/PluginProcessor.cpp Source/PluginEditor.cpp)
  target_compile_definitions(RajvikDynamicsPro PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_DISPLAY_SPLASH_SCREEN=0)
  target_link_libraries(RajvikDynamicsPro PRIVATE juce::juce_audio_utils juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)
endif()

# Optional dev tool: renders the editor to PNG (cmake -DRDP_BUILD_SHOT=ON, run under xvfb-run)
option(RDP_BUILD_SHOT "Build the screenshot tool" OFF)
if(RDP_BUILD_PLUGIN AND RDP_BUILD_SHOT)
  juce_add_console_app(RdpShot PRODUCT_NAME "RdpShot")
  target_sources(RdpShot PRIVATE Tools/Shot.cpp)
  target_compile_definitions(RdpShot PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_MODAL_LOOPS_PERMITTED=1)
  target_link_libraries(RdpShot PRIVATE RajvikDynamicsPro juce::juce_audio_utils juce::juce_dsp juce::juce_recommended_config_flags)
endif()
