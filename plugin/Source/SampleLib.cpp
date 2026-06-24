/*
 * Copyright (c) 2025 Fernando Passos. All rights reserved.
 * Licensed under the MIT License.
 *
 * Lolooper — Live performance looper for Brazilian music
 */

#include "SampleLib.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <cmath>

//==============================================================================
void SampleLib::registerSample(const juce::String& trackName,
                                const juce::String& slotName,
                                const juce::File& file)
{
    auto it = m_trackMap.find(trackName);

    TrackSamples* track;
    if (it == m_trackMap.end())
    {
        auto newTrack = new TrackSamples();
        newTrack->name = trackName;
        m_tracks.add(newTrack);
        m_trackMap[trackName] = newTrack;
        track = newTrack;
    }
    else
    {
        track = it->second;
    }

    SampleSlot slot;
    slot.file = file;
    slot.loaded = false;
    track->slots[slotName] = std::move(slot);
}

//==============================================================================
void SampleLib::loadAll()
{
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    m_ready = true;

    for (auto* track : m_tracks)
    {
        for (auto& slotPair : track->slots)
        {
            auto& slot = slotPair.second;
            auto file = slot.file;

            // If no file was provided (empty path from constructor), generate tone
            if (file.getFullPathName().isEmpty() || !file.existsAsFile())
            {
                // Generate a fallback tone instead
                slot.buffer = std::make_unique<juce::AudioBuffer<float>>(
                    generateTone(m_sampleRate, 220.0 + (double)(std::rand() % 100), (int)(m_sampleRate * 0.1), 0.5f));
                slot.loaded = true;
                continue;
            }

            auto* reader = formatManager.createReaderFor(file);

            if (reader == nullptr)
            {
                // Fallback tone
                slot.buffer = std::make_unique<juce::AudioBuffer<float>>(
                    generateTone(m_sampleRate, 220.0, (int)(m_sampleRate * 0.1), 0.5f));
                slot.loaded = true;
                continue;
            }

            // Read the entire file
            auto numChannels = reader->numChannels;
            auto numSamples = (int)reader->lengthInSamples;

            auto buffer = std::make_unique<juce::AudioBuffer<float>>(numChannels, numSamples);
            reader->read(buffer.get(), 0, numSamples, 0, true, true);

            slot.buffer = std::move(buffer);
            slot.loaded = true;

            // Use the sample rate of the first loaded sample
            if (m_sampleRate == 44100.0)
                m_sampleRate = reader->sampleRate;

            delete reader;
        }
    }
}

//==============================================================================
const juce::AudioBuffer<float>* SampleLib::getSample(const juce::String& trackName) const
{
    return getSample(trackName, "default");
}

//==============================================================================
const juce::AudioBuffer<float>* SampleLib::getSample(const juce::String& trackName,
                                                      const juce::String& slotName) const
{
    auto it = m_trackMap.find(trackName);
    if (it == m_trackMap.end())
        return nullptr;

    auto* track = it->second;
    auto slotIt = track->slots.find(slotName);

    if (slotIt == track->slots.end() || !slotIt->second.loaded)
        return nullptr;

    return slotIt->second.buffer.get();
}

//==============================================================================
juce::AudioBuffer<float> SampleLib::generateTone(double sampleRate, double frequency,
                                                   int numSamples, float amplitude)
{
    juce::AudioBuffer<float> buffer(1, numSamples);

    for (int i = 0; i < numSamples; ++i)
    {
        double phase = (double)i * frequency / sampleRate;
        float sample = (float)(std::sin(2.0 * juce::MathConstants<double>::pi * phase) * amplitude);

        // Apply a short fade-in/fade-out to avoid clicks
        float envelope = 1.0f;
        int fadeLen = numSamples / 10;
        if (i < fadeLen)
            envelope = (float)i / (float)fadeLen;
        else if (i > numSamples - fadeLen)
            envelope = (float)(numSamples - i) / (float)fadeLen;

        buffer.setSample(0, i, sample * envelope);
    }

    return buffer;
}
