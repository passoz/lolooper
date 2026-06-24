/*
 * Copyright (c) 2025 Fernando Passos. All rights reserved.
 * Licensed under the MIT License.
 *
 * Lolooper — Unit tests for the SampleLib class
 */

#include "../Source/SampleLib.h"
#include <cassert>
#include <cstdio>
#include <cmath>

//==============================================================================
static void test_register_and_get_sample()
{
    SampleLib lib;
    lib.registerSample("surdo_1", "open", juce::File());
    lib.registerSample("surdo_1", "muted", juce::File());

    // Load generates fallback tones for empty files
    lib.loadAll();

    assert(lib.isReady());

    const auto* openSample = lib.getSample("surdo_1", "open");
    assert(openSample != nullptr);
    assert(openSample->getNumSamples() > 0);

    const auto* mutedSample = lib.getSample("surdo_1", "muted");
    assert(mutedSample != nullptr);

    std::printf("  PASS: test_register_and_get_sample\n");
}

static void test_get_sample_defaults_to_default_slot()
{
    SampleLib lib;
    lib.registerSample("caixa", "default", juce::File());
    lib.loadAll();

    // getSample(name) uses "default" slot
    const auto* sample = lib.getSample("caixa");
    assert(sample != nullptr);
    assert(sample->getNumSamples() > 0);

    std::printf("  PASS: test_get_sample_defaults_to_default_slot\n");
}

static void test_get_sample_null_for_unknown_track()
{
    SampleLib lib;
    lib.loadAll();

    const auto* sample = lib.getSample("nonexistent");
    assert(sample == nullptr);

    std::printf("  PASS: test_get_sample_null_for_unknown_track\n");
}

static void test_get_sample_null_for_unknown_slot()
{
    SampleLib lib;
    lib.registerSample("surdo_1", "default", juce::File());
    lib.loadAll();

    const auto* sample = lib.getSample("surdo_1", "rim");
    assert(sample == nullptr);

    std::printf("  PASS: test_get_sample_null_for_unknown_slot\n");
}

static void test_get_sample_null_before_load()
{
    SampleLib lib;
    lib.registerSample("surdo_1", "default", juce::File());

    // Not loaded yet
    assert(!lib.isReady());
    const auto* sample = lib.getSample("surdo_1");
    assert(sample == nullptr);

    std::printf("  PASS: test_get_sample_null_before_load\n");
}

static void test_multiple_tracks()
{
    SampleLib lib;
    for (int t = 0; t < 14; ++t)
    {
        lib.registerSample(juce::String(t), "default", juce::File());
    }
    lib.loadAll();

    for (int t = 0; t < 14; ++t)
    {
        const auto* sample = lib.getSample(juce::String(t));
        assert(sample != nullptr);
        assert(sample->getNumSamples() > 0);
    }

    std::printf("  PASS: test_multiple_tracks\n");
}

//==============================================================================
static void test_generate_tone_produces_valid_buffer()
{
    // Generate a longer buffer so fade-in doesn't dominate
    auto buffer = SampleLib::generateTone(44100.0, 440.0, 44100, 0.5f);

    assert(buffer.getNumChannels() == 1);
    assert(buffer.getNumSamples() == 44100);

    // Check that samples are non-zero (sine wave)
    float firstSample = buffer.getSample(0, 0);
    assert(std::fabs(firstSample) < 0.01f);  // fade-in starts near 0

    // Check a sample in the middle (after fade-in) — sin should be near full amplitude
    int midSample = 44100 / 2;  // 0.5s into the buffer
    float midVal = buffer.getSample(0, midSample);
    // At 0.5s with 440Hz: sin(2*pi*440*0.5) = sin(440*pi) = sin(multiples of pi) ≈ 0
    // That's a zero-crossing. Pick a sample that's at peak.
    // Better: use quarter-period from start of the sine (offset by fadeLen)
    int fadeLen = 44100 / 10;  // 4410 samples
    int quarterPeriod = 44100 / 440 / 4;  // ~25 samples
    // Pick a sample after fade-in + quarter period offset
    int peakIdx = fadeLen + quarterPeriod;
    float peakSample = buffer.getSample(0, peakIdx);
    assert(std::fabs(peakSample) > 0.3f);  // should be near max amplitude after fade-in

    std::printf("  PASS: test_generate_tone_produces_valid_buffer\n");
}

static void test_generate_tone_has_fade_edges()
{
    int numSamples = 4410;  // 0.1s at 44100 Hz
    auto buffer = SampleLib::generateTone(44100.0, 100.0, numSamples, 1.0f);

    // First few samples should have very low amplitude (fade-in)
    float firstSample = buffer.getSample(0, 0);
    assert(std::fabs(firstSample) < 0.01f);

    // At quarter-period from the fade-in end, we should hit a peak
    // fadeLen = numSamples/10 = 441
    // 100 Hz → period = 441 samples → quarter period ≈ 110 samples
    // Peak at fadeLen + quarterPeriod = 441 + 110 = 551
    int peakIdx = 441 + 110;
    float peakSample = buffer.getSample(0, peakIdx);
    assert(std::fabs(peakSample) > 0.7f);  // close to full amplitude

    // Last few samples should have very low amplitude (fade-out)
    float lastSample = buffer.getSample(0, numSamples - 1);
    assert(std::fabs(lastSample) < 0.01f);

    std::printf("  PASS: test_generate_tone_has_fade_edges\n");
}

//==============================================================================
int main()
{
    std::printf("\n=== SampleLib Unit Tests ===\n\n");

    test_register_and_get_sample();
    test_get_sample_defaults_to_default_slot();
    test_get_sample_null_for_unknown_track();
    test_get_sample_null_for_unknown_slot();
    test_get_sample_null_before_load();
    test_multiple_tracks();

    std::printf("\n--- generateTone() ---\n");
    test_generate_tone_produces_valid_buffer();
    test_generate_tone_has_fade_edges();

    std::printf("\n=== All SampleLib tests PASSED ===\n\n");
    return 0;
}
