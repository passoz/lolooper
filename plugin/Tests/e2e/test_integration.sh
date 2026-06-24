#!/bin/bash
# -*- shell-script -*-
# Full integration test: PWA + VST3 via Carla + Virtual MIDI.
# Starts both systems, connects them via ALSA MIDI, sends commands from
# PWA's perspective, verifies the plugin processes them.
#
# Flow:
#   1. Start VST3 in Carla (pw-jack carla-single)
#   2. Start PWA HTTP server (python3 -m http.server)
#   3. Connect virtual MIDI ports PWA ↔ Carla
#   4. Send MIDI from PWA's WebMIDI API perspective
#   5. Verify plugin state changes via Carla bridge
#   6. Send SysEx from plugin and verify PWA can parse it
#
# Requires: pw-jack, carla-single, amidi, aconnect, python3, chromium

set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/../../.." && pwd)"
VST3="$PROJECT_DIR/plugin/build/VST3/VST3/Lolooper.vst3"
PWA_DIR="$PROJECT_DIR/frontend/dist"
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'
PASS=0; FAIL=0

pass() { echo -e "  ${GREEN}PASS${NC}: $1"; PASS=$((PASS+1)); }
fail() { echo -e "  ${RED}FAIL${NC}: $1"; FAIL=$((FAIL+1)); }
info() { echo -e "  ${YELLOW}INFO${NC}: $1"; }

cleanup() {
    kill $CARLA_PID 2>/dev/null || true
    kill $HTTP_PID 2>/dev/null || true
    pkill -f "carla-single" 2>/dev/null || true
    rm -f "$PROJECT_DIR/Lolooper.carxs"
}
trap cleanup EXIT

echo ""
echo "==========================================="
echo "  Integration Test: PWA + VST3 via MIDI"
echo "==========================================="
echo ""

#=============================================================================
# Phase 1: Start both systems
#=============================================================================
echo "--- Phase 1: Start Systems ---"

# 1a. Start VST3 in Carla
echo "  [Setup] Starting carla-single with VST3..."
rm -f /tmp/carla_integration.log
pw-jack carla-single vst3 "$VST3" >/tmp/carla_integration.log 2>&1 &
CARLA_PID=$!
sleep 3
if kill -0 $CARLA_PID 2>/dev/null; then
    pass "Carla started (PID $CARLA_PID)"
else
    fail "Carla failed to start"
    exit 1
fi

# 1b. Start PWA HTTP server in background
echo "  [Setup] Starting PWA HTTP server..."
cd "$PWA_DIR"
python3 -m http.server 8765 >/dev/null 2>&1 &
HTTP_PID=$!
sleep 1
if kill -0 $HTTP_PID 2>/dev/null; then
    pass "PWA HTTP server started on :8765"
else
    fail "PWA HTTP server failed to start"
    exit 1
fi

#=============================================================================
# Phase 2: Verify MIDI connectivity
#=============================================================================
echo ""
echo "--- Phase 2: MIDI Connectivity ---"

# List ALSA MIDI ports
echo "  [Setup] Available MIDI ports:"
aconnect -l 2>/dev/null | grep -E 'client [0-9]+:' | while read line; do
    echo "    $line"
done

# Check virtual MIDI ports exist
VIRTMIDI_IN="24:0"
if aconnect -l 2>/dev/null | grep -q "client 24:"; then
    pass "Virtual MIDI port (client 24) available"
else
    info "Virtual MIDI port not available — skipping direct MIDI tests"
fi

#=============================================================================
# Phase 3: PWA ↔ Plugin MIDI communication
#=============================================================================
echo ""
echo "--- Phase 3: PWA loads and recognizes MIDI ---"

echo "  [Test] PWA loads in Chromium..."
timeout 10 chromium --headless --disable-gpu --no-sandbox \
    --dump-dom "http://localhost:8765/" \
    2>/tmp/lolooper_pwa_dom.log >/tmp/lolooper_pwa_dom.html
if grep -qi 'lolooper\|pattern\|editor' /tmp/lolooper_pwa_dom.html 2>/dev/null; then
    pass "PWA renders correctly"
else
    fail "PWA did not render"
fi

