/*
 * Copyright (c) 2025 Fernando Passos. All rights reserved.
 * Licensed under the MIT License.
 *
 * Lolooper -- Live performance looper for Brazilian music
 */

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <juce_graphics/juce_graphics.h>

// Color scheme
static const juce::Colour kBg(15, 15, 22);
static const juce::Colour kSurface(28, 28, 42);
static const juce::Colour kBorder(60, 60, 85);
static const juce::Colour kAccent = juce::Colour::fromFloatRGBA(0.39f, 0.40f, 0.94f, 1.0f);
static const juce::Colour kAccentDim = juce::Colour::fromFloatRGBA(0.39f, 0.40f, 0.94f, 0.24f);
static const juce::Colour kRed = juce::Colour::fromFloatRGBA(0.94f, 0.27f, 0.27f, 1.0f);
static const juce::Colour kGreen = juce::Colour::fromFloatRGBA(0.13f, 0.77f, 0.37f, 1.0f);
static const juce::Colour kYellow = juce::Colour::fromFloatRGBA(0.92f, 0.70f, 0.03f, 1.0f);
static const juce::Colour kText(220, 220, 240);
static const juce::Colour kTextDim(140, 140, 170);
static const juce::Colour kTextFaint(80, 80, 100);
static const juce::Colour kGridCellColor = juce::Colour::fromFloatRGBA(0.39f, 0.40f, 0.94f, 0.70f);
static const juce::Colour kBeatLine = juce::Colour::fromFloatRGBA(0.31f, 0.31f, 0.43f, 0.31f);

//==============================================================================
LolooperAudioProcessorEditor::LolooperAudioProcessorEditor(LolooperAudioProcessor& p)
    : AudioProcessorEditor(&p), m_processor(p)
{
    setSize(780, 520);
    setResizable(true, true);
    setResizeLimits(600, 400, 1200, 800);
    startTimerHz(25);
}

LolooperAudioProcessorEditor::~LolooperAudioProcessorEditor() { stopTimer(); }

//==============================================================================
void LolooperAudioProcessorEditor::resized()
{
    doLayout();
    repaint();
}

