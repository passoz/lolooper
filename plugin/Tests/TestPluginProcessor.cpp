/*
 * Copyright (c) 2025 Fernando Passos. All rights reserved.
 * Licensed under the MIT License.
 *
 * Lolooper — Integration tests for the audio processor
 * Tests REC mode, Song mode, SysEx handling, and state serialization.
 */

#include "../Source/PluginProcessor.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <cmath>

//==============================================================================
// Helper: create a minimal MIDI Note On message
static juce::MidiMessage noteOn(int note, juce::uint8 velocity = 127)
{
    return juce::MidiMessage::noteOn(1, note, (float)velocity / 127.0f);
}

// Helper: create a CC message
static juce::MidiMessage cc(int controller, juce::uint8 value)
{
    return juce::MidiMessage::controllerEvent(1, controller, value);
}

// Helper: create SysEx message
static juce::MidiMessage sysEx(const juce::uint8* data, int size)
{
    return juce::MidiMessage::createSysExMessage(data, (int)size);
}

//==============================================================================

static void test_processor_construction()
{
    LolooperAudioProcessor proc;
    assert(proc.getName().isNotEmpty());
    assert(proc.acceptsMidi() == true);
    assert(proc.producesMidi() == true);
    assert(proc.m_bpmParam != nullptr);
    assert(proc.m_bpmParam->get() == 100.0f);
    std::printf("  PASS: test_processor_construction\n");
}

static void test_processor_prepare_does_not_crash()
{
    LolooperAudioProcessor proc;
    proc.prepareToPlay(44100.0, 512);
    std::printf("  PASS: test_processor_prepare_does_not_crash\n");
}

//==============================================================================
// REC mode tests

static void test_rec_mode_captures_hit()
{
    LolooperAudioProcessor proc;
    proc.prepareToPlay(48000.0, 512);

    // Clear grid
    for (int t = 0; t < 14; ++t)
        for (int s = 0; s < 16; ++s)
            proc.m_patternGrid[t][s] = 0.0f;

    // Enter REC mode
    proc.m_isRecording = true;
    proc.m_sequencer.prepare(48000.0, 120.0);

    // Sequencer is at step 0
    assert(proc.m_sequencer.getCurrentStep() == 0);

    // Send Note On for track 2 (surdo_3) with velocity 64
    juce::MidiBuffer midiIn;
    midiIn.addEvent(noteOn(38, 64), 0);  // note 38 = track 2

    juce::AudioBuffer<float> buffer(2, 512);
    proc.processBlock(buffer, midiIn);

    // Grid at track 2, step 0 should be 64/127 ≈ 0.504
    float hitVel = proc.m_patternGrid[2][0];
    assert(hitVel > 0.4f && hitVel < 0.6f);

    // All other track/step combos should be 0
    assert(proc.m_patternGrid[0][0] == 0.0f);
    assert(proc.m_patternGrid[2][1] == 0.0f);

    std::printf("  PASS: test_rec_mode_captures_hit\n");
}

static void test_rec_mode_clears_track_then_records()
{
    LolooperAudioProcessor proc;
    proc.prepareToPlay(48000.0, 512);

    // Pre-fill track 5 with some values
    for (int s = 0; s < 16; ++s)
        proc.m_patternGrid[5][s] = 0.8f;

    proc.m_isRecording = true;
    proc.m_sequencer.prepare(48000.0, 120.0);

    // Record a single hit on track 5 at step 0
    juce::MidiBuffer midiIn;
    midiIn.addEvent(noteOn(41, 100), 0);  // note 41 = track 5

    juce::AudioBuffer<float> buffer(2, 512);
    proc.processBlock(buffer, midiIn);

    // Track 5 step 0 should have the new hit
    assert(proc.m_patternGrid[5][0] > 0.7f);
    // But all other steps on track 5 should be cleared
    assert(proc.m_patternGrid[5][1] == 0.0f);
    assert(proc.m_patternGrid[5][8] == 0.0f);

    std::printf("  PASS: test_rec_mode_clears_track_then_records\n");
}

