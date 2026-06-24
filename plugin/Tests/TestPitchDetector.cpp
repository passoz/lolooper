/*
 * Copyright (c) 2025 Fernando Passos. All rights reserved.
 * Licensed under the MIT License.
 *
 * Lolooper -- Unit tests for PitchDetector
 */

#include "../Source/PitchDetector.h"
#include <cassert>
#include <cstdio>
#include <cmath>

// Generate a sine wave at a given frequency
static juce::AudioBuffer<float> generateSine(double freq, double sampleRate, int numSamples, float amp = 0.8f)
{
    juce::AudioBuffer<float> buffer(1, numSamples);
    auto* data = buffer.getWritePointer(0);
    for (int i = 0; i < numSamples; ++i)
    {
        double phase = (double)i * freq / sampleRate;
        data[i] = amp * (float)std::sin(2.0 * juce::MathConstants<double>::pi * phase);
    }
    return buffer;
}

// Generate a chord (sum of sine waves)
static juce::AudioBuffer<float> generateChord(const std::vector<double>& freqs,
                                               double sampleRate, int numSamples, float amp = 0.5f)
{
    juce::AudioBuffer<float> buffer(1, numSamples);
    buffer.clear();
    auto* data = buffer.getWritePointer(0);
    for (double freq : freqs)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            double phase = (double)i * freq / sampleRate;
            data[i] += amp * (float)std::sin(2.0 * juce::MathConstants<double>::pi * phase);
        }
    }
    return buffer;
}

//==============================================================================
static void test_frequency_to_midi()
{
    assert(PitchDetector::frequencyToMidiNote(440.0f) == 69);   // A4
    assert(PitchDetector::frequencyToMidiNote(261.63f) == 60);  // C4
    assert(PitchDetector::frequencyToMidiNote(329.63f) == 64);  // E4
    assert(PitchDetector::frequencyToMidiNote(392.0f) == 67);   // G4
    assert(PitchDetector::frequencyToMidiNote(10.0f) == -1);    // out of range
    assert(PitchDetector::frequencyToMidiNote(9000.0f) == -1);  // out of range
    std::printf("  PASS: test_frequency_to_midi\n");
}

static void test_detect_sine_A4()
{
    PitchDetector pd;
    pd.prepare(44100.0, 2048);

    auto buffer = generateSine(440.0, 44100.0, 2048);
    float freq = pd.detect(buffer);

    // Should be close to 440 Hz
    assert(freq > 400.0f && freq < 480.0f);
    assert(PitchDetector::frequencyToMidiNote(freq) == 69
           || PitchDetector::frequencyToMidiNote(freq) == 68
           || PitchDetector::frequencyToMidiNote(freq) == 70);
    std::printf("  PASS: test_detect_sine_A4 (freq=%.1f Hz)\n", freq);
}

static void test_detect_sine_C4()
{
    PitchDetector pd;
    pd.prepare(44100.0, 2048);

    auto buffer = generateSine(261.63, 44100.0, 2048);
    float freq = pd.detect(buffer);
    int note = PitchDetector::frequencyToMidiNote(freq);
    assert(note >= 58 && note <= 62); // C4 ± buffer tolerance
    std::printf("  PASS: test_detect_sine_C4 (freq=%.1f Hz, note=%d)\n", freq, note);
}

static void test_detect_sine_G3()
{
    PitchDetector pd;
    pd.prepare(44100.0, 2048);

    auto buffer = generateSine(196.0, 44100.0, 2048);
    float freq = pd.detect(buffer);
    int note = PitchDetector::frequencyToMidiNote(freq);
    assert(note >= 53 && note <= 57);
    std::printf("  PASS: test_detect_sine_G3 (freq=%.1f Hz, note=%d)\n", freq, note);
}

static void test_detect_silence()
{
    PitchDetector pd;
    pd.prepare(44100.0, 512);

    juce::AudioBuffer<float> buffer(1, 512);
    buffer.clear();
    float freq = pd.detect(buffer);
    // Silence should give 0 or very low freq
    assert(freq < 20.0f || PitchDetector::frequencyToMidiNote(freq) == -1);
    std::printf("  PASS: test_detect_silence (freq=%.1f Hz)\n", freq);
}

static void test_detect_chord_root()
{
    // For a C major chord, aubio detects the dominant pitch
    PitchDetector pd;
    pd.prepare(44100.0, 4096);

    auto chord = generateChord({261.63, 329.63, 392.0}, 44100.0, 4096, 0.5f);
    float freq = pd.detect(chord);

    // Monophonic detector picks ONE dominant frequency from the chord
    // It should be audible (not 0, not noise)
    assert(freq > 0.0f);
    int note = PitchDetector::frequencyToMidiNote(freq);
    assert(note > 0);
    std::printf("  PASS: test_detect_chord_root (freq=%.1f Hz, note=%d)\n", freq, note);
}

static void test_prepare_does_not_crash()
{
    PitchDetector pd;
    pd.prepare(48000.0, 1024);
    pd.prepare(44100.0, 512); // reprepare should work
    std::printf("  PASS: test_prepare_does_not_crash\n");
}

//==============================================================================
int main()
{
    std::printf("\n=== PitchDetector Unit Tests ===\n\n");

    test_frequency_to_midi();
    test_detect_sine_A4();
    test_detect_sine_C4();
    test_detect_sine_G3();
    test_detect_silence();
    test_detect_chord_root();
    test_prepare_does_not_crash();

    std::printf("\n=== All PitchDetector tests PASSED ===\n\n");
    return 0;
}
