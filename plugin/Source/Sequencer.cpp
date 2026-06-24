/*
 * Copyright (c) 2025 Fernando Passos. All rights reserved.
 * Licensed under the MIT License.
 *
 * Lolooper — Live performance looper for Brazilian music
 */

#include "Sequencer.h"
#include <cmath>
#include <cstdlib>

//==============================================================================
void Sequencer::prepare(double sampleRate, double bpm)
{
    m_sampleRate = sampleRate;
    m_bpm = bpm;
    m_samplesPerStep = (60.0 / m_bpm / 4.0) * m_sampleRate;
    m_stepPhase = 0;
    m_stepTarget = (juce::int64)m_samplesPerStep;
}

//==============================================================================
void Sequencer::setBpm(double bpm)
{
    if (bpm < 20.0) bpm = 20.0;
    if (bpm > 300.0) bpm = 300.0;
    m_bpm = bpm;
    m_samplesPerStep = (60.0 / m_bpm / 4.0) * m_sampleRate;
    m_stepTarget = (juce::int64)m_samplesPerStep;
}

//==============================================================================
int Sequencer::advance(int samplesInBlock)
{
    if (!m_isPlaying)
        return 0;

    int stepsElapsed = 0;

    for (int i = 0; i < samplesInBlock; ++i)
    {
        // Calculate step target for this particular step (accounts for swing)
        juce::int64 target = m_stepTarget;

        // Swing: delay off-beat 16th notes for the 8th-note feel
        if (m_swing > 0.0 && (m_currentStep % 2 == 1))
        {
            juce::int64 extra = (juce::int64)(m_swing * m_samplesPerStep);
            target += extra;
        }

        // Humanize: random timing jitter (max 50ms at humanize=1.0)
        if (m_humanize > 0.0 && (m_currentStep % 2 == 1))
        {
            double jitterMs = (double)(std::rand() % 1000) / 1000.0 * 50.0 * m_humanize;
            juce::int64 jitterSamples = (juce::int64)(jitterMs / 1000.0 * m_sampleRate);
            target += jitterSamples;
        }

        m_stepPhase++;

        if (m_stepPhase >= target)
        {
            m_stepPhase -= target;
            stepsElapsed++;

            // Advance step
            m_currentStep = (m_currentStep + 1) % 16;

            if (m_currentStep == 0)
            {
                // Advanced a beat
                m_currentBeat = (m_currentBeat + 1) % 4;

                if (m_currentBeat == 0)
                {
                    // Advanced a bar
                    m_currentBar++;
                }
            }
        }
    }

    return stepsElapsed;
}

//==============================================================================
void Sequencer::play()
{
    m_isPlaying = true;
}

void Sequencer::stop()
{
    m_isPlaying = false;
    m_currentStep = 0;
    m_currentBeat = 0;
    m_currentBar = 0;
    m_stepPhase = 0;
}

void Sequencer::pause()
{
    m_isPlaying = false;
}