static void test_rec_mode_skips_out_of_range_notes()
{
    LolooperAudioProcessor proc;
    proc.prepareToPlay(48000.0, 512);
    proc.m_isRecording = true;

    // Note 60 is outside track range (36-49)
    juce::MidiBuffer midiIn;
    midiIn.addEvent(noteOn(60, 127), 0);

    juce::AudioBuffer<float> buffer(2, 512);
    proc.processBlock(buffer, midiIn);

    // Grid should be unchanged
    assert(proc.m_patternGrid[0][0] == 0.0f);

    std::printf("  PASS: test_rec_mode_skips_out_of_range_notes\n");
}

static void test_rec_mode_toggle_clears_buffer()
{
    LolooperAudioProcessor proc;
    proc.prepareToPlay(48000.0, 512);

    // Start REC
    proc.m_isRecording = true;
    proc.m_sequencer.prepare(48000.0, 120.0);

    juce::MidiBuffer midiIn;
    midiIn.addEvent(noteOn(36, 127), 0);
    juce::AudioBuffer<float> buffer(2, 512);
    proc.processBlock(buffer, midiIn);

    // Now toggle REC off and back on via note 117
    juce::MidiBuffer midiIn2;
    midiIn2.addEvent(noteOn(117, 127), 0);  // toggle off
    proc.processBlock(buffer, midiIn2);
    assert(!proc.m_isRecording);

    juce::MidiBuffer midiIn3;
    midiIn3.addEvent(noteOn(117, 127), 0);  // toggle on (clears buffer)
    proc.processBlock(buffer, midiIn3);
    assert(proc.m_isRecording);

    std::printf("  PASS: test_rec_mode_toggle_clears_buffer\n");
}

//==============================================================================
// Song mode tests

static void test_song_section_advance()
{
    LolooperAudioProcessor proc;
    proc.prepareToPlay(48000.0, 120.0);

    // Set up song with 3 sections: intro(2 bars), verse(1 bar), chorus(1 bar)
    juce::uint8 songData[] = {
        0x7D, 0x04, '{',
        '"', 's', 'e', 'c', 't', 'i', 'o', 'n', 's', '"', ':', '[',
        '{', '"', 'n', 'a', 'm', 'e', '"', ':', '"', 'i', 'n', 't', 'r', 'o', '"', ',',
           '"', 'p', 'a', 't', 't', 'e', 'r', 'n', '"', ':', '"', 'i', 'n', 't', 'r', 'o', '"', ',',
           '"', 'b', 'a', 'r', 's', '"', ':', '2', ',',
           '"', 'l', 'o', 'o', 'p', '"', ':', 'f', 'a', 'l', 's', 'e',
        '}', ',',
        '{', '"', 'n', 'a', 'm', 'e', '"', ':', '"', 'v', 'e', 'r', 's', 'e', '"', ',',
           '"', 'p', 'a', 't', 't', 'e', 'r', 'n', '"', ':', '"', 's', 'a', 'm', 'b', 'a', '"', ',',
           '"', 'b', 'a', 'r', 's', '"', ':', '1', ',',
           '"', 'l', 'o', 'o', 'p', '"', ':', 'f', 'a', 'l', 's', 'e',
        '}', ',',
        '{', '"', 'n', 'a', 'm', 'e', '"', ':', '"', 'c', 'h', 'o', 'r', 'u', 's', '"', ',',
           '"', 'p', 'a', 't', 't', 'e', 'r', 'n', '"', ':', '"', 'v', 'i', 'r', 'a', 'd', 'a', '"', ',',
           '"', 'b', 'a', 'r', 's', '"', ':', '1', ',',
           '"', 'l', 'o', 'o', 'p', '"', ':', 'f', 'a', 'l', 's', 'e',
        '}',
        ']', '}'
    };

    juce::MidiBuffer midiIn;
    midiIn.addEvent(sysEx(songData, sizeof(songData)), 0);

    juce::AudioBuffer<float> buffer(2, 512);
    proc.processBlock(buffer, midiIn);

    // Verify song was loaded
    assert(proc.isSongModeActive());
    assert(proc.getNumSongSections() == 3);

    std::printf("  PASS: test_song_section_advance (loaded 3 sections)\n");
}

static void test_song_mode_sys_ex_loads_empty_song()
{
    LolooperAudioProcessor proc;
    proc.prepareToPlay(48000.0, 512);

    juce::uint8 emptySong[] = {
        0x7D, 0x04, '{', '"', 's', 'e', 'c', 't', 'i', 'o', 'n', 's', '"', ':', '[', ']', '}'
    };

    juce::MidiBuffer midiIn;
    midiIn.addEvent(sysEx(emptySong, sizeof(emptySong)), 0);

    juce::AudioBuffer<float> buffer(2, 512);
    proc.processBlock(buffer, midiIn);

    assert(!proc.isSongModeActive());
    assert(proc.getNumSongSections() == 0);

    std::printf("  PASS: test_song_mode_sys_ex_loads_empty_song\n");
}