void LolooperAudioProcessorEditor::doLayout()
{
    auto bounds = getLocalBounds();
    if (bounds.getWidth() < 100 || bounds.getHeight() < 100) return;
    auto area = bounds.reduced(6);
    if (area.getHeight() < kTransportH + kStatusH) return;

    // === Transport ===
    auto tr = area.removeFromTop(kTransportH).reduced(10);
    int btnSize = 46;
    int btnY = tr.getCentreY() - btnSize/2;
    m_playBtn = juce::Rectangle<int>(tr.getX(), btnY, btnSize, btnSize);
    m_stopBtn = juce::Rectangle<int>(m_playBtn.getRight() + 6, btnY, btnSize, btnSize);
    m_recBtn  = juce::Rectangle<int>(m_stopBtn.getRight() + 6, btnY, btnSize, btnSize);
    
    int centerX = m_recBtn.getRight() + 30;
    m_bpmRect = juce::Rectangle<int>(centerX, tr.getCentreY() - 28, 100, 56);
    m_styleRect = juce::Rectangle<int>(m_bpmRect.getRight() + 20, m_bpmRect.getY(), 130, 56);
    m_stylePrevRect = juce::Rectangle<int>(m_styleRect.getX() - 20, m_styleRect.getCentreY() - 10, 18, 20);
    m_styleNextRect = juce::Rectangle<int>(m_styleRect.getRight() + 2, m_styleRect.getCentreY() - 10, 18, 20);
    
    int rightX = m_styleRect.getRight() + 30;
    int sliderW = juce::jmin(120, tr.getRight() - rightX - 10);
    m_swingRect = juce::Rectangle<int>(rightX, tr.getCentreY() - 20, sliderW, 20);
    m_humanizeRect = juce::Rectangle<int>(rightX, tr.getCentreY() + 4, sliderW, 20);
    
    m_transportRect = juce::Rectangle<int>(0, 0, 0, 0); // placeholder

    // === Main area ===
    auto mainArea = area.removeFromTop(area.getHeight() - kStatusH - 4);
    if (mainArea.getWidth() < 200 || mainArea.getHeight() < 50) return;
    auto gridArea = mainArea.removeFromLeft(mainArea.getWidth() - kMixerColW - 4);
    mainArea.removeFromLeft(4);
    auto mixerArea = mainArea;
    
    m_gridRect = gridArea;
    m_mixerRect = mixerArea;

    // Grid cell size
    auto gArea = gridArea.reduced(8);
    int cellW = (gArea.getWidth() > kTrackLabelW) 
        ? juce::jmin(kGridCellH, (gArea.getWidth() - kTrackLabelW - 6) / 16) : kGridCellH;
    if (cellW < 2) cellW = 2;
    m_cellW = cellW;

    // Track mixer rows
    auto mA = mixerArea.reduced(6);
    int trackH = juce::jmin(22, (mA.getHeight() - 12) / 14);
    for (int t = 0; t < 14; ++t) {
        int y = mA.getY() + 4 + t * trackH + (t >= 7 ? 3 : 0);
        auto tr = juce::Rectangle<int>(mA.getX(), y, mA.getWidth(), trackH);
        m_trackRects[t] = tr;
        auto tr2 = tr;
        tr2.removeFromLeft(56);
        m_muteBtns[t] = juce::Rectangle<int>(tr2.getX(), tr2.getY(), 16, tr2.getHeight());
        tr2.removeFromLeft(18);
        m_soloBtns[t] = juce::Rectangle<int>(tr2.getX(), tr2.getY(), 16, tr2.getHeight());
        tr2.removeFromLeft(18);
        m_volRects[t] = tr2.removeFromLeft(tr2.getWidth() / 2 - 2).reduced(1, 4);
        tr2.removeFromLeft(2);
        m_panRects[t] = tr2.reduced(1, 4);
    }

    // Chord indicator
    m_chordRect = area.removeFromTop(22).reduced(0, 2);
    m_statusRect = area;
}

//==============================================================================
void LolooperAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(kBg);
    auto bounds = getLocalBounds();
    if (bounds.getWidth() < 100 || bounds.getHeight() < 100) return;
    
    auto area = bounds.reduced(6);
    if (area.getHeight() < kTransportH + kStatusH) return;

    // Transport
    auto trR = area.removeFromTop(kTransportH);
    drawTransport(g, trR);

    // Main
    auto mainArea = area.removeFromTop(area.getHeight() - kStatusH - 4);
    if (mainArea.getWidth() < 200 || mainArea.getHeight() < 50) return;
    auto gridA = mainArea.removeFromLeft(mainArea.getWidth() - kMixerColW - 4);
    mainArea.removeFromLeft(4);
    drawMiniGrid(g, gridA);
    drawTrackMixer(g, mainArea);

    // Chord
    if (area.getHeight() > 22) {
        auto cr = area.removeFromTop(22).reduced(0, 2);
        drawChordIndicator(g, cr);
    }
    drawStatusBar(g, area);
}

