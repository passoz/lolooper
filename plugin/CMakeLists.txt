cmake_minimum_required(VERSION 3.29)
project(Lolooper VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Path to JUCE
set(JUCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/JUCE")
set(FLUIDSYNTH_DIR "${CMAKE_CURRENT_SOURCE_DIR}/fluidsynth")
set(AUBIO_DIR "${CMAKE_CURRENT_SOURCE_DIR}/aubio")

if(NOT EXISTS ${JUCE_DIR})
    message(FATAL_ERROR "JUCE not found at ${JUCE_DIR}. Run: git submodule update --init")
endif()

add_subdirectory(${JUCE_DIR} JUCE)

# Main plugin target: VST3 + Standalone
# Note: JUCE 8 does NOT accept SOURCES in juce_add_plugin.
# Sources must be added via target_sources() after the call.
juce_add_plugin(Lolooper
    COMPANY_NAME "passoz"
    PLUGIN_MANUFACTURER_CODE Lolo
    PLUGIN_CODE Lpr1
    FORMATS VST3 Standalone
    PRODUCT_NAME "Lolooper"
    VERSION 1.0.0
)

# Suppress VST2/VST3 conflict warning (we use VST3 only)
target_compile_definitions(Lolooper PUBLIC JUCE_VST3_CAN_REPLACE_VST2=0)

# Add our source files to the static library target
target_sources(Lolooper PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/Sequencer.cpp
    Source/SampleLib.cpp
    Source/Mixer.cpp
    Source/Pattern.cpp
    Source/InstrumentLib.cpp
    Source/Arpeggiator.cpp
    Source/PitchDetector.cpp
    Source/ChordRecognizer.cpp
)

# Link JUCE modules
target_include_directories(Lolooper PRIVATE ${FLUIDSYNTH_DIR} ${AUBIO_DIR})
target_link_libraries(Lolooper PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_plugin_client
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_data_structures
    juce::juce_dsp
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
    fluidsynth
    aubio
)

# Set output directories
set_target_properties(Lolooper PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/VST3"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Standalone"
)

#==============================================================================
# Unit Tests
#==============================================================================

# Test Sequencer (no JUCE dependencies beyond juce_core)
add_executable(TestSequencer
    Tests/TestSequencer.cpp
    Source/Sequencer.cpp
)
target_compile_definitions(TestSequencer PRIVATE
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
)
target_include_directories(TestSequencer PRIVATE ${JUCE_DIR}/modules)
target_link_libraries(TestSequencer PRIVATE
    juce::juce_core
)

# Test SampleLib
add_executable(TestSampleLib
    Tests/TestSampleLib.cpp
    Source/SampleLib.cpp
)
target_compile_definitions(TestSampleLib PRIVATE
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
)
target_include_directories(TestSampleLib PRIVATE ${JUCE_DIR}/modules)
target_link_libraries(TestSampleLib PRIVATE
    juce::juce_core
    juce::juce_audio_basics
    juce::juce_audio_formats
)

# Test Mixer
add_executable(TestMixer
    Tests/TestMixer.cpp
    Source/Mixer.cpp
    Source/SampleLib.cpp
)
target_compile_definitions(TestMixer PRIVATE
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
)
target_include_directories(TestMixer PRIVATE ${JUCE_DIR}/modules)
target_link_libraries(TestMixer PRIVATE
    juce::juce_core
    juce::juce_audio_basics
    juce::juce_audio_formats
    juce::juce_dsp
)

# Test Pattern
add_executable(TestPattern
    Tests/TestPattern.cpp
    Source/Pattern.cpp
)
target_compile_definitions(TestPattern PRIVATE
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
)
target_include_directories(TestPattern PRIVATE ${JUCE_DIR}/modules)
target_link_libraries(TestPattern PRIVATE
    juce::juce_core
)

# Test InstrumentLib (SF2/SFZ via FluidSynth)
add_executable(TestInstrumentLib
    Tests/TestInstrumentLib.cpp
    Source/InstrumentLib.cpp
)
target_compile_definitions(TestInstrumentLib PRIVATE
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
)
target_include_directories(TestInstrumentLib PRIVATE
    ${JUCE_DIR}/modules
    ${FLUIDSYNTH_DIR}
)
target_link_libraries(TestInstrumentLib PRIVATE
    juce::juce_core
    juce::juce_audio_basics
    fluidsynth
)

# Test Arpeggiator (ChordProgression)
add_executable(TestArpeggiator
    Tests/TestArpeggiator.cpp
    Source/Arpeggiator.cpp
)
target_compile_definitions(TestArpeggiator PRIVATE
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
)
target_include_directories(TestArpeggiator PRIVATE ${JUCE_DIR}/modules)
target_link_libraries(TestArpeggiator PRIVATE
    juce::juce_core
)

# Test PitchDetector
add_executable(TestPitchDetector
    Tests/TestPitchDetector.cpp
    Source/PitchDetector.cpp
)
target_compile_definitions(TestPitchDetector PRIVATE
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
)
target_include_directories(TestPitchDetector PRIVATE
    ${JUCE_DIR}/modules
    ${AUBIO_DIR}
)
target_link_libraries(TestPitchDetector PRIVATE
    juce::juce_core
    juce::juce_audio_basics
    aubio
)

# Test ChordRecognizer
add_executable(TestChordRecognizer
    Tests/TestChordRecognizer.cpp
    Source/ChordRecognizer.cpp
    Source/Arpeggiator.cpp
)
target_compile_definitions(TestChordRecognizer PRIVATE
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
)
target_include_directories(TestChordRecognizer PRIVATE ${JUCE_DIR}/modules)
target_link_libraries(TestChordRecognizer PRIVATE
    juce::juce_core
)

# Test Chord Detection integration
add_executable(TestChordDetection
    Tests/e2e/test_chord_detect.cpp
)
target_compile_definitions(TestChordDetection PRIVATE
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
)
target_include_directories(TestChordDetection PRIVATE
    ${JUCE_DIR}/modules
    ${FLUIDSYNTH_DIR}
    ${AUBIO_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Source
)
target_link_libraries(TestChordDetection PRIVATE
    Lolooper
)

# Test PluginProcessor (integration test — links against full Lolooper lib)
add_executable(TestPluginProcessor
    Tests/TestPluginProcessor.cpp
)
target_compile_definitions(TestPluginProcessor PRIVATE
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
)
target_include_directories(TestPluginProcessor PRIVATE ${JUCE_DIR}/modules ${FLUIDSYNTH_DIR} ${AUBIO_DIR})
target_link_libraries(TestPluginProcessor PRIVATE
    Lolooper
)