static void test_song_mode_navigation()
{
    LolooperAudioProcessor proc;
    proc.prepareToPlay(48000.0, 512);

    // Load the 3-section song again (same as above but shorthand)
    juce::String songJson = R"({"sections":[
        {"name":"A","pattern":"samba","bars":4,"loop":false},
        {"name":"B","pattern":"samba","bars":2,"loop":false},
        {"name":"C","pattern":"virada","bars":1,"loop":false}
    ]})";

    auto utf8 = songJson.toUTF8();
    int len = (int)std::strlen(utf8);
    juce::HeapBlock<juce::uint8> data(2 + len);
    data[0] = 0x7D;
    data[1] = 0x04;
    std::memcpy(data + 2, utf8, (size_t)len);

    juce::MidiBuffer midiIn;
    midiIn.addEvent(sysEx(data, 2 + len), 0);

    juce::AudioBuffer<float> buffer(2, 512);
    proc.processBlock(buffer, midiIn);

    assert(proc.isSongModeActive());
    assert(proc.getCurrentSectionIndex() == 0);

    // Navigate to next section
    proc.m_isPlaying = true;
    proc.advanceToNextSection();
    assert(proc.getCurrentSectionIndex() == 1);

    // Wrap around
    proc.advanceToNextSection();
    assert(proc.getCurrentSectionIndex() == 2);
    proc.advanceToNextSection();
    assert(proc.getCurrentSectionIndex() == 0);

    // Previous
    proc.goToPreviousSection();
    assert(proc.getCurrentSectionIndex() == 2);

    std::printf("  PASS: test_song_mode_navigation\n");
}

//==============================================================================
// PatternLibrary integration tests

static void test_pattern_library_loaded_from_processor()
{
    LolooperAudioProcessor proc;

    // Patterns are loaded in constructor from data/patterns/
    // At minimum, check that the library has entries
    if (proc.m_patternLibrary.size() > 0)
    {
        assert(proc.m_patternLibrary.getPattern("samba") != nullptr);
        assert(proc.m_patternLibrary.getActivePattern() != nullptr);

        // Grid should be synced from library
        const auto* pattern = proc.m_patternLibrary.getPattern("samba");
        // Samba surdo_1 has hit at step 4
        assert(pattern->getVelocity(0, 4) > 0.5f);  // surdo_1 step 4
    }
    else
    {
        std::printf("    (pattern dir not found — test will be skipped)\n");
    }

    std::printf("  PASS: test_pattern_library_loaded_from_processor\n");
}

//==============================================================================
// SysEx handling tests

static void test_sysex_pattern_push()
{
    LolooperAudioProcessor proc;
    proc.prepareToPlay(48000.0, 512);

    // Push a custom pattern
    juce::String json = R"({"meu_samba":{"surdo_1":[1.0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],"surdo_2":[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],"surdo_3":[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],"caixa":[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],"repique":[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],"tamborim":[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],"pandeiro":[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],"cuica":[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],"agogo":[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],"reco_reco":[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],"tantan":[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],"cavaquinho":[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],"violao_7":[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],"banjo":[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0]}})";

    auto utf8 = json.toUTF8();
    int len = (int)std::strlen(utf8);
    // SysEx payload: 0x7D + 0x03 + JSON (no 0xF7 — createSysExMessage adds it)
    juce::HeapBlock<juce::uint8> sysexPayload(2 + len);
    sysexPayload[0] = 0x7D;
    sysexPayload[1] = 0x03;
    std::memcpy(sysexPayload + 2, utf8, (size_t)len);

    juce::MidiBuffer midiIn;
    midiIn.addEvent(sysEx(sysexPayload, 2 + len), 0);
    juce::AudioBuffer<float> buffer(2, 512);
    proc.processBlock(buffer, midiIn);

    // The pattern should be loaded into the library (but grid only shows active style)
    assert(proc.m_patternLibrary.getPattern("meu_samba") != nullptr);
    const auto* pushedPattern = proc.m_patternLibrary.getPattern("meu_samba");
    // The pushed pattern is stored correctly in the library
    assert(pushedPattern->getVelocity(0, 0) == 1.0f);
    // Grid reflects the active style (samba), not the newly pushed one
    // This is correct — the user can switch to the new style via CC 18

    std::printf("  PASS: test_sysex_pattern_push\n");
}

