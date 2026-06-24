/*
 * Copyright (c) 2025 Fernando Passos. All rights reserved.
 * Licensed under the MIT License.
 *
 * Lolooper -- Live performance looper for Brazilian music
 */

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

enum HitArea { kNone, kTransportPlay, kTransportStop, kTransportRec,
               kStyleBtn, kStylePrev, kStyleNext,
               kBpmLabel, kSwingTrack, kHumanizeTrack,
               kGridCellHit, kTrackMute, kTrackSolo, kTrackVolume, kTrackPan,
               kChordToggle };

class LolooperAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit LolooperAudioProcessorEditor(LolooperAudioProcessor&);
    ~LolooperAudioProcessorEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void doLayout();
    
    void drawTransport(juce::Graphics& g, juce::Rectangle<int> area);
    void drawMiniGrid(juce::Graphics& g, juce::Rectangle<int> area);
    void drawTrackMixer(juce::Graphics& g, juce::Rectangle<int> area);
    void drawChordIndicator(juce::Graphics& g, juce::Rectangle<int> area);
    void drawStatusBar(juce::Graphics& g, juce::Rectangle<int> area);
    
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    
    HitArea findHitArea(int x, int y);

    static constexpr int kTransportH = 90;
    static constexpr int kStatusH = 28;
    static constexpr int kGridCellH = 14;
    static constexpr int kTrackLabelW = 70;
    static constexpr int kMixerColW = 180;
    int m_cellW = 13;

    juce::Rectangle<int> m_playBtn, m_stopBtn, m_recBtn;
    juce::Rectangle<int> m_bpmRect, m_styleRect, m_stylePrevRect, m_styleNextRect;
    juce::Rectangle<int> m_swingRect, m_humanizeRect;
    juce::Rectangle<int> m_transportRect, m_gridRect, m_mixerRect;
    juce::Rectangle<int> m_chordRect, m_statusRect;
    juce::Rectangle<int> m_trackRects[14], m_muteBtns[14], m_soloBtns[14];
    juce::Rectangle<int> m_volRects[14], m_panRects[14];

    HitArea m_dragArea = kNone;
    int m_dragTrack = -1;
    juce::Point<float> m_dragStart;
    float m_dragStartValue = 0.0f;

    LolooperAudioProcessor& m_processor;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LolooperAudioProcessorEditor)
};
