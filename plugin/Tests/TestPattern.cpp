/*
 * Copyright (c) 2025 Fernando Passos. All rights reserved.
 * Licensed under the MIT License.
 *
 * Lolooper — Unit tests for the Pattern and PatternLibrary classes
 */

#include "../Source/Pattern.h"
#include <cassert>
#include <cstdio>
#include <cmath>

//==============================================================================
static void test_pattern_initial_state()
{
    Pattern p;
    for (int t = 0; t < 14; ++t)
        for (int s = 0; s < 16; ++s)
            assert(p.getVelocity(t, s) == 0.0f);
    std::printf("  PASS: test_pattern_initial_state\n");
}

static void test_pattern_set_get_velocity()
{
    Pattern p;
    p.setVelocity(0, 0, 0.7f);
    assert(std::fabs(p.getVelocity(0, 0) - 0.7f) < 0.001f);
    p.setVelocity(13, 15, 1.0f);
    assert(std::fabs(p.getVelocity(13, 15) - 1.0f) < 0.001f);
    std::printf("  PASS: test_pattern_set_get_velocity\n");
}

static void test_pattern_bounds_checking()
{
    Pattern p;
    p.setVelocity(-1, 0, 1.0f);  // should be no-op
    p.setVelocity(14, 0, 1.0f);
    p.setVelocity(0, -1, 1.0f);
    p.setVelocity(0, 16, 1.0f);
    assert(p.getVelocity(-1, 0) == 0.0f);
    assert(p.getVelocity(0, 0) == 0.0f);  // all should stay 0
    std::printf("  PASS: test_pattern_bounds_checking\n");
}

static void test_pattern_clamp_velocity()
{
    Pattern p;
    p.setVelocity(0, 0, 2.5f);  // clamped to 1.0
    assert(p.getVelocity(0, 0) == 1.0f);
    p.setVelocity(0, 1, -1.0f); // clamped to 0.0
    assert(p.getVelocity(0, 1) == 0.0f);
    std::printf("  PASS: test_pattern_clamp_velocity\n");
}

static void test_pattern_clear()
{
    Pattern p;
    for (int t = 0; t < 14; ++t)
        for (int s = 0; s < 16; ++s)
            p.setVelocity(t, s, 0.5f);
    p.clear();
    for (int t = 0; t < 14; ++t)
        for (int s = 0; s < 16; ++s)
            assert(p.getVelocity(t, s) == 0.0f);
    std::printf("  PASS: test_pattern_clear\n");
}

static void test_pattern_copy()
{
    Pattern p1;
    p1.setVelocity(0, 0, 0.9f);

    Pattern p2(p1);
    assert(std::fabs(p2.getVelocity(0, 0) - 0.9f) < 0.001f);

    Pattern p3;
    p3 = p1;
    assert(std::fabs(p3.getVelocity(0, 0) - 0.9f) < 0.001f);

    // Modify original: copy should be independent
    p1.setVelocity(0, 0, 0.1f);
    assert(std::fabs(p2.getVelocity(0, 0) - 0.9f) < 0.001f);
    std::printf("  PASS: test_pattern_copy\n");
}

static void test_pattern_json_roundtrip()
{
    Pattern p1;
    p1.setVelocity(0, 0, 0.4f);
    p1.setVelocity(3, 7, 0.7f);
    p1.setVelocity(7, 15, 1.0f);

    auto json = p1.toJsonString();
    auto p2 = Pattern::fromJson(json);

    assert(std::fabs(p2.getVelocity(0, 0) - 0.4f) < 0.001f);
    assert(std::fabs(p2.getVelocity(3, 7) - 0.7f) < 0.001f);
    assert(std::fabs(p2.getVelocity(7, 15) - 1.0f) < 0.001f);
    assert(p2.getVelocity(0, 7) == 0.0f); // unset remains 0
    std::printf("  PASS: test_pattern_json_roundtrip\n");
}

static void test_pattern_from_json_with_subset()
{
    // Only specify 2 tracks — others default to 0
    auto json = juce::String(R"({
        "surdo_1": [1.0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
        "caixa":   [0,0,1.0,0,0,0,0,0,0,0,0,0,0,0,0,0]
    })");

    auto p = Pattern::fromJson(json);
    assert(std::fabs(p.getVelocity(0, 0) - 1.0f) < 0.001f);  // surdo_1 = track 0
    assert(p.getVelocity(0, 1) == 0.0f);
    assert(std::fabs(p.getVelocity(3, 2) - 1.0f) < 0.001f);  // caixa = track 3
    assert(p.getVelocity(1, 0) == 0.0f);  // surdo_2 not specified
    std::printf("  PASS: test_pattern_from_json_with_subset\n");
}