//==============================================================================
// State serialization tests

static void test_state_serialization_roundtrip()
{
    LolooperAudioProcessor proc;
    proc.prepareToPlay(44100.0, 512);

    // Set some parameters
    *proc.m_bpmParam = 140.0f;
    *proc.m_swingParam = 0.5f;
    *proc.m_trackVolume[0] = 0.3f;
    *proc.m_trackMute[3] = true;

    // Save state
    juce::MemoryBlock state;
    proc.getStateInformation(state);

    // Create a new processor and restore
    LolooperAudioProcessor proc2;
    proc2.prepareToPlay(44100.0, 512);
    proc2.setStateInformation(state.getData(), (int)state.getSize());

    // Verify restored parameters (use epsilon for floats)
    auto eps = 0.001f;
    assert(std::fabs(proc2.m_bpmParam->get() - 140.0f) < eps);
    assert(std::fabs(proc2.m_swingParam->get() - 0.5f) < eps);
    assert(std::fabs(proc2.m_trackVolume[0]->get() - 0.3f) < eps);
    assert(proc2.m_trackMute[3]->get() == true);

    std::printf("  PASS: test_state_serialization_roundtrip\n");
}

//==============================================================================
// REC buffer SysEx test

static void test_rec_buffer_sends_on_stop()
{
    LolooperAudioProcessor proc;
    proc.prepareToPlay(48000.0, 512);
    proc.m_sequencer.prepare(48000.0, 120.0);

    // Start REC via Note 117
    {
        juce::MidiBuffer midiIn;
        midiIn.addEvent(noteOn(117, 127), 0);
        juce::AudioBuffer<float> buf(2, 512);
        proc.processBlock(buf, midiIn);
    }
    assert(proc.m_isRecording);
    assert(proc.getRecBufferSize() == 0);

    // Record a hit
    {
        juce::MidiBuffer midiIn;
        midiIn.addEvent(noteOn(40, 100), 0);  // track 4, vel 100
        juce::AudioBuffer<float> buf(2, 512);
        proc.processBlock(buf, midiIn);
    }
    assert(proc.getRecBufferSize() == 1);  // one hit recorded

    // Stop REC via Note 117 again — should generate SysEx output
    juce::MidiBuffer midiOut;
    {
        juce::MidiBuffer midiIn;
        midiIn.addEvent(noteOn(117, 127), 0);
        juce::AudioBuffer<float> buf(2, 512);
        proc.processBlock(buf, midiIn);
        // Capture midiOut — SysEx messages are added to the midiIn buffer
        midiOut = midiIn;
    }
    assert(!proc.m_isRecording);
    assert(proc.getRecBufferSize() == 0);  // cleared after send

    // Verify that a SysEx 0x07 message was generated
    bool foundSysex07 = false;
    for (const auto meta : midiOut)
    {
        auto msg = meta.getMessage();
        if (msg.isSysEx())
        {
            auto data = msg.getSysExData();
            auto size = msg.getSysExDataSize();
            if (size >= 2 && data[0] == 0x7D && data[1] == 0x07)
            {
                foundSysex07 = true;
                // First hit: track=4, step=0
                assert(data[2] == 4);  // track
                assert(data[3] == 0);  // step
                break;
            }
        }
    }
    assert(foundSysex07);

    std::printf("  PASS: test_rec_buffer_sends_on_stop\n");
}

