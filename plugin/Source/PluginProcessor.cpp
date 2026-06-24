/*
 * Copyright (c) 2025 Fernando Passos. All rights reserved.
 * Licensed under the MIT License.
 *
 * Lolooper -- Live performance looper for Brazilian music
 */

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <juce_core/juce_core.h>
#include <cmath>

//==============================================================================
static const juce::String trackNamesData[14] = {
    "Surdo 1", "Surdo 2", "Surdo 3", "Caixa", "Repique",
    "Tamborim", "Pandeiro", "Cuica", "Agogo", "Reco-reco",
    "Tantan", "Cavaquinho", "Violao 7", "Banjo"
};

static const juce::StringArray styleNamesData = {
    "Samba", "Pagode", "Partido Alto", "Samba Reggae",
    "Ijexa", "Frevo", "Maracatu", "Intro", "Virada"
};

const juce::String LolooperAudioProcessor::trackNames[14] = {
    "Surdo 1", "Surdo 2", "Surdo 3", "Caixa", "Repique",
    "Tamborim", "Pandeiro", "Cuica", "Agogo", "Reco-reco",
    "Tantan", "Cavaquinho", "Violao 7", "Banjo"
};

const juce::StringArray LolooperAudioProcessor::styleNames = styleNamesData;

//==============================================================================
LolooperAudioProcessor::LolooperAudioProcessor()
    : AudioProcessor(BusesProperties()
          .withInput("Input", juce::AudioChannelSet::stereo(), true)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    for (int t = 0; t < 14; ++t)
        m_sampleLib.registerSample(juce::String(t), "default", juce::File());

    m_bpmParam = new juce::AudioParameterFloat("bpm", "BPM", juce::NormalisableRange<float>(20.0f, 300.0f, 0.1f), 100.0f);
    m_swingParam = new juce::AudioParameterFloat("swing", "Swing", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f);
    m_humanizeParam = new juce::AudioParameterFloat("humanize", "Humanize", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f);
    m_patternStyleParam = new juce::AudioParameterChoice("patternStyle", "Pattern Style", styleNames, 0);

    for (int t = 0; t < 14; ++t)
    {
        auto sfx = juce::String(t);
        m_trackVolume[t] = new juce::AudioParameterFloat("volume_" + sfx, "Vol " + trackNames[t],
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.8f);
        m_trackPan[t] = new juce::AudioParameterFloat("pan_" + sfx, "Pan " + trackNames[t],
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f);
        m_trackMute[t] = new juce::AudioParameterBool("mute_" + sfx, "Mute " + trackNames[t], false);
        m_trackSolo[t] = new juce::AudioParameterBool("solo_" + sfx, "Solo " + trackNames[t], false);
        m_trackAccent[t] = new juce::AudioParameterFloat("accent_" + sfx, "Accent " + trackNames[t],
            juce::NormalisableRange<float>(0.0f, 2.0f, 0.01f), 1.0f);
    }

    addParameter(m_bpmParam); addParameter(m_swingParam);
    addParameter(m_humanizeParam); addParameter(m_patternStyleParam);
    for (int t = 0; t < 14; ++t) {
        addParameter(m_trackVolume[t]); addParameter(m_trackPan[t]);
        addParameter(m_trackMute[t]); addParameter(m_trackSolo[t]);
        addParameter(m_trackAccent[t]);
    }

    loadDefaultPatterns();
    m_mixer.setSampleLib(&m_sampleLib);

    // Init SF2 instruments for melodic tracks
    for (int i = 0; i < 3; ++i) {
        m_instrumentLib[i].init(44100.0);
        auto sf2 = juce::File("/usr/share/sounds/sf2/TimGM6mb.sf2");
        if (sf2.existsAsFile()) m_instrumentLib[i].loadSoundFont(sf2);
    }

    // Wire chord detection: recognizer updates the progression
    m_chordRecognizer.setChordProgression(&m_chordProgression);
}

LolooperAudioProcessor::~LolooperAudioProcessor() {}

