#!/bin/bash
# Plugin Interaction Test Suite
PROJECT_DIR="$(cd "$(dirname "$0")/../../.." && pwd)"
VST3="$PROJECT_DIR/plugin/build/VST3/VST3/Lolooper.vst3"
STANDALONE="$PROJECT_DIR/plugin/build/Standalone/Standalone/Lolooper"

test_timeout() { timeout "$1" "$2" >/dev/null 2>&1; return $?; }

echo "=== Plugin Interaction Tests ==="

echo -n "  Standalone starts... "
test_timeout 3 "$STANDALONE"
rc=$?; [ $rc -eq 124 ] && echo "OK" || echo "FAIL (exit $rc)"

echo -n "  VST3 in Carla... "
test_timeout 8 pw-jack carla-single vst3 "$VST3"
rc=$?; [ $rc -eq 124 ] || [ $rc -eq 0 ] && echo "OK (exit=$rc)" || echo "FAIL (exit=$rc)"

echo -n "  Unit tests... "
all_pass=true
for t in TestSequencer TestSampleLib TestMixer TestPattern TestPluginProcessor TestInstrumentLib TestArpeggiator TestPitchDetector TestChordRecognizer; do
    if ! "$PROJECT_DIR/plugin/build/$t" >/dev/null 2>&1; then
        all_pass=false; echo "FAIL: $t"; break
    fi
done
$all_pass && echo "ALL PASS" || echo "SOME FAILED"