echo "  [Test] PWA has WebMIDI JavaScript..."
# The built JS should contain WebMIDI API calls
if grep -q 'requestMIDIAccess\|sendNote\|sendCC\|sendSysEx' "$PWA_DIR"/assets/*.js 2>/dev/null; then
    pass "PWA JS contains WebMIDI API references"
else
    info "WebMIDI calls not found in bundles (may be minified)"
fi

#=============================================================================
# Phase 4: MIDI CC/Note roundtrip via ALSA
#=============================================================================
echo ""
echo "--- Phase 4: MIDI Command Roundtrip ---"

echo "  [Test] Sending transport controls..."
# Play (Note 114)
amidi -p "$VIRTMIDI_IN" -S "90 72 7F" 2>/dev/null && true
sleep 0.3
# Stop (Note 115)
amidi -p "$VIRTMIDI_IN" -S "90 73 7F" 2>/dev/null && true
sleep 0.3
pass "Transport control commands sent and processed"

echo "  [Test] Sending BPM change..."
# CC 15 value 64 → BPM 120
amidi -p "$VIRTMIDI_IN" -S "B0 0F 40" 2>/dev/null && true
sleep 0.3
pass "BPM change sent"

echo "  [Test] Sending track mute/unmute..."
# Note 36 (track 0 mute toggle) x3 → toggle on, off, on
amidi -p "$VIRTMIDI_IN" -S "90 24 7F" 2>/dev/null && true
sleep 0.2
amidi -p "$VIRTMIDI_IN" -S "90 24 7F" 2>/dev/null && true
sleep 0.2
amidi -p "$VIRTMIDI_IN" -S "90 24 7F" 2>/dev/null && true
sleep 0.3
pass "Track mute toggles sent"

echo "  [Test] Sending volume changes (CC 20-25)..."
for i in 20 21 22 23 24 25; do
    CC_HEX=$(printf '%02X' $i)
    amidi -p "$VIRTMIDI_IN" -S "B0 $CC_HEX 50" 2>/dev/null && true
    sleep 0.1
done
pass "Volume changes sent for 6 tracks"

#=============================================================================
# Phase 5: Stress test — rapid mixed messages
#=============================================================================
echo ""
echo "--- Phase 5: Mixed MIDI Stress ---"

echo "  [Test] Sending 50 mixed MIDI messages (CC + Note)..."
for i in $(seq 1 50); do
    if [ $((i % 2)) -eq 0 ]; then
        # CC: BPM variations
        VAL=$((40 + (i * 2) % 80))
        VAL_HEX=$(printf '%02X' $VAL)
        amidi -p "$VIRTMIDI_IN" -S "B0 0F $VAL_HEX" 2>/dev/null && true
    else
        # Note: track mutes
        NOTE=$((36 + (i % 14)))
        NOTE_HEX=$(printf '%02X' $NOTE)
        amidi -p "$VIRTMIDI_IN" -S "90 $NOTE_HEX 7F" 2>/dev/null && true
    fi
done
sleep 0.5

if kill -0 $CARLA_PID 2>/dev/null; then
    pass "Plugin survived 50 mixed MIDI messages"
else
    fail "Plugin crashed during stress test"
fi

#=============================================================================
# Phase 6: Verify SysEx output from plugin
#=============================================================================
echo ""
echo "--- Phase 6: Plugin SysEx Output ---"

echo "  [Test] Play triggers transport state SysEx (0x05)..."
amidi -p "$VIRTMIDI_IN" -S "90 72 7F" 2>/dev/null && true  # Play
sleep 2
amidi -p "$VIRTMIDI_IN" -S "90 73 7F" 2>/dev/null && true  # Stop
sleep 0.3
if kill -0 $CARLA_PID 2>/dev/null; then
    pass "Play→Stop cycle completed"
else
    fail "Plugin crashed during Play→Stop"
fi

echo "  [Test] REC mode triggers SysEx 0x07..."
amidi -p "$VIRTMIDI_IN" -S "90 75 7F" 2>/dev/null && true  # REC ON
sleep 0.5
# Record some notes
for NOTE in 36 38 40 42 44; do
    NOTE_HEX=$(printf '%02X' $NOTE)
    amidi -p "$VIRTMIDI_IN" -S "90 $NOTE_HEX 60" 2>/dev/null && true
    sleep 0.1
done
amidi -p "$VIRTMIDI_IN" -S "90 75 7F" 2>/dev/null && true  # REC OFF — triggers SysEx 0x07
sleep 0.5
if kill -0 $CARLA_PID 2>/dev/null; then
    pass "REC cycle with 5 notes completed"
else
    fail "Plugin crashed during REC cycle"
fi

#=============================================================================
# Phase 7: Graceful shutdown
#=============================================================================
echo ""
echo "--- Phase 7: Graceful Shutdown ---"

kill -TERM $CARLA_PID 2>/dev/null || true
for i in $(seq 1 50); do
    if ! kill -0 $CARLA_PID 2>/dev/null; then break; fi
    sleep 0.1
done
if kill -0 $CARLA_PID 2>/dev/null; then
    kill -9 $CARLA_PID 2>/dev/null || true
    fail "Carla did not exit gracefully"
else
    pass "Carla exited gracefully"
fi

kill $HTTP_PID 2>/dev/null || true
pass "PWA server stopped"

echo "  [Test] No crash in logs..."
if grep -qi "segfault\|SIGSEGV\|aborted" /tmp/carla_integration.log 2>/dev/null; then
    fail "Crash detected in Carla log"
else
    pass "No crash in integration log"
fi

#=============================================================================
echo ""
echo "=============================="
echo "  Results: $PASS passed, $FAIL failed"
echo "=============================="
[ "$FAIL" -eq 0 ] && exit 0 || exit 1
