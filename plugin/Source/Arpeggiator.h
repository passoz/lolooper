/*
 * Copyright (c) 2025 Fernando Passos. All rights reserved.
 * Licensed under the MIT License.
 *
 * Lolooper -- Live performance looper for Brazilian music
 */

#pragma once

#include <juce_core/juce_core.h>

//==============================================================================
/** Chord type definitions for Brazilian music.
 *
 *  The arpeggiator uses degrees (1-7, 0=rest) in the pattern grid.
 *  For each step, it looks up the current chord and converts degrees
 *  to actual MIDI note numbers.
 */
struct Chord
{
    juce::String name;       // "C", "Dm", "G7", etc.
    int root = 60;           // MIDI note for root (C4 = 60)
    int intervals[8] = {};   // degree-indexed (1-7): semitones from root
                             // e.g. major: [0,2,4,5,7,9,11] at indices 1-7
};

//==============================================================================
/** Chord-to-note mapper for the degree-based arpeggiator.
 *
 *  Given a chord (e.g., Dm) and a degree (1-7), returns the
 *  corresponding MIDI note number.
 *
 *  Also manages chord progressions for song sections.
 */
class ChordProgression
{
public:
    ChordProgression() = default;
    ~ChordProgression() = default;

    //==============================================================================
    /** Set the current chord by name (e.g., "C", "Dm", "G7", "Am"). */
    void setChord(const juce::String& chordName);

    /** Set the current chord by root MIDI note and type. */
    void setChord(int rootNote, const juce::String& type);

    /** Get the MIDI note for a given degree (1-7) in the current chord.
     *  Returns -1 for degree 0 (rest) or invalid.
     */
    int getNoteForDegree(int degree) const;

    /** Get degree to note for an octave offset (higher/lower octaves). */
    int getNoteForDegreeOctave(int degree, int octaveOffset) const;

    //==============================================================================
    /** Get the current chord name. */
    juce::String getCurrentChordName() const noexcept { return m_currentChord.name; }

    /** Get the current chord root note. */
    int getCurrentRoot() const noexcept { return m_currentChord.root; }

    //==============================================================================
    /** Parse a chord name into root+type and populate the Chord struct. */
    static Chord parseChord(const juce::String& chordName);

    /** Get all available chord names. */
    static const juce::StringArray& getChordNames() noexcept { return s_chordNames; }

private:
    //==============================================================================
    Chord m_currentChord;
    static const juce::StringArray s_chordNames;

    // Chord type intervals (semitones from root for degrees 1-7)
    static const int s_majorIntervals[8];
    static const int s_minorIntervals[8];
    static const int s_dominant7Intervals[8];
    static const int s_minor7Intervals[8];
};
