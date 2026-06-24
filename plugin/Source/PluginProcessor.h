/*
 * Copyright (c) 2025 Fernando Passos. All rights reserved.
 * Licensed under the MIT License.
 *
 * Lolooper — Live performance looper for Brazilian music
 */

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include "Sequencer.h"
#include "SampleLib.h"
#include "Mixer.h"
#include "Pattern.h"
#include "InstrumentLib.h"
#include "Arpeggiator.h"
#include "PitchDetector.h"
#include "ChordRecognizer.h"

//==============================================================================
/** The main audio processor for the Lolooper VST3/Standalone plugin.
 *
 *  Handles MIDI input, drives the Sequencer + Mixer + SampleLib,
 *  and manages all VST3-automatable parameters.
 *
 *  Communicates with the PWA via MIDI (CC/Note/SysEx).
 */
class LolooperAudioProcessor : public juce::AudioProcessor
{
public:
    //==============================================================================
    LolooperAudioProcessor();
    ~LolooperAudioProcessor() override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    //==============================================================================
    /** VST3-automatable parameters (public for the editor to bind) */
    juce::AudioParameterFloat* m_bpmParam;
    juce::AudioParameterFloat* m_swingParam;
    juce::AudioParameterFloat* m_humanizeParam;
    juce::AudioParameterChoice* m_patternStyleParam;

    juce::AudioParameterFloat* m_trackVolume[14];
    juce::AudioParameterFloat* m_trackPan[14];
    juce::AudioParameterBool* m_trackMute[14];
    juce::AudioParameterBool* m_trackSolo[14];
    juce::AudioParameterFloat* m_trackAccent[14];

    //==============================================================================
    /** Song mode section definition. */
    struct SongSection
    {
        juce::String name;
        juce::String patternStyle;
        int bars = 0;               // how many bars this section lasts
        bool loop = false;           // if true, loop this section
    };

    //==============================================================================
    /** Engine components (public for editor access) */
    Sequencer m_sequencer;
    SampleLib m_sampleLib;
    Mixer m_mixer;
    PatternLibrary m_patternLibrary;
    InstrumentLib m_instrumentLib[3];  // tracks 11,12,13
    ChordProgression m_chordProgression;
    PitchDetector m_pitchDetector;
    ChordRecognizer m_chordRecognizer;

    //==============================================================================
    /** Pattern data: current active grid reference */
    float m_patternGrid[14][16] = {};
    int m_currentPatternStyle = 0;

    //==============================================================================
    /** Transport control (public for editor access) */
    bool m_isPlaying = false;
    bool m_isRecording = false;

    //==============================================================================
    /** Send SysEx transport state message (called from processBlock each beat). */
    void sendTransportState(juce::MidiBuffer& midiMessages);
    void sendSongFeedback(juce::MidiBuffer& midiMessages);

    /** Send recorded pattern via SysEx 0x07 (called when REC stops). */
    void sendRecordedPattern(juce::MidiBuffer& midiMessages);

    /** Send detected chord to PWA via SysEx 0x09. */
    void sendChordDetected(const juce::String& chordName, juce::MidiBuffer& midiMessages);

    //==============================================================================
    /** Load default patterns from internal JSON data. */
    void loadDefaultPatterns();

    /** Song mode navigation. */
    void advanceToNextSection();
    void goToPreviousSection();
    int getCurrentSectionBars() const;
    bool isCurrentSectionLooping() const;
    void loadSongSectionPattern(int sectionIndex);

    /** Enable/disable audio chord detection. */
    void setChordDetectEnabled(bool enabled) noexcept { m_chordDetectEnabled = enabled; }
    bool isChordDetectEnabled() const noexcept { return m_chordDetectEnabled; }

    /** Song mode state (public for tests/editor). */
    int getCurrentSectionIndex() const noexcept { return m_currentSectionIndex; }
    int getBarsInCurrentSection() const noexcept { return m_barsInCurrentSection; }
    int getNumSongSections() const noexcept { return m_songSections.size(); }
    bool isSongModeActive() const noexcept { return m_songModeActive; }
    int getRecBufferSize() const noexcept { return m_recBuffer.size(); }

    /** Reload m_patternGrid from PatternLibrary for current style */
    void syncPatternGridFromLibrary();

    //==============================================================================
    /** Track names (public for editor/external use) */
    static const juce::String trackNames[14];
    static const juce::StringArray styleNames;

private:
    //==============================================================================
    double m_sampleRate = 44100.0;
    int m_samplesPerBlock = 512;

    // SysEx timing: send transport state every beat (4 steps)
    int m_sysExBeatCounter = 0;
    bool m_pendingRecPatternSend = false;

    //==============================================================================
    // REC mode
    struct RecordedHit {
        int track;
        int step;
        float velocity;
    };
    juce::Array<RecordedHit> m_recBuffer;
    bool m_recOverdub = false;  // true = merge with existing, false = overwrite

    //==============================================================================
    // Song mode
    juce::Array<SongSection> m_songSections;
    int m_currentSectionIndex = 0;
    int m_barsInCurrentSection = 0;  // bars elapsed in current section
    bool m_songModeActive = false;

    // Chord detection
    bool m_chordDetectEnabled = true;
    int m_chordDetectCounter = 0;
    static constexpr int kChordDetectInterval = 16; // ~11ms at 512 buf @44.1k

    /** Handle incoming SysEx messages. */
    void handleSysEx(const juce::MidiMessage& msg, juce::MidiBuffer& out);

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LolooperAudioProcessor)
};
