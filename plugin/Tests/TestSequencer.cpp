/*
 * Copyright (c) 2025 Fernando Passos. All rights reserved.
 * Licensed under the MIT License.
 *
 * Lolooper — Unit tests for the Sequencer class
 */

#include "../Source/Sequencer.h"
#include <cassert>
#include <cstdio>
#include <cmath>

//==============================================================================
// Helper: check if two doubles are approximately equal
static bool approx(double a, double b, double epsilon = 1e-6)
{
    return std::fabs(a - b) < epsilon;
}

//==============================================================================
static void test_prepare_sets_samples_per_step()
{
    Sequencer seq;
    seq.prepare(44100.0, 120.0);
    double expected = (60.0 / 120.0 / 4.0) * 44100.0; // 16th note at 120 BPM, 44100 Hz
    assert(approx(seq.getSamplesPerStep(), expected));
    std::printf("  PASS: test_prepare_sets_samples_per_step\n");
}

static void test_prepare_different_bpm()
{
    Sequencer seq;
    seq.prepare(44100.0, 60.0);
    // At 60 BPM, 1 beat = 44100 samples, 1/16th = 44100/4 = 11025
    double expected = 11025.0;
    assert(approx(seq.getSamplesPerStep(), expected));
    std::printf("  PASS: test_prepare_different_bpm\n");
}

static void test_prepare_different_sample_rate()
{
    Sequencer seq;
    seq.prepare(48000.0, 120.0);
    double expected = (60.0 / 120.0 / 4.0) * 48000.0;
    assert(approx(seq.getSamplesPerStep(), expected));
    std::printf("  PASS: test_prepare_different_sample_rate\n");
}

//==============================================================================
static void test_advance_exact_one_step()
{
    Sequencer seq;
    seq.prepare(44100.0, 120.0);
    seq.play();

    double samplesPerStep = seq.getSamplesPerStep();
    // Advance enough samples to guarantee crossing the step boundary
    // (ceil because sps can be fractional)
    int blockSize = (int)std::ceil(samplesPerStep);

    int elapsed = seq.advance(blockSize);
    assert(elapsed == 1);
    assert(seq.getCurrentStep() == 1);
    assert(seq.getCurrentBeat() == 0);
    assert(seq.getCurrentBar() == 0);
    std::printf("  PASS: test_advance_exact_one_step\n");
}

static void test_advance_does_nothing_when_stopped()
{
    Sequencer seq;
    seq.prepare(44100.0, 120.0);
    // Not playing

    int elapsed = seq.advance(44100);
    assert(elapsed == 0);
    assert(seq.getCurrentStep() == 0);
    std::printf("  PASS: test_advance_does_nothing_when_stopped\n");
}

static void test_advance_wraps_to_bar_after_64_steps()
{
    Sequencer seq;
    seq.prepare(48000.0, 120.0);  // exactly 6000 sps
    seq.play();

    // 64 step changes = 4 bars (beat wraps 4 times, bar increments once)
    // 64 * 6000 = 384000 samples
    seq.advance(384000);

    assert(seq.getCurrentStep() == 0);
    assert(seq.getCurrentBeat() == 0);
    assert(seq.getCurrentBar() == 1);
    std::printf("  PASS: test_advance_wraps_to_bar_after_64_steps\n");
}

static void test_advance_beat_increments_on_step_wrap()
{
    Sequencer seq;
    seq.prepare(48000.0, 120.0);
    seq.play();

    // Advance 4 steps (one 16th-note cycle)
    seq.advance(24000);  // 4 steps: 0→1→2→3→4
    assert(seq.getCurrentStep() == 4);
    assert(seq.getCurrentBeat() == 0);  // beat hasn't wrapped from 15→0 yet
    assert(seq.getCurrentBar() == 0);

    // Advance 12 more steps (16 total) — wrap to step 0, beat 0→1
    seq.advance(72000);
    assert(seq.getCurrentStep() == 0);
    assert(seq.getCurrentBeat() == 1);
    assert(seq.getCurrentBar() == 0);
    std::printf("  PASS: test_advance_beat_increments_on_step_wrap\n");
}

static void test_advance_multiple_bars()
{
    Sequencer seq;
    seq.prepare(48000.0, 120.0);
    seq.play();

    // Advance 128 steps = 8 bars (8 beats = 2 bars, 4 wraps per bar)
    seq.advance(128 * 6000);

    assert(seq.getCurrentStep() == 0);
    assert(seq.getCurrentBeat() == 0);
    assert(seq.getCurrentBar() == 2);
    std::printf("  PASS: test_advance_multiple_bars\n");
}

static void test_advance_partial_step()
{
    Sequencer seq;
    seq.prepare(44100.0, 120.0);
    seq.play();

    double sps = seq.getSamplesPerStep();
    // Advance half a step
    int elapsed = seq.advance((int)(sps * 0.5));
    assert(elapsed == 0);
    assert(seq.getCurrentStep() == 0);
    std::printf("  PASS: test_advance_partial_step\n");
}

static void test_advance_catch_up_partial_steps()
{
    Sequencer seq;
    seq.prepare(44100.0, 120.0);
    seq.play();

    double sps = seq.getSamplesPerStep();
    // Advance 0.5 + 0.6 = 1.1 steps
    seq.advance((int)(sps * 0.5));
    assert(seq.getCurrentStep() == 0);
    int elapsed = seq.advance((int)(sps * 0.6));
    assert(elapsed == 1);
    assert(seq.getCurrentStep() == 1);
    std::printf("  PASS: test_advance_catch_up_partial_steps\n");
}

