/*
 * Copyright (c) 2025 Fernando Passos. All rights reserved.
 * Licensed under the MIT License.
 *
 * Lolooper -- Live performance looper for Brazilian music
 */

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include "aubio.h"

//==============================================================================
/** Real-time monophonic pitch detector using aubio.
 *
 *  Detects the fundamental frequency from an audio buffer.
 *  For chord detection, this is run on a summed/mono mix of the input.
 */
class PitchDetector
{
public:
    PitchDetector();
    ~PitchDetector();

    //==============================================================================
    /** Prepare the detector for a given sample rate and buffer size. */
    void prepare(double sampleRate, int bufferSize);

    /** Detect pitch from an audio buffer. Returns frequency in Hz, or 0.0 if none. */
    float detect(const juce::AudioBuffer<float>& buffer);

    /** Get the MIDI note number from a frequency.
     *  Returns -1 if frequency is 0 or out of range (sub C0 or above C8).
     */
    static int frequencyToMidiNote(float freq);

    /** Get the last detected frequency. */
    float getLastFrequency() const noexcept { return m_lastFrequency; }

private:
    //==============================================================================
    aubio_pitch_t* m_pitch = nullptr;
    fvec_t* m_inVec = nullptr;
    fvec_t* m_outVec = nullptr;
    float m_lastFrequency = 0.0f;
    double m_sampleRate = 44100.0;
    int m_bufferSize = 512;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PitchDetector)
};
