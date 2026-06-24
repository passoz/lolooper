/*
 * Copyright (c) 2025 Fernando Passos. All rights reserved.
 * Licensed under the MIT License.
 *
 * Lolooper -- Live performance looper for Brazilian music
 */

#include "ChordRecognizer.h"
#include <cstring>

//==============================================================================
void ChordRecognizer::addNote(int midiNote)
{
    if (midiNote < 0 || midiNote > 127) return;

    int pitchClass = midiNote % 12;
    if (m_pitchClassCount[pitchClass] == 0)
        m_noteCount++;
    m_pitchClassCount[pitchClass]++;

    m_rawNotes.add(midiNote);
}

//==============================================================================
void ChordRecognizer::clear()
{
    std::memset(m_pitchClassCount, 0, sizeof(m_pitchClassCount));
    m_rawNotes.clear();
    m_noteCount = 0;
}

//==============================================================================
juce::String ChordRecognizer::identify()
{
    if (m_noteCount < 3) return {};

    // Sort raw MIDI notes to find the actual bass note
    m_rawNotes.sort();
    int bassMidi = m_rawNotes[0];
    int root = bassMidi % 12;

    // Get unique pitch classes
    juce::Array<int> present;
    for (int i = 0; i < 12; ++i)
        if (m_pitchClassCount[i] > 0)
            present.add(i);

    auto chord = matchChord(root, present);
    if (chord.isEmpty()) return {};

    if (m_enabled && m_progression)
        m_progression->setChord(chord);

    return chord;
}

//==============================================================================
juce::String ChordRecognizer::matchChord(int root, const juce::Array<int>& notes) const
{
    juce::Array<int> intervals;
    for (auto note : notes)
        intervals.add((note - root + 12) % 12);

    bool hasMinor3rd = intervals.contains(3);
    bool hasMajor3rd = intervals.contains(4);
    bool has5th = intervals.contains(7);
    bool hasMinor7th = intervals.contains(10);
    bool hasMajor7th = intervals.contains(11);

    juce::String type;
    if (hasMinor3rd && hasMinor7th)       type = "m7";
    else if (hasMajor3rd && hasMinor7th)   type = "7";
    else if (hasMinor3rd)                  type = "m";
    else if (hasMajor3rd)                  type = "";

    if (type.isEmpty() && !has5th) return {};

    static const char* noteNames[] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
    return juce::String(noteNames[root]) + type;
}
