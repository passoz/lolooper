/*
 * Copyright (c) 2025 Fernando Passos. All rights reserved.
 * Licensed under the MIT License.
 *
 * Lolooper -- Unit tests for ChordProgression
 */

#include "../Source/Arpeggiator.h"
#include <cassert>
#include <cstdio>
#include <cmath>

static void test_parse_chord_major()
{
    auto chord = ChordProgression::parseChord("C");
    assert(chord.root == 48); // C3
    assert(chord.intervals[1] == 0);
    assert(chord.intervals[3] == 4); // E = 4 semitones from C
    assert(chord.intervals[5] == 7); // G = 7 semitones from C
    std::printf("  PASS: test_parse_chord_major\n");
}

static void test_parse_chord_minor()
{
    auto chord = ChordProgression::parseChord("Am");
    assert(chord.root == 57); // A3
    assert(chord.intervals[3] == 3); // C = 3 semitones (minor 3rd)
    assert(chord.intervals[5] == 7); // E = 7 semitones (perfect 5th)
    std::printf("  PASS: test_parse_chord_minor\n");
}

static void test_parse_chord_dominant7()
{
    auto chord = ChordProgression::parseChord("G7");
    assert(chord.root == 55); // G3
    assert(chord.intervals[7] == 10); // F = 10 semitones (dominant 7th)
    std::printf("  PASS: test_parse_chord_dominant7\n");
}

static void test_parse_chord_minor7()
{
    auto chord = ChordProgression::parseChord("Dm7");
    assert(chord.root == 50); // D3
    assert(chord.intervals[3] == 3); // minor 3rd
    assert(chord.intervals[7] == 10); // minor 7th
    std::printf("  PASS: test_parse_chord_minor7\n");
}

static void test_get_note_for_degree()
{
    ChordProgression prog;
    prog.setChord("C");

    // C major: C D E F G A B
    int note1 = prog.getNoteForDegree(1); // root = C
    assert(note1 == 48); // C3

    int note3 = prog.getNoteForDegree(3); // E
    assert(note3 == 52); // E3

    int note5 = prog.getNoteForDegree(5); // G
    assert(note5 == 55); // G3

    std::printf("  PASS: test_get_note_for_degree\n");
}

static void test_get_note_for_degree_minor()
{
    ChordProgression prog;
    prog.setChord("Dm");

    // D minor: D E F G A Bb C
    int note1 = prog.getNoteForDegree(1);
    assert(note1 == 50); // D3

    int note3 = prog.getNoteForDegree(3);
    assert(note3 == 53); // F3 (minor 3rd = 3 semitones above D)

    std::printf("  PASS: test_get_note_for_degree_minor\n");
}

static void test_degree_zero_returns_rest()
{
    ChordProgression prog;
    prog.setChord("C");
    int note = prog.getNoteForDegree(0);
    assert(note == -1); // rest
    std::printf("  PASS: test_degree_zero_returns_rest\n");
}

static void test_octave_offset()
{
    ChordProgression prog;
    prog.setChord("C");
    int note = prog.getNoteForDegreeOctave(1, 0); // C3
    assert(note == 48);
    int noteUp = prog.getNoteForDegreeOctave(1, 1); // C4
    assert(noteUp == 60);
    int noteDown = prog.getNoteForDegreeOctave(1, -1); // C2
    assert(noteDown == 36);
    std::printf("  PASS: test_octave_offset\n");
}

static void test_chord_progression()
{
    ChordProgression prog;

    // Verse: C | Am | F | G
    prog.setChord("C");
    assert(prog.getCurrentChordName() == "C");
    assert(prog.getNoteForDegree(1) == 48);

    prog.setChord("Am");
    assert(prog.getCurrentChordName() == "Am");
    assert(prog.getNoteForDegree(1) == 57);

    prog.setChord("F");
    assert(prog.getNoteForDegree(5) == 60); // C4 = F3(53) + 7

    prog.setChord("G7");
    assert(prog.getNoteForDegree(5) == 62); // D4 = G3(55) + 7

    std::printf("  PASS: test_chord_progression\n");
}

static void test_sharp_chords()
{
    ChordProgression prog;
    prog.setChord("F#m");
    assert(prog.getCurrentChordName() == "F#m");
    assert(prog.getNoteForDegree(1) == 54); // F#3
    assert(prog.getNoteForDegree(3) == 57); // A3

    prog.setChord("C#7");
    assert(prog.getCurrentChordName() == "C#7");
    assert(prog.getNoteForDegree(1) == 49); // C#3
    std::printf("  PASS: test_sharp_chords\n");
}

//==============================================================================
int main()
{
    std::printf("\n=== Arpeggiator (ChordProgression) Unit Tests ===\n\n");

    std::printf("--- Chord Parsing ---\n");
    test_parse_chord_major();
    test_parse_chord_minor();
    test_parse_chord_dominant7();
    test_parse_chord_minor7();
    test_sharp_chords();

    std::printf("\n--- Degree to Note ---\n");
    test_get_note_for_degree();
    test_get_note_for_degree_minor();
    test_degree_zero_returns_rest();
    test_octave_offset();

    std::printf("\n--- Progression ---\n");
    test_chord_progression();

    std::printf("\n=== All Arpeggiator tests PASSED ===\n\n");
    return 0;
}