//==============================================================================
void LolooperAudioProcessorEditor::drawTransport(juce::Graphics& g, juce::Rectangle<int> r)
{
    g.setColour(kSurface);
    g.fillRoundedRectangle(r.toFloat(), 10.0f);
    g.setColour(kBorder);
    g.drawRoundedRectangle(r.toFloat(), 10.0f, 1.0f);

    auto area = r.reduced(10);

    auto drawCircBtn = [&](juce::Rectangle<int> btn, juce::Colour c, const char* icon, bool active) {
        g.setColour(active ? c.withAlpha(0.3f) : juce::Colours::white.withAlpha(0.1f));
        g.fillEllipse(btn.toFloat());
        g.setColour(active ? c.withAlpha(0.6f) : juce::Colours::white.withAlpha(0.2f));
        g.drawEllipse(btn.toFloat(), 1.5f);
        g.setColour(active ? juce::Colours::white : juce::Colours::white.withAlpha(0.5f));
        g.setFont(juce::Font(juce::FontOptions(18.0f).withStyle("Bold")));
        g.drawText(icon, btn, juce::Justification::centred);
    };

    drawCircBtn(m_playBtn, kAccent, m_processor.m_isPlaying ? "||" : ">", m_processor.m_isPlaying);
    drawCircBtn(m_stopBtn, juce::Colours::white, "[]", false);
    drawCircBtn(m_recBtn, kRed, "O", m_processor.m_isRecording);

    // BPM
    int bpm = (int)m_processor.m_bpmParam->get();
    g.setColour(kTextDim);
    g.setFont(juce::Font(juce::FontOptions(10.0f)));
    g.drawText("BPM", juce::Rectangle<int>(m_bpmRect.getX(), m_bpmRect.getY(), m_bpmRect.getWidth(), 14), juce::Justification::centred);
    g.setColour(kText);
    g.setFont(juce::Font(juce::FontOptions(32.0f).withStyle("Bold")));
    g.drawText(juce::String(bpm), juce::Rectangle<int>(m_bpmRect.getX(), m_bpmRect.getY() + 14, m_bpmRect.getWidth(), 42), juce::Justification::centred);

    // Style
    g.setColour(kTextDim);
    g.setFont(juce::Font(juce::FontOptions(10.0f)));
    g.drawText("STYLE", juce::Rectangle<int>(m_styleRect.getX(), m_styleRect.getY(), m_styleRect.getWidth(), 14), juce::Justification::centred);
    g.setColour(kAccent);
    g.setFont(juce::Font(juce::FontOptions(13.0f).withStyle("Bold")));
    juce::String styleName = "--";
    if (m_processor.m_patternLibrary.size() > 0 && m_processor.m_currentPatternStyle < 9)
        styleName = LolooperAudioProcessor::styleNames[m_processor.m_currentPatternStyle];
    g.drawText(styleName, juce::Rectangle<int>(m_styleRect.getX(), m_styleRect.getY() + 14, m_styleRect.getWidth(), 42), juce::Justification::centred);

    // Style arrows
    g.setColour(kTextDim);
    g.setFont(juce::Font(juce::FontOptions(12.0f).withStyle("Bold")));
    g.drawText("<", m_stylePrevRect, juce::Justification::centred);
    g.drawText(">", m_styleNextRect, juce::Justification::centred);

    // Swing
    float swing = m_processor.m_swingParam->get() * 100.0f;
    g.setColour(kTextDim);
    g.setFont(juce::Font(juce::FontOptions(10.0f)));
    g.drawText("SWING", m_swingRect.withHeight(12), juce::Justification::centredLeft);
    auto swTrack = m_swingRect.withTrimmedTop(14).withTrimmedBottom(2);
    g.setColour(juce::Colours::white.withAlpha(0.08f));
    g.fillRoundedRectangle(swTrack.toFloat(), 3.0f);
    g.setColour(kAccent.withAlpha(0.5f));
    g.fillRoundedRectangle(swTrack.withRight(swTrack.getX() + (int)(swing / 100.0f * swTrack.getWidth())).toFloat(), 3.0f);
    g.setColour(kText);
    g.setFont(juce::Font(juce::FontOptions(8.0f)));
    g.drawText(juce::String((int)swing), swTrack, juce::Justification::centred);

    // Humanize
    float hum = m_processor.m_humanizeParam->get() * 50.0f;
    g.setColour(kTextDim);
    g.setFont(juce::Font(juce::FontOptions(10.0f)));
    g.drawText("HUMAN", m_humanizeRect.withHeight(12), juce::Justification::centredLeft);
    auto huTrack = m_humanizeRect.withTrimmedTop(14).withTrimmedBottom(2);
    g.setColour(juce::Colours::white.withAlpha(0.08f));
    g.fillRoundedRectangle(huTrack.toFloat(), 3.0f);
    g.setColour(kAccent.withAlpha(0.5f));
    g.fillRoundedRectangle(huTrack.withRight(huTrack.getX() + (int)(hum / 50.0f * huTrack.getWidth())).toFloat(), 3.0f);
    g.drawText(juce::String((int)hum), huTrack, juce::Justification::centred);
}

