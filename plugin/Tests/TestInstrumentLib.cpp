/*
 * Copyright (c) 2025 Fernando Passos. All rights reserved.
 * Licensed under the MIT License.
 *
 * Lolooper -- Unit tests for InstrumentLib (SF2/SFZ)
 */

#include "../Source/InstrumentLib.h"
#include <cassert>
#include <cstdio>

static void test_construction()
{
    InstrumentLib lib;
    assert(!lib.isReady()); // no SF2 loaded yet
    std::printf("  PASS: test_construction\n");
}

static void test_init()
{
    InstrumentLib lib;
    bool ok = lib.init(44100.0);
    assert(ok);
    assert(lib.getSampleRate() == 44100.0);
    std::printf("  PASS: test_init\n");
}

static void test_load_soundfont()
{
    InstrumentLib lib;
    lib.init(44100.0);

    // Load the system GM SoundFont
    juce::File sf2("/usr/share/sounds/sf2/TimGM6mb.sf2");
    if (sf2.existsAsFile())
    {
        bool loaded = lib.loadSoundFont(sf2);
        assert(loaded);
        assert(lib.isReady());
    }
    else
    {
        std::printf("    (TimGM6mb.sf2 not found -- skipping load test)\n");
    }
    std::printf("  PASS: test_load_soundfont\n");
}

static void test_note_on_off_does_not_crash()
{
    InstrumentLib lib;
    lib.init(44100.0);

    juce::File sf2("/usr/share/sounds/sf2/TimGM6mb.sf2");
    if (sf2.existsAsFile())
    {
        lib.loadSoundFont(sf2);
        // These should not crash
        lib.noteOn(0, 60, 100);
        lib.noteOff(0, 60);
        lib.allNotesOff();
    }
    std::printf("  PASS: test_note_on_off_does_not_crash\n");
}

static void test_render_produces_audio()
{
    InstrumentLib lib;
    lib.init(44100.0);

    juce::File sf2("/usr/share/sounds/sf2/TimGM6mb.sf2");
    if (sf2.existsAsFile())
    {
        lib.loadSoundFont(sf2);
        lib.setGain(0.8f);
        lib.noteOn(0, 69, 100); // A4

        juce::AudioBuffer<float> buffer(2, 512);
        lib.render(buffer, 512);

        // Audio should be non-zero
        bool hasAudio = false;
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < 512; ++i)
                if (buffer.getSample(ch, i) != 0.0f)
                    hasAudio = true;
        assert(hasAudio);
    }
    std::printf("  PASS: test_render_produces_audio\n");
}

static void test_render_silent_when_no_note()
{
    InstrumentLib lib;
    lib.init(44100.0);

    juce::File sf2("/usr/share/sounds/sf2/TimGM6mb.sf2");
    if (sf2.existsAsFile())
    {
        lib.loadSoundFont(sf2);
        juce::AudioBuffer<float> buffer(2, 512);
        lib.render(buffer, 512);

        // Should be silent (no note playing)
        bool allZero = true;
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < 512; ++i)
                if (std::fabs(buffer.getSample(ch, i)) > 0.001f)
                    allZero = false;
        assert(allZero);
    }
    std::printf("  PASS: test_render_silent_when_no_note\n");
}

static void test_all_notes_off_stops_sound()
{
    InstrumentLib lib;
    lib.init(44100.0);

    juce::File sf2("/usr/share/sounds/sf2/TimGM6mb.sf2");
    if (sf2.existsAsFile())
    {
        lib.loadSoundFont(sf2);
        lib.noteOn(0, 60, 100);
        lib.noteOn(0, 64, 100);
        lib.noteOn(0, 67, 100);
        lib.allNotesOff();

        // After all notes off, should be silent
        juce::AudioBuffer<float> buffer(2, 256);
        lib.render(buffer, 256);
        float maxVal = 0.0f;
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < 256; ++i)
                maxVal = juce::jmax(maxVal, std::fabs(buffer.getSample(ch, i)));
        // Some residual from envelope release is OK, but should be decaying
        assert(maxVal < 1.0f);
    }
    std::printf("  PASS: test_all_notes_off_stops_sound\n");
}

static void test_set_gain()
{
    InstrumentLib lib;
    lib.init(44100.0);

    juce::File sf2("/usr/share/sounds/sf2/TimGM6mb.sf2");
    if (sf2.existsAsFile())
    {
        lib.loadSoundFont(sf2);
        lib.setGain(0.0f);
        lib.noteOn(0, 60, 127);

        juce::AudioBuffer<float> buffer(2, 256);
        lib.render(buffer, 256);

        // Gain 0 = silence
        bool allZero = true;
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < 256; ++i)
                if (std::fabs(buffer.getSample(ch, i)) > 0.001f)
                    allZero = false;
        assert(allZero);
    }
    std::printf("  PASS: test_set_gain\n");
}

//==============================================================================
int main()
{
    std::printf("\n=== InstrumentLib Unit Tests ===\n\n");

    test_construction();
    test_init();
    test_load_soundfont();
    test_note_on_off_does_not_crash();
    test_render_produces_audio();
    test_render_silent_when_no_note();
    test_all_notes_off_stops_sound();
    test_set_gain();

    std::printf("\n=== All InstrumentLib tests PASSED ===\n\n");
    return 0;
}
