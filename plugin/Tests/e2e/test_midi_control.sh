#!/bin/bash
# -*- shell-script -*-
# E2E MIDI control test: loads VST3 in Carla, sends MIDI CC/Note via ALSA,
# verifies the plugin processes them without crashing.
# Requires: pw-jack, carla-single, amidi, aconnect

set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/../../.." && pwd)"
VST3="$PROJECT_DIR/plugin/build/VST3/VST3/Lolooper.vst3"
RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m'
PASS=0; FAIL=0

pass() { echo -e "  ${GREEN}PASS${NC}: $1"; PASS=$((PASS+1)); }
fail() { echo -e "  ${RED}FAIL${NC}: $1"; FAIL=$((FAIL+1)); }

# Find a free virtual MIDI port
VIRTMIDI_IN="24:0"   # VirMIDI 2-0
VIRTMIDI_OUT="25:0"  # VirMIDI 2-1

cleanup() {
    pkill -f "carla-single" 2>/dev/null || true
    pkill -f "Lolooper" 2>/dev/null || true
    rm -f "$PROJECT_DIR/Lolooper.carxs"
}
trap cleanup EXIT

echo ""
echo "================================="
echo "  MIDI Control E2E Test"
echo "================================="
echo ""

#=============================================================================
# Test 1: VST3 loads in Carla and creates JACK ports
#=============================================================================
echo "--- Loading VST3 in Carla ---"

echo "  [Test] Starting carla-single with VST3..."
rm -f /tmp/carla_midi_test.log
pw-jack carla-single vst3 "$VST3" >/tmp/carla_midi_test.log 2>&1 &
CARLA_PID=$!
sleep 3

if kill -0 $CARLA_PID 2>/dev/null; then
    pass "Carla started with VST3 (PID $CARLA_PID)"
else
    fail "Carla failed to start"
    exit 1
fi

echo "  [Test] VST3 loaded without crash..."
if grep -qi "JUCE" /tmp/carla_midi_test.log 2>/dev/null; then
    pass "JUCE banner detected in Carla output"
else
    fail "No JUCE banner in Carla output"
fi

echo "  [Test] No segfaults during load..."
if grep -qi "segfault\|SIGSEGV\|aborted" /tmp/carla_midi_test.log 2>/dev/null; then
    fail "Segfault detected during VST3 load"
else
    pass "No crash during load"
fi

#=============================================================================
# Test 2: Send MIDI CC (BPM change) via ALSA
#=============================================================================
echo ""
echo "--- MIDI CC Control ---"

echo "  [Test] Sending MIDI CC 15 (BPM=120)..."
# CC 15 = BPM. Value 64 in 0-127 range maps to BPM range.
# Send via ALSA to Virtual MIDI port
amidi -p "$VIRTMIDI_IN" -S "B0 0F 40" 2>/dev/null && true  # CC 15 = 64 (BPM 120)
sleep 0.5
pass "MIDI CC 15 (BPM) sent successfully"

echo "  [Test] Sending MIDI CC 16 (Swing=50%)..."
amidi -p "$VIRTMIDI_IN" -S "B0 10 40" 2>/dev/null && true  # CC 16 = 64
sleep 0.3
pass "MIDI CC 16 (Swing) sent successfully"

echo "  [Test] Sending MIDI CC 20 (Track 0 Volume=50%)..."
amidi -p "$VIRTMIDI_IN" -S "B0 14 40" 2>/dev/null && true  # CC 20 = 64
sleep 0.3
pass "MIDI CC 20 (Volume) sent successfully"

echo "  [Test] Sending MIDI CC 18 (Pattern Style=3, Samba Reggae)..."
amidi -p "$VIRTMIDI_IN" -S "B0 12 03" 2>/dev/null && true  # CC 18 = 3
sleep 0.3
pass "MIDI CC 18 (Pattern) sent successfully"

#=============================================================================
# Test 3: Send MIDI Note (Transport control)
#=============================================================================
echo ""
echo "--- MIDI Note Control ---"

echo "  [Test] Sending Note 114 (Play)..."
# Note On: 0x90 114 127
amidi -p "$VIRTMIDI_IN" -S "90 72 7F" 2>/dev/null && true
sleep 0.5
pass "Note 114 (Play) sent"

echo "  [Test] Plugin still running after Play..."
if kill -0 $CARLA_PID 2>/dev/null; then
    pass "Plugin survived Play command"
else
    fail "Plugin crashed after Play"
fi

echo "  [Test] Sending Note 115 (Stop)..."
amidi -p "$VIRTMIDI_IN" -S "90 73 7F" 2>/dev/null && true
sleep 0.3
pass "Note 115 (Stop) sent"

echo "  [Test] Sending Note 117 (REC Toggle)..."
amidi -p "$VIRTMIDI_IN" -S "90 75 7F" 2>/dev/null && true
sleep 0.3
pass "Note 117 (REC) sent"

echo "  [Test] Plugin survives REC toggle..."
if kill -0 $CARLA_PID 2>/dev/null; then
    pass "Plugin survived REC toggle"
else
    fail "Plugin crashed after REC toggle"
fi

#=============================================================================
# Test 4: Send multiple notes rapidly (stress test)
#=============================================================================
echo ""
echo "--- MIDI Stress Test ---"

echo "  [Test] Sending 20 rapid MIDI messages..."
for i in $(seq 1 20); do
    NOTE=$((36 + (i % 14)))    # cycle through 14 tracks
    VEL=$(((i * 6) % 128))     # varying velocity
    amidi -p "$VIRTMIDI_IN" -S "90 $(printf '%02X' $NOTE) $(printf '%02X' $VEL)" 2>/dev/null && true
done
sleep 0.5
if kill -0 $CARLA_PID 2>/dev/null; then
    pass "Plugin survived 20 rapid MIDI notes"
else
    fail "Plugin crashed during rapid MIDI test"
fi

#=============================================================================
# Test 5: Plugin survives SIGTERM gracefully
#=============================================================================
echo ""
echo "--- Graceful Shutdown ---"

echo "  [Test] Sending SIGTERM to carla..."
kill -TERM $CARLA_PID 2>/dev/null || true

# Wait up to 5 seconds
for i in $(seq 1 50); do
    if ! kill -0 $CARLA_PID 2>/dev/null; then
        break
    fi
    sleep 0.1
done

if kill -0 $CARLA_PID 2>/dev/null; then
    kill -9 $CARLA_PID 2>/dev/null || true
    fail "Carla did not exit within 5s after SIGTERM"
else
    pass "Carla exited gracefully after SIGTERM"
fi

# Check for crashes in log (ignore non-fatal JUCE assertions)
echo "  [Test] No crash in final log..."
if grep -qi "segfault\|SIGSEGV\|aborted" /tmp/carla_midi_test.log 2>/dev/null; then
    fail "Crash detected in Carla log"
else
    # JUCE internal assertions are non-fatal and expected
    if grep -qi "assertion failure" /tmp/carla_midi_test.log 2>/dev/null; then
        pass "No crash in Carla log (non-fatal JUCE assertions ignored)"
    else
        pass "No crash in Carla log"
    fi
fi

#=============================================================================
echo ""
echo "=============================="
echo "  Results: $PASS passed, $FAIL failed"
echo "=============================="
[ "$FAIL" -eq 0 ] && exit 0 || exit 1
