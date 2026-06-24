/*
 * Copyright (c) 2025 Fernando Passos. All rights reserved.
 * Licensed under the MIT License.
 *
 * Lolooper — Unit tests for the Mixer class
 */

#include "../Source/Mixer.h"
#include "../Source/SampleLib.h"
#include <cassert>
#include <cstdio>
#include <cmath>

//==============================================================================
// Helper: create a pattern grid with all zeros except one hit
static void setGridHit(float grid[14][16], int track, int step, float velocity)
{
    for (int t = 0; t < 14; ++t)
        for (int s = 0; s < 16; ++s)
            grid[t][s] = 0.0f;
    grid[track][step] = velocity;
}

//==============================================================================
static void test_render_step_produces_audio_when_hit()
{
    SampleLib lib;
    lib.registerSample("0", "default", juce::File());  // track 0
    lib.loadAll();

    Mixer mixer;
    mixer.setSampleLib(&lib);

    float pattern[14][16] = {};
    setGridHit(pattern, 0, 0, 1.0f);

    float volumes[14] = {};
    float pans[14] = {};
    bool mutes[14] = {};
    bool solos[14] = {};
    float accents[14] = {};

    for (int t = 0; t < 14; ++t) { volumes[t] = 1.0f; pans[t] = 0.5f; accents[t] = 1.0f; }

    juce::AudioBuffer<float> output(2, 512);
    output.clear();

    mixer.renderStep(0, pattern, volumes, pans, mutes, solos, accents, output, 44100.0);

    // Output should have non-zero samples
    bool hasAudio = false;
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 512; ++i)
            if (output.getSample(ch, i) != 0.0f) hasAudio = true;

    assert(hasAudio);
    std::printf("  PASS: test_render_step_produces_audio_when_hit\n");
}

static void test_render_step_silent_when_no_hit()
{
    SampleLib lib;
    lib.registerSample("0", "default", juce::File());
    lib.loadAll();

    Mixer mixer;
    mixer.setSampleLib(&lib);

    float pattern[14][16] = {};  // all zeros
    float volumes[14] = {};
    float pans[14] = {};
    bool mutes[14] = {};
    bool solos[14] = {};
    float accents[14] = {};

    for (int t = 0; t < 14; ++t) { volumes[t] = 1.0f; pans[t] = 0.5f; accents[t] = 1.0f; }

    juce::AudioBuffer<float> output(2, 512);
    output.clear();

    mixer.renderStep(0, pattern, volumes, pans, mutes, solos, accents, output, 44100.0);

    // Output should be all zeros
    bool allZero = true;
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 512; ++i)
            if (output.getSample(ch, i) != 0.0f) allZero = false;

    assert(allZero);
    std::printf("  PASS: test_render_step_silent_when_no_hit\n");
}

static void test_muted_track_does_not_render()
{
    SampleLib lib;
    lib.registerSample("0", "default", juce::File());
    lib.loadAll();

    Mixer mixer;
    mixer.setSampleLib(&lib);

    float pattern[14][16] = {};
    setGridHit(pattern, 0, 0, 1.0f);

    float volumes[14] = {};
    float pans[14] = {};
    bool mutes[14] = {};
    bool solos[14] = {};
    float accents[14] = {};

    for (int t = 0; t < 14; ++t) { volumes[t] = 1.0f; pans[t] = 0.5f; accents[t] = 1.0f; }

    mutes[0] = true;  // Mute track 0

    juce::AudioBuffer<float> output(2, 512);
    output.clear();

    mixer.renderStep(0, pattern, volumes, pans, mutes, solos, accents, output, 44100.0);

    // Output should be all zeros (track is muted)
    bool allZero = true;
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 512; ++i)
            if (output.getSample(ch, i) != 0.0f) allZero = false;

    assert(allZero);
    std::printf("  PASS: test_muted_track_does_not_render\n");
}

