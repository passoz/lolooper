/*
 * Copyright (c) 2025 Fernando Passos. All rights reserved.
 * Licensed under the MIT License.
 *
 * Lolooper -- Live performance looper for Brazilian music
 */

#include "Arpeggiator.h"

//==============================================================================
// Interval definitions (semitones from root for degrees 1-7, index 0 unused)
//==============================================================================
const int ChordProgression::s_majorIntervals[8] =      { 0, 0, 2, 4, 5, 7, 9, 11 };
const int ChordProgression::s_minorIntervals[8] =      { 0, 0, 2, 3, 5, 7, 8, 10 };
const int ChordProgression::s_dominant7Intervals[8] =  { 0, 0, 2, 4, 5, 7, 9, 10 };
const int ChordProgression::s_minor7Intervals[8] =     { 0, 0, 2, 3, 5, 7, 8, 10 };

const juce::StringArray ChordProgression::s_chordNames = {
    "C",  "Cm",  "C7",  "Cm7",
    "C#", "C#m", "C#7", "C#m7",
    "D",  "Dm",  "D7",  "Dm7",
    "D#", "D#m", "D#7", "D#m7",
    "E",  "Em",  "E7",  "Em7",
    "F",  "Fm",  "F7",  "Fm7",
    "F#", "F#m", "F#7", "F#m7",
    "G",  "Gm",  "G7",  "Gm7",
    "G#", "G#m", "G#7", "G#m7",
    "A",  "Am",  "A7",  "Am7",
    "A#", "A#m", "A#7", "A#m7",
    "B",  "Bm",  "B7",  "Bm7",
};

// Root note MIDI numbers (C3 = 48 to B3 = 59)
static int getRootMidi(const juce::String& noteName)
{
    static const int baseNotes[] = { 0, 2, 4, 5, 7, 9, 11 }; // C D E F G A B
    char root = noteName[0];
    int base = 48; // C3
    switch (root)
    {
        case 'C': base = 48; break;
        case 'D': base = 50; break;
        case 'E': base = 52; break;
        case 'F': base = 53; break;
        case 'G': base = 55; break;
        case 'A': base = 57; break;
        case 'B': base = 59; break;
        default: return 60;
    }
    // Check for sharp (#)
    if (noteName.length() > 1 && noteName[1] == '#')
        base += 1;
    return base;
}

//==============================================================================
Chord ChordProgression::parseChord(const juce::String& chordName)
{
    Chord chord;
    chord.name = chordName;

    // Default: C major
    chord.root = 60;
    std::memcpy(chord.intervals, s_majorIntervals, sizeof(s_majorIntervals));

    if (chordName.isEmpty()) return chord;

    chord.root = getRootMidi(chordName);

    // Determine chord type from the name suffix
    if (chordName.contains("m7"))
        std::memcpy(chord.intervals, s_minor7Intervals, sizeof(s_minor7Intervals));
    else if (chordName.contains("m"))
        std::memcpy(chord.intervals, s_minorIntervals, sizeof(s_minorIntervals));
    else if (chordName.contains("7"))
        std::memcpy(chord.intervals, s_dominant7Intervals, sizeof(s_dominant7Intervals));
    else
        std::memcpy(chord.intervals, s_majorIntervals, sizeof(s_majorIntervals));

    return chord;
}

//==============================================================================
void ChordProgression::setChord(const juce::String& chordName)
{
    m_currentChord = parseChord(chordName);
}

void ChordProgression::setChord(int rootNote, const juce::String& type)
{
    juce::String name;
    static const char* noteNames[] = { "C","C#","D","D#","E","F","F#","G","G#","A","A#","B" };
    int idx = rootNote % 12;
    name = noteNames[idx] + type;
    setChord(name);
}

//==============================================================================
int ChordProgression::getNoteForDegree(int degree) const
{
    if (degree <= 0 || degree > 7) return -1;
    return m_currentChord.root + m_currentChord.intervals[degree];
}

int ChordProgression::getNoteForDegreeOctave(int degree, int octaveOffset) const
{
    int note = getNoteForDegree(degree);
    if (note < 0) return -1;
    return note + (octaveOffset * 12);
}
