cmake_minimum_required(VERSION 3.22)
project(RajvikDynamicsPro VERSION 1.0.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

# --- DSP unit tests (no JUCE needed) ---
enable_testing()
add_executable(DspTests Tests/DspTests.cpp)
target_include_directories(DspTests PRIVATE Source/DSP)
add_test(NAME DspTests COMMAND DspTests)

# --- JUCE plugin (disable with -DRDP_BUILD_PLUGIN=OFF to build tests only) ---
# Keep the primary build deterministic: VST3 only by default. Optional formats
# are enabled explicitly by separate CI configure steps.
option(RDP_BUILD_PLUGIN "Build the JUCE plugin" ON)
option(RDP_BUILD_CLAP "Build CLAP via clap-juce-extensions" OFF)
option(RDP_BUILD_AU "Build Audio Unit (macOS only)" OFF)
option(RDP_BUILD_STANDALONE "Build a standalone app" OFF)
if(RDP_BUILD_PLUGIN)
  include(FetchContent)
  set(JUCE_BUILD_EXTRAS OFF CACHE BOOL "" FORCE)
  set(JUCE_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.15
    GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(JUCE)

  set(RDP_FORMATS VST3)
  if(RDP_BUILD_STANDALONE)
    list(APPEND RDP_FORMATS Standalone)
  endif()
  if(APPLE AND RDP_BUILD_AU)
    list(APPEND RDP_FORMATS AU)
  endif()
  if(RDP_BUILD_CLAP)
    set(CLAP_JUCE_EXTENSIONS_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(clap-juce-extensions
      GIT_REPOSITORY https://github.com/free-audio/clap-juce-extensions.git
      GIT_TAG main
      GIT_SHALLOW TRUE
      GIT_SUBMODULES_RECURSE TRUE)
    FetchContent_MakeAvailable(clap-juce-extensions)
  endif()

  juce_add_plugin(RajvikDynamicsPro
    COMPANY_NAME "RAJVIK"
    PRODUCT_NAME "RAJVIK DYNAMICS PRO"
    PLUGIN_MANUFACTURER_CODE Rajv
    PLUGIN_CODE Rdp1
    FORMATS ${RDP_FORMATS}
    AU_MAIN_TYPE kAudioUnitType_Effect
    VST3_CATEGORIES Fx Dynamics
    IS_SYNTH FALSE NEEDS_MIDI_INPUT TRUE NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)

  target_sources(RajvikDynamicsPro PRIVATE Source/PluginProcessor.cpp Source/PluginEditor.cpp)
  target_compile_definitions(RajvikDynamicsPro PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_DISPLAY_SPLASH_SCREEN=0)
  target_link_libraries(RajvikDynamicsPro PRIVATE juce::juce_audio_utils juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

  if(RDP_BUILD_CLAP)
    clap_juce_extensions_plugin(TARGET RajvikDynamicsPro CLAP_ID "com.rajvik.dynamicspro" CLAP_FEATURES audio-effect compressor)
  endif()
endif()
