/*
 * Copyright (c) 2025 Fernando Passos. All rights reserved.
 * Licensed under the MIT License.
 *
 * Lolooper — Live performance looper for Brazilian music
 */

#pragma once

#include <juce_core/juce_core.h>

//==============================================================================
/** Represents a rhythmic pattern: 14 tracks × 16 steps with velocity per cell.
 *
 *  Supports JSON serialization for communication with the PWA via SysEx.
 *  Manages multiple pattern "styles" (samba, pagode, frevo, etc.), each
 *  containing a full 14×16 grid.
 */
class Pattern
{
public:
    Pattern() = default;
    ~Pattern() = default;

    static constexpr int kNumTracks = 14;
    static constexpr int kNumSteps = 16;

    //==============================================================================
    /** Create a pattern from a 2D grid array. */
    Pattern(const float grid[kNumTracks][kNumSteps]);

    /** Copy from another pattern. */
    Pattern(const Pattern& other);
    Pattern& operator=(const Pattern& other);

    //==============================================================================
    /** Access grid values. */
    float getVelocity(int track, int step) const noexcept;
    void setVelocity(int track, int step, float velocity) noexcept;
    void clear() noexcept;

    /** Get/Set the entire grid. */
    const float (*getGrid() const)[kNumSteps] { return m_grid; }
    void setGrid(const float grid[kNumTracks][kNumSteps]);

    //==============================================================================
    /** JSON serialization. */
    juce::String toJsonString() const;
    static Pattern fromJson(const juce::String& jsonStr);

    /** Serialize just one track to a JSON array string. */
    juce::String trackToJsonString(int track) const;

private:
    //==============================================================================
    float m_grid[kNumTracks][kNumSteps] = {};
};

//==============================================================================
/** Manages a collection of named patterns (styles).
 *
 *  Provides loading from JSON files/metadata, SysEx serialization,
 *  and active pattern switching.
 */
class PatternLibrary
{
public:
    PatternLibrary() = default;
    ~PatternLibrary() = default;

    //==============================================================================
    /** Load all pattern styles from the given directory containing JSON files. */
    void loadFromDirectory(const juce::File& directory);

    /** Load patterns from embedded JSON resource. */
    void loadFromResource();

    //==============================================================================
    /** Get a pattern by style name. Returns nullptr if not found. */
    const Pattern* getPattern(const juce::String& styleName) const;
    Pattern* getPattern(const juce::String& styleName);

    /** Get all available style names. */
    juce::StringArray getStyleNames() const;

    /** Get the active pattern. */
    const Pattern* getActivePattern() const noexcept { return m_activePattern; }
    Pattern* getActivePattern() noexcept { return m_activePattern; }

    /** Get/set the active style index. */
    int getActiveStyleIndex() const noexcept { return m_activeStyleIndex; }
    void setActiveStyleIndex(int index);

    /** Get the active style name. */
    juce::String getActiveStyleName() const;

    //==============================================================================
    /** Add or replace a pattern. */
    void setPattern(const juce::String& styleName, const Pattern& pattern);

    /** Remove a pattern by name. */
    void removePattern(const juce::String& styleName);

    /** Number of loaded patterns. */
    int size() const noexcept { return m_patterns.size(); }

    //==============================================================================
    /** Export all patterns as a single JSON object. */
    juce::String exportAllToJson() const;

    /** Import patterns from a JSON object. */
    void importFromJson(const juce::String& jsonStr);

private:
    //==============================================================================
    struct PatternEntry
    {
        juce::String name;
        Pattern pattern;
    };

    juce::OwnedArray<PatternEntry> m_patterns;
    int m_activeStyleIndex = 0;
    Pattern* m_activePattern = nullptr;

    // Track order used in JSON files
    static const juce::StringArray s_trackOrder;
};