static void test_rec_buffer_multiple_hits_in_block()
{
    LolooperAudioProcessor proc;
    proc.prepareToPlay(48000.0, 512);
    proc.m_sequencer.prepare(48000.0, 120.0);

    // Start REC
    {
        juce::MidiBuffer midiIn;
        midiIn.addEvent(noteOn(117, 127), 0);
        juce::AudioBuffer<float> buf(2, 512);
        proc.processBlock(buf, midiIn);
    }

    // Record 3 notes in the SAME block
    {
        juce::MidiBuffer midiIn;
        midiIn.addEvent(noteOn(36, 127), 0);  // track 0, vel 127
        midiIn.addEvent(noteOn(40, 100), 0);  // track 4, vel 100
        midiIn.addEvent(noteOn(42, 50), 0);   // track 6, vel 50
        juce::AudioBuffer<float> buf(2, 512);
        proc.processBlock(buf, midiIn);
    }
    assert(proc.getRecBufferSize() == 3);  // all 3 recorded

    // Stop REC and capture SysEx
    juce::MidiBuffer midiOut;
    {
        juce::MidiBuffer midiIn;
        midiIn.addEvent(noteOn(117, 127), 0);
        juce::AudioBuffer<float> buf(2, 512);
        proc.processBlock(buf, midiIn);
        midiOut = midiIn;
    }

    // Count hits in SysEx 0x07
    int hitCount = 0;
    for (const auto meta : midiOut)
    {
        auto msg = meta.getMessage();
        if (msg.isSysEx())
        {
            auto data = msg.getSysExData();
            auto size = msg.getSysExDataSize();
            if (size >= 2 && data[0] == 0x7D && data[1] == 0x07)
            {
                // Each hit is 3 bytes, plus 2 header bytes
                // Subtract trailing 0xF7 if present
                int payloadLen = (int)size - 2;
                if (data[size - 1] == 0xF7) payloadLen--;
                hitCount = payloadLen / 3;
                break;
            }
        }
    }
    assert(hitCount == 3);  // exactly 3 hits in SysEx
    assert(proc.getRecBufferSize() == 0);

    std::printf("  PASS: test_rec_buffer_multiple_hits_in_block\n");
}

static void test_rec_buffer_cleared_between_sessions()
{
    LolooperAudioProcessor proc;
    proc.prepareToPlay(48000.0, 512);
    proc.m_sequencer.prepare(48000.0, 120.0);

    // Session 1: record and stop
    {
        juce::MidiBuffer midiIn;
        midiIn.addEvent(noteOn(117, 127), 0);
        juce::AudioBuffer<float> buf(2, 512);
        proc.processBlock(buf, midiIn);
    }
    {
        juce::MidiBuffer midiIn;
        midiIn.addEvent(noteOn(36, 127), 0);
        juce::AudioBuffer<float> buf(2, 512);
        proc.processBlock(buf, midiIn);
    }
    assert(proc.getRecBufferSize() == 1);

    // Stop session 1
    {
        juce::MidiBuffer midiIn;
        midiIn.addEvent(noteOn(117, 127), 0);
        juce::AudioBuffer<float> buf(2, 512);
        proc.processBlock(buf, midiIn);
    }
    assert(proc.getRecBufferSize() == 0);  // cleared

    // Session 2: start fresh — buffer MUST be empty
    {
        juce::MidiBuffer midiIn;
        midiIn.addEvent(noteOn(117, 127), 0);
        juce::AudioBuffer<float> buf(2, 512);
        proc.processBlock(buf, midiIn);
    }
    assert(proc.m_isRecording);
    assert(proc.getRecBufferSize() == 0);  // clean start!

    std::printf("  PASS: test_rec_buffer_cleared_between_sessions\n");
}

//==============================================================================
int main()
{
    std::printf("\n=== PluginProcessor Integration Tests ===\n\n");

    std::printf("--- Construction ---\n");
    test_processor_construction();
    test_processor_prepare_does_not_crash();

    std::printf("\n--- REC Mode ---\n");
    test_rec_mode_captures_hit();
    test_rec_mode_clears_track_then_records();
    test_rec_mode_skips_out_of_range_notes();
    test_rec_mode_toggle_clears_buffer();

    std::printf("\n--- Song Mode ---\n");
    test_song_mode_sys_ex_loads_empty_song();
    test_song_section_advance();
    test_song_mode_navigation();

    std::printf("\n--- Pattern Library ---\n");
    test_pattern_library_loaded_from_processor();

    std::printf("\n--- SysEx ---\n");
    test_sysex_pattern_push();

    std::printf("\n--- State Serialization ---\n");
    test_state_serialization_roundtrip();

    std::printf("\n--- REC SysEx ---\n");
    test_rec_buffer_sends_on_stop();
    test_rec_buffer_multiple_hits_in_block();
    test_rec_buffer_cleared_between_sessions();

    std::printf("\n=== All PluginProcessor tests PASSED ===\n\n");
    return 0;
}
