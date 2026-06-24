/*
 * Copyright (c) 2025 Fernando Passos. All rights reserved.
 * Licensed under the MIT License.
 *
 * Lolooper — Live performance looper for Brazilian music
 */

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_core/juce_core.h>
#include <unordered_map>

//==============================================================================
/** Manages percussive sample loading, caching, and retrieval.
 *
 *  Samples are loaded from disk (WAV/FLAC) and cached in memory as
 *  AudioBuffer<float>. Supports multi-sample tracks (e.g., surdo
 *  can have open, muted, and rim variants).
 */
class SampleLib
{
public:
    SampleLib() = default;
    ~SampleLib() = default;

    //==============================================================================
    /** Register a sample slot. Each track can have one or more samples.
     *  Example: "surdo_1" → { "open": "samples/surdo_aberto.wav", ... }
     */
    void registerSample(const juce::String& trackName,
                        const juce::String& slotName,
                        const juce::File& file);

    /** Load all registered samples into memory. */
    void loadAll();

    /** Check if all samples loaded successfully. */
    bool isReady() const noexcept { return m_ready; }

    //==============================================================================
    /** Get a sample buffer for the given track's primary (default) slot.
     *  Returns nullptr if not loaded.
     */
    const juce::AudioBuffer<float>* getSample(const juce::String& trackName) const;

    /** Get a sample buffer for a specific slot of a track.
     *  Returns nullptr if not loaded.
     */
    const juce::AudioBuffer<float>* getSample(const juce::String& trackName,
                                               const juce::String& slotName) const;

    //==============================================================================
    /** Get the sample rate of the loaded samples (assumes all match). */
    double getSampleRate() const noexcept { return m_sampleRate; }

    //==============================================================================
    /** Set the base directory for sample file lookups. */
    void setSampleDirectory(const juce::File& directory) { m_sampleDir = directory; }

    /** Generate a simple sine-wave tone as a fallback sample for testing. */
    static juce::AudioBuffer<float> generateTone(double sampleRate, double frequency,
                                                  int numSamples, float amplitude);

private:
    //==============================================================================
    struct SampleSlot
    {
        juce::File file;
        std::unique_ptr<juce::AudioBuffer<float>> buffer;
        bool loaded = false;
    };

    struct TrackSamples
    {
        juce::String name;
        std::unordered_map<juce::String, SampleSlot, std::hash<juce::String>> slots;
    };

    juce::OwnedArray<TrackSamples> m_tracks;
    std::unordered_map<juce::String, TrackSamples*, std::hash<juce::String>> m_trackMap;

    juce::File m_sampleDir;
    double m_sampleRate = 44100.0;
    bool m_ready = false;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SampleLib)
};