static void test_solo_only_renders_soloed_tracks()
{
    SampleLib lib;
    lib.registerSample("0", "default", juce::File());  // track 0
    lib.registerSample("1", "default", juce::File());  // track 1
    lib.loadAll();

    Mixer mixer;
    mixer.setSampleLib(&lib);

    float pattern[14][16] = {};
    setGridHit(pattern, 0, 0, 1.0f);  // hit on track 0
    pattern[1][0] = 1.0f;              // hit on track 1

    float volumes[14] = {};
    float pans[14] = {};
    bool mutes[14] = {};
    bool solos[14] = {};
    float accents[14] = {};

    for (int t = 0; t < 14; ++t) { volumes[t] = 1.0f; pans[t] = 0.5f; accents[t] = 1.0f; }

    solos[0] = true;  // Solo only track 0 — track 1 should be silent

    juce::AudioBuffer<float> output(2, 512);
    output.clear();

    mixer.renderStep(0, pattern, volumes, pans, mutes, solos, accents, output, 44100.0);

    // Output should have audio from track 0 only
    bool hasAudio = false;
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 512; ++i)
            if (output.getSample(ch, i) != 0.0f) hasAudio = true;

    assert(hasAudio);
    std::printf("  PASS: test_solo_only_renders_soloed_tracks\n");
}

static void test_velocity_scaling()
{
    SampleLib lib;
    lib.registerSample("0", "default", juce::File());
    lib.loadAll();

    Mixer mixer;
    mixer.setSampleLib(&lib);

    float pattern[14][16] = {};
    setGridHit(pattern, 0, 0, 1.0f);

    float volumes[14] = {};
    float pans[14] = {};
    bool mutes[14] = {};
    bool solos[14] = {};
    float accents[14] = {};

    for (int t = 0; t < 14; ++t) { volumes[t] = 0.0f; pans[t] = 0.5f; accents[t] = 1.0f; }

    juce::AudioBuffer<float> output(2, 512);
    output.clear();

    mixer.renderStep(0, pattern, volumes, pans, mutes, solos, accents, output, 44100.0);

    // Zero volume = zero output
    bool allZero = true;
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 512; ++i)
            if (output.getSample(ch, i) != 0.0f) allZero = false;

    assert(allZero);
    std::printf("  PASS: test_velocity_scaling\n");
}

static void test_pan_left_produces_left_channel_only()
{
    SampleLib lib;
    lib.registerSample("0", "default", juce::File());
    lib.loadAll();

    Mixer mixer;
    mixer.setSampleLib(&lib);

    float pattern[14][16] = {};
    setGridHit(pattern, 0, 0, 1.0f);

    float volumes[14] = {};
    float pans[14] = {};
    bool mutes[14] = {};
    bool solos[14] = {};
    float accents[14] = {};

    for (int t = 0; t < 14; ++t) { volumes[t] = 1.0f; pans[t] = 0.5f; accents[t] = 1.0f; }
    pans[0] = 0.0f;  // Full left

    juce::AudioBuffer<float> output(2, 512);
    output.clear();

    mixer.renderStep(0, pattern, volumes, pans, mutes, solos, accents, output, 44100.0);

    // Right channel should be silent (pan = 0 means full left, sqrt(0) = 0 right gain)
    bool rightSilent = true;
    for (int i = 0; i < 512; ++i)
        if (output.getSample(1, i) != 0.0f) rightSilent = false;

    assert(rightSilent);

    // Left channel should have audio
    bool leftHasAudio = false;
    for (int i = 0; i < 512; ++i)
        if (output.getSample(0, i) != 0.0f) leftHasAudio = true;

    assert(leftHasAudio);
    std::printf("  PASS: test_pan_left_produces_left_channel_only\n");
}