//==============================================================================
static void test_transport_stop_resets_position()
{
    Sequencer seq;
    seq.prepare(44100.0, 120.0);
    seq.play();
    seq.advance(44100);

    seq.stop();
    assert(!seq.isPlaying());
    assert(seq.getCurrentStep() == 0);
    assert(seq.getCurrentBeat() == 0);
    assert(seq.getCurrentBar() == 0);
    std::printf("  PASS: test_transport_stop_resets_position\n");
}

static void test_transport_pause_preserves_position()
{
    Sequencer seq;
    seq.prepare(48000.0, 120.0);  // integer sps: 6000
    seq.play();

    double sps = seq.getSamplesPerStep();
    seq.advance((int)(sps * 5));  // step 5

    seq.pause();
    assert(!seq.isPlaying());
    assert(seq.getCurrentStep() == 5);

    // Resume
    seq.play();
    assert(seq.isPlaying());
    seq.advance((int)(sps * 3));
    assert(seq.getCurrentStep() == 8);
    std::printf("  PASS: test_transport_pause_preserves_position\n");
}

//==============================================================================
static void test_bpm_clamping()
{
    Sequencer seq;
    seq.setBpm(10.0);   // below min
    assert(seq.getBpm() == 20.0);
    seq.setBpm(500.0);  // above max
    assert(seq.getBpm() == 300.0);
    seq.setBpm(120.0);  // valid
    assert(seq.getBpm() == 120.0);
    std::printf("  PASS: test_bpm_clamping\n");
}

static void test_set_bpm_updates_samples_per_step()
{
    Sequencer seq;
    seq.prepare(44100.0, 120.0);
    double old = seq.getSamplesPerStep();

    seq.setBpm(60.0);
    assert(seq.getBpm() == 60.0);
    assert(seq.getSamplesPerStep() == old * 2.0);  // half BPM = double duration
    std::printf("  PASS: test_set_bpm_updates_samples_per_step\n");
}

//==============================================================================
static void test_swing_delays_offbeats()
{
    Sequencer seq;
    seq.prepare(48000.0, 120.0);  // exactly 6000 sps
    seq.setSwing(0.5);
    seq.play();

    // Advance one on-beat step (step 0)
    double sps = seq.getSamplesPerStep();
    int elapsed0 = seq.advance((int)sps);
    assert(elapsed0 == 1);
    assert(seq.getCurrentStep() == 1);

    // Now advancing the off-beat (step 1) should take longer due to swing
    // With swing=0.5, stepDuration = sps + 0.5*sps = 9000 samples
    // So advancing 6000 samples should produce 0 steps (not enough)
    int elapsed1 = seq.advance((int)sps);
    assert(elapsed1 == 0);  // not enough samples to cross the extended step
    assert(seq.getCurrentStep() == 1);  // still on step 1

    // Advance the remaining 3000 samples
    int elapsed2 = seq.advance(3001);
    assert(elapsed2 == 1);
    assert(seq.getCurrentStep() == 2);
    std::printf("  PASS: test_swing_delays_offbeats\n");
}

static void test_swing_zero_does_not_affect()
{
    Sequencer seq;
    seq.prepare(44100.0, 120.0);
    seq.setSwing(0.0);
    seq.play();

    double sps = seq.getSamplesPerStep();
    // Advance 2 steps with no swing
    int elapsed = seq.advance((int)(sps * 2));
    assert(elapsed == 2);
    assert(seq.getCurrentStep() == 2);
    std::printf("  PASS: test_swing_zero_does_not_affect\n");
}

//==============================================================================
static void test_initial_state()
{
    Sequencer seq;
    assert(!seq.isPlaying());
    assert(seq.getCurrentStep() == 0);
    assert(seq.getCurrentBeat() == 0);
    assert(seq.getCurrentBar() == 0);
    assert(seq.getBpm() == 100.0);
    assert(seq.getSwing() == 0.0);
    assert(seq.getHumanize() == 0.0);
    std::printf("  PASS: test_initial_state\n");
}

//==============================================================================
int main()
{
    std::printf("\n=== Sequencer Unit Tests ===\n\n");

    test_initial_state();

    std::printf("\n--- prepare() ---\n");
    test_prepare_sets_samples_per_step();
    test_prepare_different_bpm();
    test_prepare_different_sample_rate();

    std::printf("\n--- advance() ---\n");
    test_advance_exact_one_step();
    test_advance_does_nothing_when_stopped();
    test_advance_wraps_to_bar_after_64_steps();
    test_advance_beat_increments_on_step_wrap();
    test_advance_multiple_bars();
    test_advance_partial_step();
    test_advance_catch_up_partial_steps();

    std::printf("\n--- Transport ---\n");
    test_transport_stop_resets_position();
    test_transport_pause_preserves_position();

    std::printf("\n--- BPM ---\n");
    test_bpm_clamping();
    test_set_bpm_updates_samples_per_step();

    std::printf("\n--- Swing ---\n");
    test_swing_delays_offbeats();
    test_swing_zero_does_not_affect();

    std::printf("\n=== All Sequencer tests PASSED ===\n\n");
    return 0;
}