const juce::String LolooperAudioProcessor::getName() const { return JucePlugin_Name; }
bool LolooperAudioProcessor::acceptsMidi() const { return true; }
bool LolooperAudioProcessor::producesMidi() const { return true; }
bool LolooperAudioProcessor::isMidiEffect() const { return false; }
double LolooperAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int LolooperAudioProcessor::getNumPrograms() { return 1; }
int LolooperAudioProcessor::getCurrentProgram() { return 0; }
void LolooperAudioProcessor::setCurrentProgram(int) {}
const juce::String LolooperAudioProcessor::getProgramName(int) { return {}; }
void LolooperAudioProcessor::changeProgramName(int, const juce::String&) {}

void LolooperAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    m_sampleRate = sampleRate;
    m_samplesPerBlock = samplesPerBlock;
    m_sequencer.prepare(sampleRate, m_bpmParam->get());
    m_sampleLib.loadAll();

    // Init pitch detector for chord recognition
    m_pitchDetector.prepare(sampleRate, samplesPerBlock);
}

void LolooperAudioProcessor::releaseResources() {}

//==============================================================================
void LolooperAudioProcessor::syncPatternGridFromLibrary()
{
    if (m_patternLibrary.size() == 0 || m_patternLibrary.getActivePattern() == nullptr)
    {
        std::memset(m_patternGrid, 0, sizeof(m_patternGrid));
        return;
    }
    const auto* pattern = m_patternLibrary.getActivePattern();
    for (int t = 0; t < 14; ++t)
        for (int s = 0; s < 16; ++s)
            m_patternGrid[t][s] = pattern->getVelocity(t, s);
}