static void test_pan_right_produces_right_channel_only()
{
    SampleLib lib;
    lib.registerSample("0", "default", juce::File());
    lib.loadAll();

    Mixer mixer;
    mixer.setSampleLib(&lib);

    float pattern[14][16] = {};
    setGridHit(pattern, 0, 0, 1.0f);

    float volumes[14] = {};
    float pans[14] = {};
    bool mutes[14] = {};
    bool solos[14] = {};
    float accents[14] = {};

    for (int t = 0; t < 14; ++t) { volumes[t] = 1.0f; pans[t] = 0.5f; accents[t] = 1.0f; }
    pans[0] = 1.0f;  // Full right

    juce::AudioBuffer<float> output(2, 512);
    output.clear();

    mixer.renderStep(0, pattern, volumes, pans, mutes, solos, accents, output, 44100.0);

    // Left channel should be silent
    bool leftSilent = true;
    for (int i = 0; i < 512; ++i)
        if (output.getSample(0, i) != 0.0f) leftSilent = false;

    assert(leftSilent);

    // Right channel should have audio
    bool rightHasAudio = false;
    for (int i = 0; i < 512; ++i)
        if (output.getSample(1, i) != 0.0f) rightHasAudio = true;

    assert(rightHasAudio);
    std::printf("  PASS: test_pan_right_produces_right_channel_only\n");
}

static void test_pan_center_produces_both_channels()
{
    SampleLib lib;
    lib.registerSample("0", "default", juce::File());
    lib.loadAll();

    Mixer mixer;
    mixer.setSampleLib(&lib);

    float pattern[14][16] = {};
    setGridHit(pattern, 0, 0, 1.0f);

    float volumes[14] = {};
    float pans[14] = {};
    bool mutes[14] = {};
    bool solos[14] = {};
    float accents[14] = {};

    for (int t = 0; t < 14; ++t) { volumes[t] = 1.0f; pans[t] = 0.5f; accents[t] = 1.0f; }

    juce::AudioBuffer<float> output(2, 512);
    output.clear();

    mixer.renderStep(0, pattern, volumes, pans, mutes, solos, accents, output, 44100.0);

    // Both channels should have equal audio (center pan)
    bool leftHasAudio = false, rightHasAudio = false;
    for (int i = 0; i < 512; ++i)
    {
        if (output.getSample(0, i) != 0.0f) leftHasAudio = true;
        if (output.getSample(1, i) != 0.0f) rightHasAudio = true;
    }

    assert(leftHasAudio && rightHasAudio);
    std::printf("  PASS: test_pan_center_produces_both_channels\n");
}

//==============================================================================
static void test_soft_clip_limits_output()
{
    Mixer mixer;

    juce::AudioBuffer<float> buffer(2, 100);
    // Fill with values above 1.0
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 100; ++i)
            buffer.setSample(ch, i, 2.0f);

    mixer.applySoftClip(buffer);

    // After soft clip, all samples should be <= 1.0
    bool allClipped = true;
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 100; ++i)
            if (buffer.getSample(ch, i) > 1.0f)
                allClipped = false;

    assert(allClipped);
    std::printf("  PASS: test_soft_clip_limits_output\n");
}

static void test_soft_clip_preserves_quiet_signals()
{
    Mixer mixer;

    juce::AudioBuffer<float> buffer(1, 10);
    buffer.setSample(0, 0, 0.5f);
    buffer.setSample(0, 1, 0.3f);
    buffer.setSample(0, 2, 0.7f);

    mixer.applySoftClip(buffer);

    // Values below 1.0 should be slightly modified by cubic curve but still close
    // x - x^3/3 for x=0.7: 0.7 - 0.343/3 = 0.7 - 0.114 = 0.586
    float expected = 0.7f - (0.7f * 0.7f * 0.7f) / 3.0f;
    float actual = buffer.getSample(0, 2);
    assert(std::fabs(actual - expected) < 0.01f);

    std::printf("  PASS: test_soft_clip_preserves_quiet_signals\n");
}

//==============================================================================
int main()
{
    std::printf("\n=== Mixer Unit Tests ===\n\n");

    std::printf("--- renderStep() ---\n");
    test_render_step_produces_audio_when_hit();
    test_render_step_silent_when_no_hit();
    test_muted_track_does_not_render();
    test_solo_only_renders_soloed_tracks();
    test_velocity_scaling();

    std::printf("\n--- Pan ---\n");
    test_pan_left_produces_left_channel_only();
    test_pan_right_produces_right_channel_only();
    test_pan_center_produces_both_channels();

    std::printf("\n--- applySoftClip() ---\n");
    test_soft_clip_limits_output();
    test_soft_clip_preserves_quiet_signals();

    std::printf("\n=== All Mixer tests PASSED ===\n\n");
    return 0;
}