void LolooperAudioProcessorEditor::drawMiniGrid(juce::Graphics& g, juce::Rectangle<int> r)
{
    g.setColour(kSurface);
    g.fillRoundedRectangle(r.toFloat(), 10.0f);
    g.setColour(kBorder);
    g.drawRoundedRectangle(r.toFloat(), 10.0f, 1.0f);

    auto area = r.reduced(8);
    g.setFont(juce::Font(juce::FontOptions(8.0f)));
    int cellW = m_cellW;
    int activeStep = m_processor.m_sequencer.getCurrentStep();

    for (int t = 0; t < 14; ++t) {
        int y = area.getY() + t * kGridCellH + (t >= 7 ? 4 : 0);
        g.setColour(kTextDim);
        g.drawText(LolooperAudioProcessor::trackNames[t],
                   juce::Rectangle<int>(area.getX(), y, kTrackLabelW, kGridCellH),
                   juce::Justification::centredLeft);
        for (int s = 0; s < 16; ++s) {
            int x = area.getX() + kTrackLabelW + 6 + s * cellW;
            float vel = m_processor.m_patternGrid[t][s];
            bool isActive = (s == activeStep && m_processor.m_isPlaying);
            auto cellRect = juce::Rectangle<float>((float)x, (float)y, (float)cellW - 1.0f, (float)kGridCellH - 1.0f);
            
            if (vel > 0.0f) {
                g.setColour(kGridCellColor.withAlpha(vel * 0.9f + 0.1f));
                g.fillRoundedRectangle(cellRect, 2.0f);
            } else if (s > 0 && (s % 4 == 0)) {
                g.setColour(kBeatLine);
                g.fillRect(cellRect.withWidth(1.0f));
            } else {
                g.setColour(juce::Colours::white.withAlpha(0.03f));
                g.fillRoundedRectangle(cellRect, 2.0f);
            }
            if (isActive) {
                g.setColour(juce::Colours::white.withAlpha(0.3f));
                g.drawRoundedRectangle(cellRect.expanded(1.0f), 3.0f, 1.5f);
            }
        }
    }
}

