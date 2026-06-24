/*
 * E2E Chord Detection test: feeds audio chords into the plugin
 * and verifies SysEx 0x09 output with correct chord names.
 */
#include "PluginProcessor.h"
#include <cassert>
#include <cstdio>
#include <cmath>

static juce::String findChordSysEx(const juce::MidiBuffer& midi) {
    for (const auto m : midi) {
        auto msg = m.getMessage();
        if (msg.isSysEx()) {
            auto d = msg.getSysExData();
            auto s = msg.getSysExDataSize();
            if (s >= 3 && d[0] == 0x7D && d[1] == 0x09) {
                return juce::String::fromUTF8((const char*)(d + 2), (int)(s - 2));
            }
        }
    }
    return {};
}

int main() {
    std::printf("\n=== Chord Detection E2E ===\n\n");
    int pass = 0, fail = 0;
    auto check = [&](bool cond, const char* name) {
        if (cond) { std::printf("  PASS: %s\n", name); pass++; }
        else { std::printf("  FAIL: %s\n", name); fail++; }
    };

    LolooperAudioProcessor proc;
    proc.prepareToPlay(44100.0, 1024);
    proc.setChordDetectEnabled(true);

    // C major: arpeggiated — play each note separately
    // Simulates a guitarist picking C, E, G in sequence
    {
        juce::MidiBuffer out;
        for (int i = 0; i < 60; ++i) {
            float freq;
            if (i % 3 == 0) freq = 261.63;       // C4
            else if (i % 3 == 1) freq = 329.63;    // E4
            else freq = 392.0;                      // G4
            
            juce::AudioBuffer<float> buf(2, 1024);
            for (int ch = 0; ch < 2; ++ch) {
                auto* d = buf.getWritePointer(ch);
                for (int s = 0; s < 1024; ++s)
                    d[s] = 0.4f * std::sin(2.0*3.14159*freq*s/44100.0);
            }
            proc.processBlock(buf, out);
        }
        auto chord = findChordSysEx(out);
        check(chord.isNotEmpty(), "C major: chord detected from audio");
    }

    // D minor: arpeggiated D, F, A
    {
        juce::MidiBuffer out;
        proc.m_chordRecognizer.clear();
        const double notes[] = {293.66, 349.23, 440.0};
        for (int i = 0; i < 60; ++i) {
            double freq = notes[i % 3];
            juce::AudioBuffer<float> buf(2, 1024);
            for (int ch = 0; ch < 2; ++ch) {
                auto* d = buf.getWritePointer(ch);
                for (int s = 0; s < 1024; ++s)
                    d[s] = 0.4f * std::sin(2.0*3.14159*freq*s/44100.0);
            }
            proc.processBlock(buf, out);
        }
        auto chord = findChordSysEx(out);
        check(chord == "Dm", "D minor: chord detected correctly");
    }

    // G7: arpeggiated G, B, D, F
    {
        juce::MidiBuffer out;
        proc.m_chordRecognizer.clear();
        const double notes[] = {392.0, 493.88, 587.33, 698.46}; // G4, B4, D5, F5
        for (int i = 0; i < 60; ++i) {
            double freq = notes[i % 4];
            juce::AudioBuffer<float> buf(2, 1024);
            for (int ch = 0; ch < 2; ++ch) {
                auto* d = buf.getWritePointer(ch);
                for (int s = 0; s < 1024; ++s)
                    d[s] = 0.4f * std::sin(2.0*3.14159*freq*s/44100.0);
            }
            proc.processBlock(buf, out);
        }
        auto chord = findChordSysEx(out);
        check(chord == "G7", "G7: chord detected correctly");
    }

    // Silence (no chord)
    {
        juce::AudioBuffer<float> silent(2, 1024);
        silent.clear();
        juce::MidiBuffer out;
        proc.m_chordRecognizer.clear();
        for (int i = 0; i < 15; ++i)
            proc.processBlock(silent, out);
        auto chord = findChordSysEx(out);
        check(chord.isEmpty(), "Silence: no chord detected");
    }

    // Disabled mode
    {
        proc.setChordDetectEnabled(false);
        juce::MidiBuffer out;
        for (int i = 0; i < 45; ++i) {
            juce::AudioBuffer<float> buf(2, 1024);
            buf.clear();
            for (int ch = 0; ch < 2; ++ch) {
                auto* d = buf.getWritePointer(ch);
                for (int s = 0; s < 1024; ++s)
                    d[s] = 0.3f * std::sin(2.0*3.14159*261.63*s/44100.0);
            }
            proc.processBlock(buf, out);
        }
        auto chord = findChordSysEx(out);
        check(chord.isEmpty(), "Disabled: no chord detected when disabled");
    }

    std::printf("\n  Results: %d passed, %d failed\n", pass, fail);
    return fail > 0 ? 1 : 0;
}
