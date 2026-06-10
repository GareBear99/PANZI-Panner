cmake_minimum_required(VERSION 3.15)
project(PANZI VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── JUCE ──────────────────────────────────────────────────────────────
# Option A (recommended): JUCE as submodule at ./JUCE
#   git submodule add https://github.com/juce-framework/JUCE.git JUCE
#   git -C JUCE checkout 7.0.12
# Option B: pass -DJUCE_DIR=/path/to/JUCE
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
    add_subdirectory(JUCE)
elseif (DEFINED JUCE_DIR AND EXISTS "${JUCE_DIR}/CMakeLists.txt")
    add_subdirectory("${JUCE_DIR}" JUCE)
else()
    message(FATAL_ERROR
        "JUCE not found. Add as submodule in ./JUCE "
        "or pass -DJUCE_DIR=/path/to/JUCE")
endif()

# ── Plugin target ─────────────────────────────────────────────────────
juce_add_plugin(PANZI
    COMPANY_NAME               "TizWildin"
    IS_SYNTH                   FALSE
    NEEDS_MIDI_INPUT           FALSE
    NEEDS_MIDI_OUTPUT          FALSE
    IS_MIDI_EFFECT             FALSE
    COPY_PLUGIN_AFTER_BUILD    TRUE
    PLUGIN_MANUFACTURER_CODE   TzWd
    PLUGIN_CODE                PNZI
    FORMATS                    VST3 AU Standalone
    PRODUCT_NAME               "PANZI"
    VST3_CATEGORIES            "Spatial Fx"
    AU_MAIN_TYPE               "aufx"
    # 8-channel (7.1) output bus
    PLUGIN_AU_SANDBOX_SAFE     FALSE
)

target_sources(PANZI PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/Config.h
    Source/DSP/AutoPanDSP.h
    Source/DSP/BakedCoefficients.h
    Source/DSP/PolyhedralTopology.h
    Source/DSP/TopologyBaker.h
    Source/DSP/DelayLine.h
    Source/DSP/SvfLowpass.h
    Source/DSP/RbjShelf.h
    Source/DSP/PanziEngine.h
)

target_compile_definitions(PANZI PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(PANZI PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
)

# ── Optimisation flags ────────────────────────────────────────────────
if (MSVC)
    target_compile_options(PANZI PRIVATE /O2 /arch:AVX2)
else()
    target_compile_options(PANZI PRIVATE -O3 -march=native -ffast-math)
endif()