void LolooperAudioProcessorEditor::drawTrackMixer(juce::Graphics& g, juce::Rectangle<int> r)
{
    g.setColour(kSurface);
    g.fillRoundedRectangle(r.toFloat(), 10.0f);
    g.setColour(kBorder);
    g.drawRoundedRectangle(r.toFloat(), 10.0f, 1.0f);
    
    g.setFont(juce::Font(juce::FontOptions(7.0f)));
    int activeStep = m_processor.m_sequencer.getCurrentStep();

    for (int t = 0; t < 14; ++t) {
        auto tr = m_trackRects[t];
        if (tr.isEmpty()) continue;
        
        bool muted = m_processor.m_trackMute[t]->get();
        bool soloed = m_processor.m_trackSolo[t]->get();
        bool active = m_processor.m_patternGrid[t][activeStep] > 0.0f && m_processor.m_isPlaying && !muted;
        float vol = m_processor.m_trackVolume[t]->get();
        float pan = m_processor.m_trackPan[t]->get();

        g.setColour(muted ? juce::Colours::white.withAlpha(0.02f)
                  : soloed ? kYellow.withAlpha(0.08f)
                  : active ? kAccentDim.withAlpha(0.15f)
                  : juce::Colours::white.withAlpha(0.02f));
        g.fillRoundedRectangle(tr.toFloat(), 3.0f);

        auto nameRect = tr.removeFromLeft(56).reduced(2, 0);
        g.setColour(muted ? kTextFaint : (active ? kText : kTextDim));
        g.drawText(LolooperAudioProcessor::trackNames[t], nameRect, juce::Justification::centredLeft);

        // Mute
        g.setColour(muted ? kRed.withAlpha(0.5f) : juce::Colours::white.withAlpha(0.1f));
        g.fillRoundedRectangle(m_muteBtns[t].toFloat(), 2.0f);
        g.setColour(muted ? kRed : kTextDim);
        g.drawText("M", m_muteBtns[t], juce::Justification::centred);
        tr.removeFromLeft(18);

        // Solo
        g.setColour(soloed ? kYellow.withAlpha(0.5f) : juce::Colours::white.withAlpha(0.1f));
        g.fillRoundedRectangle(m_soloBtns[t].toFloat(), 2.0f);
        g.setColour(soloed ? kYellow : kTextDim);
        g.drawText("S", m_soloBtns[t], juce::Justification::centred);
        tr.removeFromLeft(18);

        // Volume
        g.setColour(juce::Colours::white.withAlpha(0.06f));
        g.fillRoundedRectangle(m_volRects[t].toFloat(), 2.0f);
        g.setColour(active ? kGreen.withAlpha(0.6f) : kAccent.withAlpha(0.3f));
        g.fillRoundedRectangle(m_volRects[t].withRight(
            m_volRects[t].getX() + (int)(vol * m_volRects[t].getWidth())).toFloat(), 2.0f);

        // Pan
        g.setColour(juce::Colours::white.withAlpha(0.06f));
        g.fillRoundedRectangle(m_panRects[t].toFloat(), 2.0f);
        float panPos = m_panRects[t].getX() + pan * m_panRects[t].getWidth();
        g.setColour(kAccent.withAlpha(0.5f));
        g.fillRect(panPos - 1.0f, (float)m_panRects[t].getY(), 3.0f, (float)m_panRects[t].getHeight());
    }
}

void LolooperAudioProcessorEditor::drawChordIndicator(juce::Graphics& g, juce::Rectangle<int> r)
{
    if (r.isEmpty()) return;
    g.setColour(kSurface);
    g.fillRoundedRectangle(r.toFloat(), 5.0f);
    juce::String chord = m_processor.m_chordProgression.getCurrentChordName();
    if (chord.isNotEmpty()) {
        g.setColour(kAccent);
        g.setFont(juce::Font(juce::FontOptions(11.0f).withStyle("Bold")));
        g.drawText("CHORD: " + chord, r, juce::Justification::centred);
    }
}

void LolooperAudioProcessorEditor::drawStatusBar(juce::Graphics& g, juce::Rectangle<int> r)
{
    if (r.isEmpty()) return;
    g.setColour(kSurface);
    g.fillRoundedRectangle(r.toFloat(), 6.0f);
    auto bar = m_processor.m_sequencer.getCurrentBar();
    auto beat = m_processor.m_sequencer.getCurrentBeat() + 1;
    auto step = m_processor.m_sequencer.getCurrentStep() + 1;
    juce::String posText = juce::String(bar) + "." + juce::String(beat) + "." + juce::String(step);
    
    g.setColour(kAccent);
    g.setFont(juce::Font(juce::FontOptions(12.0f).withStyle("Bold")));
    g.drawText(posText, r.removeFromLeft(80), juce::Justification::centred);

    auto dotRect = juce::Rectangle<int>(90, r.getCentreY() - 4, 8, 8);
    g.setColour(m_processor.m_isPlaying ? kGreen : juce::Colours::white.withAlpha(0.2f));
    g.fillEllipse(dotRect.toFloat());
    g.setColour(kTextDim);
    g.setFont(juce::Font(juce::FontOptions(8.0f)));
    g.drawText(m_processor.m_isPlaying ? "PLAYING" : "STOPPED", juce::Rectangle<int>(102, r.getY(), 70, r.getHeight()), juce::Justification::centredLeft);

    if (m_processor.m_isRecording) {
        g.setColour(kRed);
        auto rd = juce::Rectangle<int>(174, r.getCentreY() - 4, 8, 8);
        g.fillEllipse(rd.toFloat());
        g.drawText("REC", juce::Rectangle<int>(186, r.getY(), 30, r.getHeight()), juce::Justification::centredLeft);
    }
    g.setColour(kTextFaint);
    g.setFont(juce::Font(juce::FontOptions(7.0f)));
    g.drawText("Lolooper v1.0", r.withTrimmedLeft(r.getWidth() - 90), juce::Justification::centredRight);
}

