/*
 * Copyright (c) 2025 Fernando Passos. All rights reserved.
 * Licensed under the MIT License.
 *
 * Lolooper — Live performance looper for Brazilian music
 */

#include "Pattern.h"
#include <cmath>

//==============================================================================
// Pattern
//==============================================================================

Pattern::Pattern(const float grid[kNumTracks][kNumSteps])
{
    for (int t = 0; t < kNumTracks; ++t)
        for (int s = 0; s < kNumSteps; ++s)
            m_grid[t][s] = grid[t][s];
}

Pattern::Pattern(const Pattern& other)
{
    for (int t = 0; t < kNumTracks; ++t)
        for (int s = 0; s < kNumSteps; ++s)
            m_grid[t][s] = other.m_grid[t][s];
}

Pattern& Pattern::operator=(const Pattern& other)
{
    if (this != &other)
    {
        for (int t = 0; t < kNumTracks; ++t)
            for (int s = 0; s < kNumSteps; ++s)
                m_grid[t][s] = other.m_grid[t][s];
    }
    return *this;
}

//==============================================================================
float Pattern::getVelocity(int track, int step) const noexcept
{
    if (track < 0 || track >= kNumTracks) return 0.0f;
    if (step < 0 || step >= kNumSteps) return 0.0f;
    return m_grid[track][step];
}

void Pattern::setVelocity(int track, int step, float velocity) noexcept
{
    if (track < 0 || track >= kNumTracks) return;
    if (step < 0 || step >= kNumSteps) return;
    m_grid[track][step] = juce::jlimit(0.0f, 1.0f, velocity);
}

void Pattern::clear() noexcept
{
    for (int t = 0; t < kNumTracks; ++t)
        for (int s = 0; s < kNumSteps; ++s)
            m_grid[t][s] = 0.0f;
}

void Pattern::setGrid(const float grid[kNumTracks][kNumSteps])
{
    for (int t = 0; t < kNumTracks; ++t)
        for (int s = 0; s < kNumSteps; ++s)
            m_grid[t][s] = grid[t][s];
}

//==============================================================================
juce::String Pattern::toJsonString() const
{
    auto obj = new juce::DynamicObject();

    static const char* trackNames[] = {
        "surdo_1", "surdo_2", "surdo_3", "caixa", "repique",
        "tamborim", "pandeiro", "cuica", "agogo", "reco_reco",
        "tantan", "cavaquinho", "violao_7", "banjo"
    };

    for (int t = 0; t < kNumTracks; ++t)
    {
        juce::Array<juce::var> trackArr;
        for (int s = 0; s < kNumSteps; ++s)
            trackArr.add((double)m_grid[t][s]);
        obj->setProperty(juce::Identifier(trackNames[t]), trackArr);
    }

    return juce::JSON::toString(juce::var(obj));
}

//==============================================================================
Pattern Pattern::fromJson(const juce::String& jsonStr)
{
    Pattern pattern;
    auto var = juce::JSON::parse(jsonStr);

    if (auto* obj = var.getDynamicObject())
    {
        auto& props = obj->getProperties();

        static const char* trackNames[] = {
            "surdo_1", "surdo_2", "surdo_3", "caixa", "repique",
            "tamborim", "pandeiro", "cuica", "agogo", "reco_reco",
            "tantan", "cavaquinho", "violao_7", "banjo"
        };

        for (int t = 0; t < kNumTracks; ++t)
        {
            auto propId = juce::Identifier(trackNames[t]);
            if (props.contains(propId))
            {
                auto trackVar = props[propId];
                if (auto* arr = trackVar.getArray())
                {
                    for (int s = 0; s < juce::jmin(kNumSteps, arr->size()); ++s)
                        pattern.m_grid[t][s] = (float)(double)(*arr)[s];
                }
            }
        }
    }

    return pattern;
}

//==============================================================================
juce::String Pattern::trackToJsonString(int track) const
{
    juce::Array<juce::var> arr;
    for (int s = 0; s < kNumSteps; ++s)
        arr.add((double)m_grid[track][s]);
    return juce::JSON::toString(juce::var(arr));
}

//==============================================================================
// PatternLibrary
//==============================================================================

const juce::StringArray PatternLibrary::s_trackOrder = {
    "surdo_1", "surdo_2", "surdo_3", "caixa", "repique",
    "tamborim", "pandeiro", "cuica", "agogo", "reco_reco",
    "tantan", "cavaquinho", "violao_7", "banjo"
};

