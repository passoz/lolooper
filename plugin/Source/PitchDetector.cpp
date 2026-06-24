/*
 * Copyright (c) 2025 Fernando Passos. All rights reserved.
 * Licensed under the MIT License.
 *
 * Lolooper -- Live performance looper for Brazilian music
 */

#include "PitchDetector.h"
#include <cmath>

//==============================================================================
PitchDetector::PitchDetector()
{
}

PitchDetector::~PitchDetector()
{
    if (m_pitch) del_aubio_pitch(m_pitch);
    if (m_inVec) del_fvec(m_inVec);
    if (m_outVec) del_fvec(m_outVec);
}

//==============================================================================
void PitchDetector::prepare(double sampleRate, int bufferSize)
{
    m_sampleRate = sampleRate;
    m_bufferSize = bufferSize;

    // Clean up previous instances
    if (m_pitch) del_aubio_pitch(m_pitch);
    if (m_inVec) del_fvec(m_inVec);
    if (m_outVec) del_fvec(m_outVec);

    // Create aubio pitch detector (yinfft is fast and accurate for guitar)
    m_pitch = new_aubio_pitch("yinfft", (uint_t)bufferSize, (uint_t)(bufferSize / 2), (uint_t)sampleRate);
    aubio_pitch_set_tolerance(m_pitch, 0.7f);
    aubio_pitch_set_silence(m_pitch, -40.0f);

    m_inVec = new_fvec((uint_t)bufferSize);
    m_outVec = new_fvec(1);
}

//==============================================================================
float PitchDetector::detect(const juce::AudioBuffer<float>& buffer)
{
    if (!m_pitch || !m_inVec) return 0.0f;

    // Mix down to mono (channel 0)
    auto* readPtr = buffer.getReadPointer(0);
    for (int i = 0; i < juce::jmin(m_bufferSize, buffer.getNumSamples()); ++i)
    {
        float sample = readPtr[i];
        // Simple soft clip to prevent aubio overflow
        if (sample > 1.0f) sample = 1.0f;
        if (sample < -1.0f) sample = -1.0f;
        m_inVec->data[i] = sample;
    }

    aubio_pitch_do(m_pitch, m_inVec, m_outVec);

    float freq = m_outVec->data[0];
    m_lastFrequency = freq;
    return freq;
}

//==============================================================================
int PitchDetector::frequencyToMidiNote(float freq)
{
    if (freq < 20.0f || freq > 8000.0f) return -1;
    // MIDI note 69 = A4 = 440 Hz
    float midi = 69.0f + 12.0f * std::log2(freq / 440.0f);
    int note = (int)std::round(midi);
    if (note < 0 || note > 127) return -1;
    return note;
}