//==============================================================================
void LolooperAudioProcessorEditor::timerCallback() { repaint(); }

//==============================================================================
HitArea LolooperAudioProcessorEditor::findHitArea(int x, int y)
{
    if (m_playBtn.contains(x, y)) return kTransportPlay;
    if (m_stopBtn.contains(x, y)) return kTransportStop;
    if (m_recBtn.contains(x, y)) return kTransportRec;
    if (m_bpmRect.contains(x, y)) return kBpmLabel;
    if (m_stylePrevRect.contains(x, y)) return kStylePrev;
    if (m_styleNextRect.contains(x, y)) return kStyleNext;
    if (m_styleRect.contains(x, y)) return kStyleBtn;
    if (m_swingRect.contains(x, y)) return kSwingTrack;
    if (m_humanizeRect.contains(x, y)) return kHumanizeTrack;
    if (!m_chordRect.isEmpty() && m_chordRect.contains(x, y)) return kChordToggle;
    if (m_gridRect.contains(x, y)) return kGridCellHit;

    for (int t = 0; t < 14; ++t) {
        if (m_muteBtns[t].contains(x, y)) return kTrackMute;
        if (m_soloBtns[t].contains(x, y)) return kTrackSolo;
        if (m_volRects[t].contains(x, y)) return kTrackVolume;
        if (m_panRects[t].contains(x, y)) return kTrackPan;
    }
    return kNone;
}

static int findTrack(int x, int y, const juce::Rectangle<int>* rects) {
    for (int t = 0; t < 14; ++t)
        if (rects[t].contains(x, y)) return t;
    return -1;
}

