/*
 * Copyright (c) 2025 Fernando Passos. All rights reserved.
 * Licensed under the MIT License.
 *
 * Lolooper -- Live performance looper for Brazilian music
 */

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include "fluidsynth.h"

//==============================================================================
/** Wraps FluidSynth for SF2/SFZ instrument playback.
 *
 *  Handles loading SoundFont files, MIDI note-on/off, and audio rendering.
 *  One instance per melodic track (cavaquinho, violao 7, banjo).
 */
class InstrumentLib
{
public:
    InstrumentLib();
    ~InstrumentLib();

    //==============================================================================
    /** Initialize FluidSynth with the given sample rate. */
    bool init(double sampleRate);

    /** Load a SoundFont (.sf2) or SFZ file. Returns true on success. */
    bool loadSoundFont(const juce::File& file);

    /** Check if a SoundFont is loaded and ready. */
    bool isReady() const noexcept { return m_ready; }

    //==============================================================================
    /** Play a MIDI note. */
    void noteOn(int channel, int note, int velocity);

    /** Stop a MIDI note. */
    void noteOff(int channel, int note);

    /** Stop all notes immediately (panic). */
    void allNotesOff();

    //==============================================================================
    /** Render audio into a stereo buffer.
     *  @param buffer  Stereo output buffer (must be 2 channels)
     *  @param numSamples  Number of samples to render
     */
    void render(juce::AudioBuffer<float>& buffer, int numSamples);

    //==============================================================================
    /** Set the master gain (0.0 to 1.0). */
    void setGain(float gain);

    /** Get the current sample rate. */
    double getSampleRate() const noexcept { return m_sampleRate; }

private:
    //==============================================================================
    fluid_settings_t* m_settings = nullptr;
    fluid_synth_t* m_synth = nullptr;
    int m_soundFontId = -1;
    bool m_ready = false;
    double m_sampleRate = 44100.0;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InstrumentLib)
};