//==============================================================================
void PatternLibrary::loadFromDirectory(const juce::File& directory)
{
    if (!directory.isDirectory())
        return;

    juce::StringArray patternFiles = {
        "samba.json", "pagode.json", "partido_alto.json",
        "samba_reggae.json", "ijexa.json", "frevo.json",
        "maracatu.json", "intro.json", "virada.json"
    };

    for (auto& fileName : patternFiles)
    {
        auto file = directory.getChildFile(fileName);
        if (file.existsAsFile())
        {
            auto jsonStr = file.loadFileAsString();
            auto var = juce::JSON::parse(jsonStr);

            if (auto* obj = var.getDynamicObject())
            {
                auto& outerProps = obj->getProperties();

                // The JSON has a single key: style name → { track: [...] }
                for (auto& prop : outerProps)
                {
                    if (auto* gridObj = prop.value.getDynamicObject())
                    {
                        juce::String styleName = prop.name.toString();
                        auto patternJson = juce::JSON::toString(prop.value);
                        auto pattern = Pattern::fromJson(patternJson);

                        auto entry = new PatternEntry();
                        entry->name = styleName;
                        entry->pattern = pattern;
                        m_patterns.add(entry);
                    }
                }
            }
        }
    }

    // Set active pattern if we loaded any
    if (m_patterns.size() > 0)
    {
        m_activeStyleIndex = 0;
        m_activePattern = &m_patterns[0]->pattern;
    }
}

//==============================================================================
void PatternLibrary::loadFromResource()
{
    // Try data/patterns relative to current directory
    auto patternsDir = juce::File::getCurrentWorkingDirectory()
        .getChildFile("data").getChildFile("patterns");
    loadFromDirectory(patternsDir);
}

//==============================================================================
const Pattern* PatternLibrary::getPattern(const juce::String& styleName) const
{
    for (auto* entry : m_patterns)
        if (entry->name == styleName)
            return &entry->pattern;
    return nullptr;
}

Pattern* PatternLibrary::getPattern(const juce::String& styleName)
{
    for (auto* entry : m_patterns)
        if (entry->name == styleName)
            return &entry->pattern;
    return nullptr;
}

//==============================================================================
juce::StringArray PatternLibrary::getStyleNames() const
{
    juce::StringArray names;
    for (auto* entry : m_patterns)
        names.add(entry->name);
    return names;
}

//==============================================================================
void PatternLibrary::setActiveStyleIndex(int index)
{
    if (index >= 0 && index < m_patterns.size())
    {
        m_activeStyleIndex = index;
        m_activePattern = &m_patterns[index]->pattern;
    }
}

//==============================================================================
juce::String PatternLibrary::getActiveStyleName() const
{
    if (m_activeStyleIndex >= 0 && m_activeStyleIndex < m_patterns.size())
        return m_patterns[m_activeStyleIndex]->name;
    return {};
}

//==============================================================================
void PatternLibrary::setPattern(const juce::String& styleName, const Pattern& pattern)
{
    for (auto* entry : m_patterns)
    {
        if (entry->name == styleName)
        {
            entry->pattern = pattern;
            if (m_activePattern == &entry->pattern)
                m_activePattern = &entry->pattern;
            return;
        }
    }

    auto entry = new PatternEntry();
    entry->name = styleName;
    entry->pattern = pattern;
    m_patterns.add(entry);

    if (m_patterns.size() == 1)
    {
        m_activeStyleIndex = 0;
        m_activePattern = &m_patterns[0]->pattern;
    }
}

//==============================================================================
void PatternLibrary::removePattern(const juce::String& styleName)
{
    for (int i = 0; i < m_patterns.size(); ++i)
    {
        if (m_patterns[i]->name == styleName)
        {
            m_patterns.remove(i);
            if (m_activeStyleIndex >= m_patterns.size())
                m_activeStyleIndex = m_patterns.size() - 1;
            if (m_activeStyleIndex >= 0)
                m_activePattern = &m_patterns[m_activeStyleIndex]->pattern;
            else
                m_activePattern = nullptr;
            return;
        }
    }
}

//==============================================================================
juce::String PatternLibrary::exportAllToJson() const
{
    auto obj = new juce::DynamicObject();

    for (auto* entry : m_patterns)
    {
        auto patternJson = entry->pattern.toJsonString();
        auto patternVar = juce::JSON::parse(patternJson);
        obj->setProperty(juce::Identifier(entry->name), patternVar);
    }

    return juce::JSON::toString(juce::var(obj));
}

//==============================================================================
void PatternLibrary::importFromJson(const juce::String& jsonStr)
{
    auto var = juce::JSON::parse(jsonStr);

    if (auto* obj = var.getDynamicObject())
    {
        auto& props = obj->getProperties();

        for (auto& prop : props)
        {
            auto styleName = prop.name.toString();
            auto patternJson = juce::JSON::toString(prop.value);
            auto pattern = Pattern::fromJson(patternJson);
            setPattern(styleName, pattern);
        }
    }
}
