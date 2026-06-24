/*
 * Copyright (c) 2025 Fernando Passos. All rights reserved.
 * Licensed under the MIT License.
 *
 * Lolooper -- Live performance looper for Brazilian music
 */

#pragma once

#include <juce_core/juce_core.h>
#include "Arpeggiator.h"

//==============================================================================
/** Accumulates detected MIDI notes and identifies the current chord.
 *
 *  Designed for acoustic guitar input: holds notes from the most recent
 *  audio frame, deduplicates, and matches against known chord types.
 *
 *  Output feeds directly into ChordProgression::setChord().
 */
class ChordRecognizer
{
public:
    ChordRecognizer() = default;
    ~ChordRecognizer() = default;

    //==============================================================================
    void addNote(int midiNote);
    juce::String identify();
    void clear();
    int getNumNotes() const noexcept { return m_noteCount; }
    void setChordProgression(ChordProgression* prog) noexcept { m_progression = prog; }
    void setEnabled(bool enabled) noexcept { m_enabled = enabled; }
    bool isEnabled() const noexcept { return m_enabled; }

private:
    int m_pitchClassCount[12] = {};
    juce::Array<int> m_rawNotes;     // actual MIDI notes held
    int m_noteCount = 0;
    bool m_enabled = true;
    ChordProgression* m_progression = nullptr;

    juce::String matchChord(int root, const juce::Array<int>& notes) const;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChordRecognizer)
};
