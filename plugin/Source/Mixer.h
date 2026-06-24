/*
 * Copyright (c) 2025 Fernando Passos. All rights reserved.
 * Licensed under the MIT License.
 *
 * Lolooper — Live performance looper for Brazilian music
 */

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>

//==============================================================================
/** The Mixer renders audio for one step: it reads pattern velocities,
 *  applies accent, volume, pan, and sums samples from the SampleLib
 *  into the output buffer.
 */
class Mixer
{
public:
    Mixer() = default;
    ~Mixer() = default;

    //==============================================================================
    /** Render all active tracks for one step into the output buffer.
     *
     *  @param step       Current step (0-15)
     *  @param pattern    14×16 grid of velocities (0.0 to 1.0)
     *  @param volumes    14 track volumes (0.0 to 1.0)
     *  @param pans       14 track pans (0.0=left, 0.5=center, 1.0=right)
     *  @param mutes      14 track mute states
     *  @param solos      14 track solo states
     *  @param accents    14 track accent multipliers (0.0 to 2.0)
     *  @param output     Stereo output buffer (filled by this method)
     *  @param sampleRate Current sample rate (for pitch/resampling if needed)
     *
     *  @note If any solo is active, only soloed tracks render.
     *  @note The output buffer must be stereo (2 channels).
     */
    void renderStep(int step,
                    const float pattern[14][16],
                    const float volumes[14],
                    const float pans[14],
                    const bool mutes[14],
                    const bool solos[14],
                    const float accents[14],
                    juce::AudioBuffer<float>& output,
                    double sampleRate);

    //==============================================================================
    /** Apply soft clipping to the output buffer to prevent harsh distortion. */
    void applySoftClip(juce::AudioBuffer<float>& buffer);

    //==============================================================================
    /** Set the sample library reference (must be set before rendering). */
    void setSampleLib(class SampleLib* lib) noexcept { m_sampleLib = lib; }

    /** Get the current sample library pointer. */
    SampleLib* getSampleLib() const noexcept { return m_sampleLib; }

private:
    //==============================================================================
    SampleLib* m_sampleLib = nullptr;

    // Per-sample read pointers for each track (to handle sample-length mismatches)
    juce::int64 m_trackReadPos[14] = {};

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Mixer)
};
