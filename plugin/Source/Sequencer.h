/*
 * Copyright (c) 2025 Fernando Passos. All rights reserved.
 * Licensed under the MIT License.
 *
 * Lolooper — Live performance looper for Brazilian music
 */

#pragma once

#include <juce_core/juce_core.h>

//==============================================================================
/** The sequencer advances step-by-step based on BPM and sample rate.
 *
 *  One step = one 16th note (semicolcheia).
 *  4 steps = 1 beat.
 *  16 steps = 1 bar (4/4 time).
 */
class Sequencer
{
public:
    Sequencer() = default;
    ~Sequencer() = default;

    //==============================================================================
    /** Configure the sequencer with the current sample rate and BPM.
     *  Must be called before advance(), and whenever sample rate or BPM changes.
     */
    void prepare(double sampleRate, double bpm);

    //==============================================================================
    /** Advance the sequencer by the given number of samples.
     *  Should be called from processBlock().
     *  Returns the number of steps that elapsed in this block.
     */
    int advance(int samplesInBlock);

    //==============================================================================
    /** Transport control */
    void play();
    void stop();
    void pause();

    bool isPlaying() const noexcept { return m_isPlaying; }

    //==============================================================================
    /** Current position getters */
    int getCurrentStep() const noexcept { return m_currentStep; }
    int getCurrentBeat() const noexcept { return m_currentBeat; }
    int getCurrentBar() const noexcept { return m_currentBar; }

    //==============================================================================
    /** Parameter setters */
    void setBpm(double bpm);
    void setSwing(double swing) noexcept { m_swing = swing; }
    void setHumanize(double humanize) noexcept { m_humanize = humanize; }

    double getBpm() const noexcept { return m_bpm; }
    double getSwing() const noexcept { return m_swing; }
    double getHumanize() const noexcept { return m_humanize; }

    //==============================================================================
    /** Calculate the number of samples per 16th note step at the current BPM */
    double getSamplesPerStep() const noexcept { return m_samplesPerStep; }

private:
    //==============================================================================
    double m_sampleRate = 44100.0;
    double m_bpm = 100.0;
    double m_swing = 0.0;
    double m_humanize = 0.0;
    double m_samplesPerStep = 0.0;

    bool m_isPlaying = false;

    int m_currentStep = 0;    // 0-15
    int m_currentBeat = 0;    // 0-3
    int m_currentBar = 0;     // unbounded

    // Integer-based phase accumulator: counts samples toward the next step
    // Avoids floating-point drift that occurs when repeatedly adding 1.0/sps
    juce::int64 m_stepPhase = 0;   // samples accumulated toward current step
    juce::int64 m_stepTarget = 0;  // target samples to reach before triggering step

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Sequencer)
};