void LolooperAudioProcessorEditor::mouseDown(const juce::MouseEvent& e)
{
    auto p = e.position;
    auto area = findHitArea((int)p.x, (int)p.y);

    switch (area) {
        case kTransportPlay:
            m_processor.m_isPlaying = !m_processor.m_isPlaying;
            m_processor.m_isPlaying ? m_processor.m_sequencer.play() : m_processor.m_sequencer.pause();
            break;
        case kTransportStop:
            m_processor.m_isPlaying = false; m_processor.m_sequencer.stop();
            break;
        case kTransportRec:
            m_processor.m_isRecording = !m_processor.m_isRecording;
            break;
        case kBpmLabel: {
            // Drag to change BPM: click and drag up/down
            m_dragArea = kBpmLabel;
            m_dragStartValue = m_processor.m_bpmParam->get();
            m_dragStart = p;
            break;
        }
        case kStylePrev:
        case kStyleNext:
        case kStyleBtn: {
            int libSize = m_processor.m_patternLibrary.size();
            if (libSize == 0) break;
            int idx = m_processor.m_patternStyleParam->getIndex();
            idx = (area == kStylePrev) ? (idx - 1 + libSize) % libSize : (idx + 1) % libSize;
            *m_processor.m_patternStyleParam = idx;
            m_processor.m_currentPatternStyle = idx;
            m_processor.m_patternLibrary.setActiveStyleIndex(idx);
            m_processor.syncPatternGridFromLibrary();
            break;
        }
        case kTrackMute: {
            int t = findTrack((int)p.x, (int)p.y, m_muteBtns);
            if (t >= 0) *m_processor.m_trackMute[t] = !m_processor.m_trackMute[t]->get();
            break;
        }
        case kTrackSolo: {
            int t = findTrack((int)p.x, (int)p.y, m_soloBtns);
            if (t >= 0) *m_processor.m_trackSolo[t] = !m_processor.m_trackSolo[t]->get();
            break;
        }
        case kTrackVolume: {
            int t = findTrack((int)p.x, (int)p.y, m_volRects);
            if (t >= 0) {
                m_dragTrack = t; m_dragArea = kTrackVolume;
                float frac = (float)(p.x - m_volRects[t].getX()) / m_volRects[t].getWidth();
                *m_processor.m_trackVolume[t] = juce::jlimit(0.0f, 1.0f, frac);
            }
            break;
        }
        case kTrackPan: {
            int t = findTrack((int)p.x, (int)p.y, m_panRects);
            if (t >= 0) {
                m_dragTrack = t; m_dragArea = kTrackPan;
                float frac = (float)(p.x - m_panRects[t].getX()) / m_panRects[t].getWidth();
                *m_processor.m_trackPan[t] = juce::jlimit(0.0f, 1.0f, frac);
            }
            break;
        }
        case kSwingTrack: {
            float frac = (float)(p.x - m_swingRect.getX()) / m_swingRect.getWidth();
            *m_processor.m_swingParam = juce::jlimit(0.0f, 1.0f, frac);
            break;
        }
        case kHumanizeTrack: {
            float frac = (float)(p.x - m_humanizeRect.getX()) / m_humanizeRect.getWidth();
            *m_processor.m_humanizeParam = juce::jlimit(0.0f, 1.0f, frac);
            break;
        }
        case kChordToggle:
            m_processor.setChordDetectEnabled(!m_processor.isChordDetectEnabled());
            break;
        case kGridCellHit: {
            // Convert click position to grid cell coordinates
            auto ga = m_gridRect.reduced(8);
            int cellX = (int)p.x - (ga.getX() + kTrackLabelW + 6);
            int cellY = (int)p.y - ga.getY();
            if (cellX >= 0 && cellY >= 0) {
                int step = cellX / m_cellW;
                // Adjust track for the gap between track groups (after track 6)
                int track = cellY / kGridCellH;
                if (track >= 7) {
                    int adjustedY = cellY - 4; // gap after group 1
                    track = adjustedY / kGridCellH;
                }
                if (track >= 0 && track < 14 && step >= 0 && step < 16) {
                    // Toggle: 0 → 0.4 → 0.7 → 1.0 → 0
                    float current = m_processor.m_patternGrid[track][step];
                    float next;
                    if (current < 0.1f) next = 0.4f;
                    else if (current < 0.5f) next = 0.7f;
                    else if (current < 0.8f) next = 1.0f;
                    else next = 0.0f;
                    m_processor.m_patternGrid[track][step] = next;
                }
            }
            break;
        }
        default: break;
    }
}

void LolooperAudioProcessorEditor::mouseDrag(const juce::MouseEvent& e)
{
    if (m_dragArea == kBpmLabel) {
        // Drag up = increase BPM (1 BPM per 2 pixels)
        float deltaY = m_dragStart.y - e.y;
        float newBpm = m_dragStartValue + deltaY * 0.5f;
        newBpm = juce::jlimit(20.0f, 300.0f, newBpm);
        *m_processor.m_bpmParam = newBpm;
        m_processor.m_sequencer.setBpm((double)newBpm);
    }
    else if (m_dragArea == kTrackVolume && m_dragTrack >= 0) {
        float frac = (float)(e.x - m_volRects[m_dragTrack].getX()) / m_volRects[m_dragTrack].getWidth();
        *m_processor.m_trackVolume[m_dragTrack] = juce::jlimit(0.0f, 1.0f, frac);
    } else if (m_dragArea == kTrackPan && m_dragTrack >= 0) {
        float frac = (float)(e.x - m_panRects[m_dragTrack].getX()) / m_panRects[m_dragTrack].getWidth();
        *m_processor.m_trackPan[m_dragTrack] = juce::jlimit(0.0f, 1.0f, frac);
    }
}

void LolooperAudioProcessorEditor::mouseUp(const juce::MouseEvent&) { m_dragArea = kNone; m_dragTrack = -1; }