//==============================================================================
void LolooperAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    // === Chord detection from audio input ===
    if (m_chordDetectEnabled && buffer.getNumChannels() > 0)
    {
        float freq = m_pitchDetector.detect(buffer);
        int note = PitchDetector::frequencyToMidiNote(freq);
        if (note > 0)
        {
            m_chordRecognizer.addNote(note);
            m_chordDetectCounter++;
            // Accumulate notes over ~100ms before attempting chord detection
            // (monophonic detector needs multiple frames to capture all chord tones)
            if (m_chordDetectCounter >= kChordDetectInterval)
            {
                auto chord = m_chordRecognizer.identify();
                if (chord.isNotEmpty())
                {
                    sendChordDetected(chord, midiMessages);
                }
                // Clear periodically to avoid stale notes accumulating indefinitely
                m_chordRecognizer.clear();
                m_chordDetectCounter = 0;
            }
        }
        else
        {
            // Silence: clear accumulated notes
            m_chordRecognizer.clear();
            m_chordDetectCounter = 0;
        }
    }

    buffer.clear();

    m_sequencer.setBpm(m_bpmParam->get());
    m_sequencer.setSwing(m_swingParam->get());
    m_sequencer.setHumanize(m_humanizeParam->get());

    // Process incoming MIDI
    for (const auto metadata : midiMessages)
    {
        auto msg = metadata.getMessage();

        if (msg.isNoteOn())
        {
            auto note = msg.getNoteNumber();
            auto vel = msg.getVelocity();

            // REC mode: capture MIDI notes into the grid
            if (m_isRecording && note >= 36 && note <= 49)
            {
                int track = note - 36;
                int step = m_sequencer.getCurrentStep();
                float velocity = juce::jlimit(0.0f, 1.0f, vel / 127.0f);

                if (m_recOverdub)
                {
                    // Overdub: don't zero existing, just record new hits
                }
                else
                {
                    // Clear the track's current step first
                    for (int s = 0; s < 16; ++s)
                        m_patternGrid[track][s] = 0.0f;
                }
                m_patternGrid[track][step] = velocity;

                // Also store in rec buffer for SysEx response
                m_recBuffer.add({ track, step, velocity });

                continue;
            }

            // Track mute toggle (36-49)
            if (note >= 36 && note <= 49)
            {
                int t = note - 36;
                *m_trackMute[t] = !m_trackMute[t]->get();
            }
            else if (note == 114)
            {
                m_isPlaying = !m_isPlaying;
                m_isPlaying ? m_sequencer.play() : m_sequencer.pause();
            }
            else if (note == 115) { m_isPlaying = false; m_sequencer.stop(); m_songModeActive = false; }
            else if (note == 116) { m_isPlaying = false; m_sequencer.pause(); }
            else if (note == 117) {
                bool wasRecording = m_isRecording;
                m_isRecording = !m_isRecording;
                // When stopping REC, defer SysEx output (can't write midiMessages mid-iteration).
                // Don't clear buffer here — flush happens after MIDI loop.
                if (wasRecording && !m_isRecording && m_recBuffer.size() > 0)
                    m_pendingRecPatternSend = true;
            }
            else if (note == 118) { /* metronome TBD */ }
            else if (note == 119) { advanceToNextSection(); }
            else if (note == 120) { goToPreviousSection(); }
            else if (note == 121)
            {
                m_currentPatternStyle = 8;
                *m_patternStyleParam = 8;
                syncPatternGridFromLibrary();
            }
            else if (note == 122) {
                for (int t = 0; t < 14; ++t) *m_trackSolo[t] = false;
            }
        }
        else if (msg.isController())
        {
            auto cc = msg.getControllerNumber();
            auto val = msg.getControllerValue();

            if (cc == 15) *m_bpmParam = juce::jmap<float>(val, 0, 127, 40.0f, 200.0f);
            else if (cc == 16) *m_swingParam = val / 127.0f;
            else if (cc == 17) *m_humanizeParam = val / 127.0f;
            else if (cc == 18)
            {
                int idx = juce::jlimit(0, m_patternLibrary.size() - 1, val);
                *m_patternStyleParam = idx;
                m_currentPatternStyle = idx;
                m_patternLibrary.setActiveStyleIndex(idx);
                syncPatternGridFromLibrary();
            }
            else if (cc >= 20 && cc <= 33) *m_trackVolume[cc - 20] = val / 127.0f;
            else if (cc >= 34 && cc <= 47) *m_trackPan[cc - 34] = val / 127.0f;
            else if (cc >= 48 && cc <= 61) *m_trackAccent[cc - 48] = juce::jmap<float>(val, 0, 127, 0.0f, 2.0f);
        }
        else if (msg.isSysEx())
        {
            handleSysEx(msg, midiMessages);
        }
    }

    // Flush deferred SysEx outputs (must happen after MIDI read loop to avoid re-parse)
    if (m_pendingRecPatternSend)
    {
        sendRecordedPattern(midiMessages);
        m_recBuffer.clear();
        m_pendingRecPatternSend = false;
    }

    // Render audio if playing
    if (m_isPlaying)
    {
        m_sequencer.play();
        int steps = m_sequencer.advance(buffer.getNumSamples());

        int step = m_sequencer.getCurrentStep();

        // Song mode: count bars and advance sections
        if (m_songModeActive && steps > 0)
        {
            // Check if we just started a new bar (step wrapped from 15 to 0)
            if (step == 0)
            {
                m_barsInCurrentSection++;
                if (m_barsInCurrentSection >= getCurrentSectionBars())
                {
                    if (!isCurrentSectionLooping())
                        advanceToNextSection();
                    else
                        m_barsInCurrentSection = 0; // loop
                }
            }
        }

        float volumes[14], pans[14], accents[14];
        bool mutes[14], solos[14];
        for (int t = 0; t < 14; ++t)
        {
            volumes[t] = m_trackVolume[t]->get();
            pans[t] = m_trackPan[t]->get();
            mutes[t] = m_trackMute[t]->get();
            solos[t] = m_trackSolo[t]->get();
            accents[t] = m_trackAccent[t]->get();
        }

        m_mixer.renderStep(step, m_patternGrid, volumes, pans, mutes, solos, accents, buffer, m_sampleRate);
        m_mixer.applySoftClip(buffer);

        m_sysExBeatCounter++;
        if (m_sysExBeatCounter >= 4) {
            m_sysExBeatCounter = 0;
            sendTransportState(midiMessages);
        }
        if (step == 0) sendSongFeedback(midiMessages);
    }
    else
    {
        m_sysExBeatCounter = 0;
    }
}