//==============================================================================
static void test_library_initial_state()
{
    PatternLibrary lib;
    assert(lib.size() == 0);
    assert(lib.getActivePattern() == nullptr);
    assert(lib.getActiveStyleName().isEmpty());
    std::printf("  PASS: test_library_initial_state\n");
}

static void test_library_set_and_get_pattern()
{
    PatternLibrary lib;
    Pattern p;
    p.setVelocity(10, 10, 1.0f);
    lib.setPattern("meu_samba", p);

    assert(lib.size() == 1);
    assert(lib.getActiveStyleName() == "meu_samba");
    const auto* retrieved = lib.getPattern("meu_samba");
    assert(retrieved != nullptr);
    assert(std::fabs(retrieved->getVelocity(10, 10) - 1.0f) < 0.001f);
    std::printf("  PASS: test_library_set_and_get_pattern\n");
}

static void test_library_get_style_names()
{
    PatternLibrary lib;
    lib.setPattern("samba", Pattern());
    lib.setPattern("frevo", Pattern());
    lib.setPattern("maracatu", Pattern());

    auto names = lib.getStyleNames();
    assert(names.size() == 3);
    assert(names[0] == "samba");
    assert(names[1] == "frevo");
    assert(names[2] == "maracatu");
    std::printf("  PASS: test_library_get_style_names\n");
}

static void test_library_remove_pattern()
{
    PatternLibrary lib;
    lib.setPattern("samba", Pattern());
    lib.setPattern("frevo", Pattern());
    lib.removePattern("samba");

    assert(lib.size() == 1);
    assert(lib.getActiveStyleName() == "frevo");
    assert(lib.getPattern("samba") == nullptr);
    std::printf("  PASS: test_library_remove_pattern\n");
}

static void test_library_set_active_style()
{
    PatternLibrary lib;
    lib.setPattern("samba", Pattern());
    lib.setPattern("frevo", Pattern());

    assert(lib.getActiveStyleName() == "samba");  // first added

    lib.setActiveStyleIndex(1);
    assert(lib.getActiveStyleName() == "frevo");

    // Modify active pattern
    lib.getActivePattern()->setVelocity(0, 0, 0.8f);
    assert(std::fabs(lib.getPattern("frevo")->getVelocity(0, 0) - 0.8f) < 0.001f);
    // Samba should be unchanged
    assert(lib.getPattern("samba")->getVelocity(0, 0) == 0.0f);
    std::printf("  PASS: test_library_set_active_style\n");
}

static void test_library_replace_pattern()
{
    PatternLibrary lib;
    Pattern p1;
    p1.setVelocity(0, 0, 0.5f);
    lib.setPattern("test", p1);

    Pattern p2;
    p2.setVelocity(8, 8, 1.0f);
    lib.setPattern("test", p2);

    assert(lib.size() == 1);  // not duplicated
    assert(lib.getPattern("test")->getVelocity(0, 0) == 0.0f);  // old value gone
    assert(std::fabs(lib.getPattern("test")->getVelocity(8, 8) - 1.0f) < 0.001f);
    std::printf("  PASS: test_library_replace_pattern\n");
}

static void test_library_export_import()
{
    PatternLibrary lib;
    Pattern p1;
    p1.setVelocity(5, 5, 0.9f);
    lib.setPattern("custom", p1);

    auto json = lib.exportAllToJson();

    PatternLibrary lib2;
    lib2.importFromJson(json);

    assert(lib2.size() == 1);
    assert(lib2.getActiveStyleName() == "custom");
    assert(std::fabs(lib2.getPattern("custom")->getVelocity(5, 5) - 0.9f) < 0.001f);
    std::printf("  PASS: test_library_export_import\n");
}

//==============================================================================
int main()
{
    std::printf("\n=== Pattern Unit Tests ===\n\n");

    std::printf("--- Pattern ---\n");
    test_pattern_initial_state();
    test_pattern_set_get_velocity();
    test_pattern_bounds_checking();
    test_pattern_clamp_velocity();
    test_pattern_clear();
    test_pattern_copy();
    test_pattern_json_roundtrip();
    test_pattern_from_json_with_subset();

    std::printf("\n--- PatternLibrary ---\n");
    test_library_initial_state();
    test_library_set_and_get_pattern();
    test_library_get_style_names();
    test_library_remove_pattern();
    test_library_set_active_style();
    test_library_replace_pattern();
    test_library_export_import();

    std::printf("\n=== All Pattern tests PASSED ===\n\n");
    return 0;
}
