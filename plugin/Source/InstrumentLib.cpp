/*
 * Copyright (c) 2025 Fernando Passos. All rights reserved.
 * Licensed under the MIT License.
 *
 * Lolooper -- Live performance looper for Brazilian music
 */

#include "InstrumentLib.h"
#include <cstring>

//==============================================================================
InstrumentLib::InstrumentLib()
{
    m_settings = new_fluid_settings();
    if (!m_settings) return;

    // Disable console output from fluidsynth
    fluid_settings_setint(m_settings, "synth.verbose", 0);
    fluid_settings_setint(m_settings, "synth.midi-channels", 16);
    fluid_settings_setint(m_settings, "synth.polyphony", 32);

    m_synth = new_fluid_synth(m_settings);
    if (!m_synth)
    {
        delete_fluid_settings(m_settings);
        m_settings = nullptr;
    }
}

InstrumentLib::~InstrumentLib()
{
    if (m_synth)
    {
        delete_fluid_synth(m_synth);
        m_synth = nullptr;
    }
    if (m_settings)
    {
        delete_fluid_settings(m_settings);
        m_settings = nullptr;
    }
}

//==============================================================================
bool InstrumentLib::init(double sampleRate)
{
    m_sampleRate = sampleRate;
    if (!m_synth || !m_settings) return false;

    fluid_settings_setnum(m_settings, "synth.sample-rate", sampleRate);
    return true;
}

//==============================================================================
bool InstrumentLib::loadSoundFont(const juce::File& file)
{
    if (!m_synth || !file.existsAsFile()) return false;

    auto path = file.getFullPathName().toStdString();
    m_soundFontId = fluid_synth_sfload(m_synth, path.c_str(), 1);

    if (m_soundFontId == FLUID_FAILED)
    {
        m_ready = false;
        return false;
    }

    // Select a default preset (piano/acoustic grand)
    fluid_synth_program_select(m_synth, 0, m_soundFontId, 0, 0);

    m_ready = true;
    return true;
}

//==============================================================================
void InstrumentLib::noteOn(int channel, int note, int velocity)
{
    if (m_ready)
        fluid_synth_noteon(m_synth, channel, note, velocity);
}

void InstrumentLib::noteOff(int channel, int note)
{
    if (m_ready)
        fluid_synth_noteoff(m_synth, channel, note);
}

void InstrumentLib::allNotesOff()
{
    if (m_ready)
        fluid_synth_system_reset(m_synth);
}

//==============================================================================
void InstrumentLib::render(juce::AudioBuffer<float>& buffer, int numSamples)
{
    if (!m_ready)
    {
        buffer.clear();
        return;
    }

    jassert(buffer.getNumChannels() >= 2);

    auto* left = buffer.getWritePointer(0);
    auto* right = buffer.getWritePointer(1);

    // FluidSynth fills stereo float arrays
    fluid_synth_write_float(m_synth, numSamples, left, 0, 1, right, 0, 1);
}

//==============================================================================
void InstrumentLib::setGain(float gain)
{
    if (m_ready)
        fluid_synth_set_gain(m_synth, gain);
}