//==============================================================================
void LolooperAudioProcessor::handleSysEx(const juce::MidiMessage& msg, juce::MidiBuffer& out)
{
    auto data = msg.getSysExData();
    auto size = msg.getSysExDataSize();
    if (size < 3 || data[0] != 0x7D) return;

    uint8_t cmd = data[1];

    // 0x01 = Request patterns (PWA -> Plugin)
    if (cmd == 0x01)
    {
        // Respond with all patterns as JSON
        auto json = m_patternLibrary.exportAllToJson();
        auto jsonBytes = json.toUTF8();
        int jsonLen = (int)std::strlen(jsonBytes);

        // Build SysEx response with heap-allocated buffer (no VLA for MSVC compat)
        juce::HeapBlock<juce::uint8> sysexData(2 + jsonLen);
        sysexData[0] = 0x7D;
        sysexData[1] = 0x02; // Response
        std::memcpy(sysexData + 2, jsonBytes, (size_t)jsonLen);

        out.addEvent(juce::MidiMessage::createSysExMessage(sysexData, (int)(2 + jsonLen)), 0);
    }
    // 0x03 = Push patterns (PWA -> Plugin)
    else if (cmd == 0x03 && size > 3)
    {
        // Strip trailing 0xF7 if present
        int jsonBytes = (int)size - 2;
        if (data[size - 1] == 0xF7) jsonBytes--;
        auto jsonStr = juce::String::fromUTF8((const char*)(data + 2), jsonBytes);
        m_patternLibrary.importFromJson(jsonStr);

        int activeIdx = m_patternStyleParam->getIndex();
        m_patternLibrary.setActiveStyleIndex(activeIdx);
        syncPatternGridFromLibrary();
    }
    // 0x04 = Song structure update (PWA -> Plugin)
    else if (cmd == 0x04 && size > 3)
    {
        int jsonBytes = (int)size - 2;
        if (data[size - 1] == 0xF7) jsonBytes--;
        auto jsonStr = juce::String::fromUTF8((const char*)(data + 2), jsonBytes);
        auto var = juce::JSON::parse(jsonStr);

        m_songSections.clear();
        m_songModeActive = false;

        if (auto* obj = var.getDynamicObject())
        {
            if (auto* sectionsArr = obj->getProperty("sections").getArray())
            {
                for (auto& secVar : *sectionsArr)
                {
                    if (auto* sec = secVar.getDynamicObject())
                    {
                        SongSection s;
                        s.name = sec->getProperty("name").toString();
                        s.patternStyle = sec->getProperty("pattern").toString();
                        s.bars = (int)sec->getProperty("bars");
                        s.loop = (bool)sec->getProperty("loop");
                        m_songSections.add(s);
                    }
                }
                m_songModeActive = m_songSections.size() > 0;
                m_currentSectionIndex = 0;
                m_barsInCurrentSection = 0;
                loadSongSectionPattern(0);
            }
        }
    }
}

//==============================================================================
void LolooperAudioProcessor::loadSongSectionPattern(int sectionIndex)
{
    if (sectionIndex < 0 || sectionIndex >= m_songSections.size()) return;

    auto section = m_songSections[sectionIndex];
    const auto* pattern = m_patternLibrary.getPattern(section.patternStyle);
    if (pattern)
    {
        for (int t = 0; t < 14; ++t)
            for (int s = 0; s < 16; ++s)
                m_patternGrid[t][s] = pattern->getVelocity(t, s);

        auto names = m_patternLibrary.getStyleNames();
        for (int i = 0; i < names.size(); ++i)
        {
            if (names[i] == section.patternStyle)
            {
                *m_patternStyleParam = i;
                m_currentPatternStyle = i;
                break;
            }
        }
    }
}

void LolooperAudioProcessor::advanceToNextSection()
{
    if (!m_songModeActive || m_songSections.size() == 0) return;
    m_currentSectionIndex = (m_currentSectionIndex + 1) % m_songSections.size();
    m_barsInCurrentSection = 0;
    loadSongSectionPattern(m_currentSectionIndex);
}

void LolooperAudioProcessor::goToPreviousSection()
{
    if (!m_songModeActive || m_songSections.size() == 0) return;
    m_currentSectionIndex = (m_currentSectionIndex - 1 + m_songSections.size()) % m_songSections.size();
    m_barsInCurrentSection = 0;
    loadSongSectionPattern(m_currentSectionIndex);
}

int LolooperAudioProcessor::getCurrentSectionBars() const
{
    if (m_currentSectionIndex < 0 || m_currentSectionIndex >= m_songSections.size()) return 0;
    // Need to access by copy since juce::Array may return rvalue
    auto sec = m_songSections[m_currentSectionIndex];
    return sec.bars;
}

bool LolooperAudioProcessor::isCurrentSectionLooping() const
{
    if (m_currentSectionIndex < 0 || m_currentSectionIndex >= m_songSections.size()) return false;
    auto sec = m_songSections[m_currentSectionIndex];
    return sec.loop;
}

//==============================================================================
void LolooperAudioProcessor::sendTransportState(juce::MidiBuffer& midi)
{
    juce::uint8 d[] = {
        0x7D, 0x05,
        (juce::uint8)(m_sequencer.getCurrentBar() & 0xFF),
        (juce::uint8)(m_sequencer.getCurrentBeat() & 0xFF),
        (juce::uint8)(m_sequencer.getCurrentStep() & 0xFF),
        (juce::uint8)(m_isPlaying ? 1 : 0),
        (juce::uint8)(m_isRecording ? 1 : 0)
    };
    midi.addEvent(juce::MidiMessage::createSysExMessage(d, sizeof(d)), 0);
}

void LolooperAudioProcessor::sendSongFeedback(juce::MidiBuffer& midi)
{
    juce::uint8 d[] = {
        0x7D, 0x06,
        (juce::uint8)m_currentSectionIndex,
        (juce::uint8)m_barsInCurrentSection,
        (juce::uint8)m_songSections.size()
    };
    midi.addEvent(juce::MidiMessage::createSysExMessage(d, sizeof(d)), 0);
}

//==============================================================================
void LolooperAudioProcessor::sendRecordedPattern(juce::MidiBuffer& midi)
{
    // Build a JSON-like pattern from the recorded hits
    // SysEx 0x07: [0x7D, 0x07, track, step, velocity, ...]
    // Each hit is 3 bytes: track(1), step(1), velocity(as 0-127)
    int numHits = m_recBuffer.size();
    juce::HeapBlock<juce::uint8> sysexData(2 + numHits * 3);
    sysexData[0] = 0x7D;
    sysexData[1] = 0x07;
    for (int i = 0; i < numHits; ++i)
    {
        sysexData[2 + i * 3] = (juce::uint8)m_recBuffer[i].track;
        sysexData[2 + i * 3 + 1] = (juce::uint8)m_recBuffer[i].step;
        sysexData[2 + i * 3 + 2] = (juce::uint8)(m_recBuffer[i].velocity * 127.0f);
    }
    midi.addEvent(juce::MidiMessage::createSysExMessage(sysexData, 2 + numHits * 3), 0);
}

//==============================================================================
void LolooperAudioProcessor::sendChordDetected(const juce::String& chordName, juce::MidiBuffer& midi)
{
    // SysEx 0x09: Detected chord
    // [0x7D, 0x09, chordName...]
    auto utf8 = chordName.toUTF8();
    int len = (int)std::strlen(utf8);
    juce::HeapBlock<juce::uint8> sysexData(2 + len);
    sysexData[0] = 0x7D;
    sysexData[1] = 0x09;
    std::memcpy(sysexData + 2, utf8, (size_t)len);
    midi.addEvent(juce::MidiMessage::createSysExMessage(sysexData, 2 + len), 0);
}

//==============================================================================
bool LolooperAudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* LolooperAudioProcessor::createEditor()
{
    return new LolooperAudioProcessorEditor(*this);
}

//==============================================================================
void LolooperAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = juce::ValueTree("Lolooper");
    state.setProperty("bpm", m_bpmParam->get(), nullptr);
    state.setProperty("swing", m_swingParam->get(), nullptr);
    state.setProperty("humanize", m_humanizeParam->get(), nullptr);
    state.setProperty("patternStyle", m_patternStyleParam->getIndex(), nullptr);

    for (int t = 0; t < 14; ++t) {
        auto tr = juce::ValueTree("Track");
        tr.setProperty("index", t, nullptr);
        tr.setProperty("volume", (double)m_trackVolume[t]->get(), nullptr);
        tr.setProperty("pan", (double)m_trackPan[t]->get(), nullptr);
        tr.setProperty("mute", m_trackMute[t]->get(), nullptr);
        tr.setProperty("solo", m_trackSolo[t]->get(), nullptr);
        tr.setProperty("accent", (double)m_trackAccent[t]->get(), nullptr);
        state.addChild(tr, -1, nullptr);
    }

    state.setProperty("patternLibrary", m_patternLibrary.exportAllToJson(), nullptr);

    juce::MemoryOutputStream stream(destData, false);
    state.writeToStream(stream);
}

void LolooperAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    auto state = juce::ValueTree::readFromData(data, size_t(sizeInBytes));
    if (!state.isValid() || !state.hasType("Lolooper")) return;

    if (state.hasProperty("bpm")) *m_bpmParam = (float)state.getProperty("bpm");
    if (state.hasProperty("swing")) *m_swingParam = (float)state.getProperty("swing");
    if (state.hasProperty("humanize")) *m_humanizeParam = (float)state.getProperty("humanize");
    if (state.hasProperty("patternStyle")) {
        int idx = (int)state.getProperty("patternStyle");
        *m_patternStyleParam = juce::jlimit(0, m_patternLibrary.size() - 1, idx);
        m_currentPatternStyle = *m_patternStyleParam;
        m_patternLibrary.setActiveStyleIndex(m_currentPatternStyle);
    }

    if (state.hasProperty("patternLibrary"))
    {
        auto libJson = state.getProperty("patternLibrary").toString();
        m_patternLibrary.importFromJson(libJson);
        m_patternLibrary.setActiveStyleIndex(m_currentPatternStyle);
        syncPatternGridFromLibrary();
    }

    for (int t = 0; t < state.getNumChildren(); ++t) {
        auto ch = state.getChild(t);
        if (ch.hasType("Track")) {
            int idx = (int)ch.getProperty("index");
            if (idx >= 0 && idx < 14) {
                if (ch.hasProperty("volume")) *m_trackVolume[idx] = (float)ch.getProperty("volume");
                if (ch.hasProperty("pan")) *m_trackPan[idx] = (float)ch.getProperty("pan");
                if (ch.hasProperty("mute")) *m_trackMute[idx] = (bool)ch.getProperty("mute");
                if (ch.hasProperty("solo")) *m_trackSolo[idx] = (bool)ch.getProperty("solo");
                if (ch.hasProperty("accent")) *m_trackAccent[idx] = (float)ch.getProperty("accent");
            }
        }
    }
}

//==============================================================================
void LolooperAudioProcessor::loadDefaultPatterns()
{
    // Try current working directory first
    auto dir = juce::File::getCurrentWorkingDirectory().getChildFile("data").getChildFile("patterns");
    if (!dir.isDirectory())
    {
        // Fallback: look relative to the plugin binary
        auto binDir = juce::File::getSpecialLocation(juce::File::currentExecutableFile).getParentDirectory();
        dir = binDir.getChildFile("data").getChildFile("patterns");
    }
    m_patternLibrary.loadFromDirectory(dir);

    if (m_patternLibrary.size() > 0) {
        m_patternLibrary.setActiveStyleIndex(m_currentPatternStyle);
        syncPatternGridFromLibrary();
    }
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LolooperAudioProcessor();
}
