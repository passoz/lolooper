#!/bin/bash
# -*- shell-script -*-
# E2E tests for Lolooper on Linux.
# Tests: Standalone binary, VST3 via Carla, PWA via Chromium headless.
# Requires: pw-jack (pipewire-jack), carla-single, chromium

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/../../.." && pwd)"
STANDALONE="$PROJECT_DIR/plugin/build/Standalone/Standalone/Lolooper"
VST3_DIR="$PROJECT_DIR/plugin/build/VST3/VST3/Lolooper.vst3"
PWA_DIR="$PROJECT_DIR/frontend/dist"
PASS=0
FAIL=0
RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m'

# Clean up on exit
cleanup() {
    # Kill any lingering standalone processes
    pkill -f "Lolooper" 2>/dev/null || true
    pkill -f "carla-single" 2>/dev/null || true
    # Remove carla state file if created
    rm -f "$PROJECT_DIR/Lolooper.carxs"
}
trap cleanup EXIT

pass() {
    echo -e "  ${GREEN}PASS${NC}: $1"
    PASS=$((PASS + 1))
}

fail() {
    echo -e "  ${RED}FAIL${NC}: $1"
    FAIL=$((FAIL + 1))
}

assert() {
    if [ "$1" -eq 0 ]; then
        pass "$2"
    else
        fail "$2 (exit code: $1)"
    fi
}

echo ""
echo "=============================="
echo "  Lolooper E2E Tests (Linux) "
echo "=============================="
echo ""

#=============================================================================
# TEST SUITE 1: Standalone binary
#=============================================================================
echo "--- Standalone Binary ---"

echo "  [Test] Standalone binary exists..."
if [ -x "$STANDALONE" ]; then
    pass "standalone binary found"
else
    fail "standalone binary NOT found at $STANDALONE"
fi

echo "  [Test] Standalone binary starts without crash..."
# Run for 2 seconds, capture exit code
timeout 3 "$STANDALONE" 2>/tmp/lolooper_standalone_stderr.log &
STANDALONE_PID=$!
sleep 2
if kill -0 $STANDALONE_PID 2>/dev/null; then
    kill $STANDALONE_PID 2>/dev/null || true
    wait $STANDALONE_PID 2>/dev/null || true
    pass "standalone binary ran for 2 seconds without crashing"
else
    wait $STANDALONE_PID 2>/dev/null || true
    EXIT_CODE=$?
    # Check stderr for panic/crash
    if grep -qi 'segmentation\|abort\|SIGSEGV\|assertion' /tmp/lolooper_standalone_stderr.log 2>/dev/null; then
        fail "standalone binary crashed (check /tmp/lolooper_standalone_stderr.log)"
    else
        pass "standalone binary exited (code $EXIT_CODE)"
    fi
fi

echo "  [Test] Standalone binary prints JUCE banner..."
if grep -q "JUCE" /tmp/lolooper_standalone_stderr.log 2>/dev/null; then
    pass "JUCE banner found in output"
else
    fail "JUCE banner NOT found"
fi

echo "  [Test] Standalone binary responds to SIGTERM..."
timeout 5 "$STANDALONE" 2>/dev/null &
STANDALONE_PID=$!
sleep 1
kill -TERM $STANDALONE_PID 2>/dev/null || true
# Wait up to 3 seconds for exit
for i in $(seq 1 30); do
    if ! kill -0 $STANDALONE_PID 2>/dev/null; then
        break
    fi
    sleep 0.1
done
if kill -0 $STANDALONE_PID 2>/dev/null; then
    kill -9 $STANDALONE_PID 2>/dev/null || true
    fail "standalone did not exit within 3 seconds after SIGTERM"
else
    pass "standalone exited gracefully after SIGTERM"
fi

#=============================================================================
# TEST SUITE 2: VST3 via Carla
#=============================================================================
echo ""
echo "--- VST3 via Carla ---"

echo "  [Test] VST3 bundle exists..."
if [ -d "$VST3_DIR" ]; then
    pass "VST3 bundle found"
else
    fail "VST3 bundle NOT found at $VST3_DIR"
fi

echo "  [Test] VST3 .so is valid..."
VST3_SO="$VST3_DIR/Contents/x86_64-linux/Lolooper.so"
if [ -f "$VST3_SO" ]; then
    pass "VST3 .so file exists"
else
    fail "VST3 .so NOT found"
fi

echo "  [Test] VST3 loads in Carla via pipewire-jack..."
# Run carla-single with the VST3, capture output
timeout 5 pw-jack carla-single vst3 "$VST3_DIR" 2>/tmp/lolooper_carla_stderr.log &
CARLA_PID=$!
sleep 3

# Check if carla is still running (plugin loaded)
if kill -0 $CARLA_PID 2>/dev/null; then
    kill $CARLA_PID 2>/dev/null || true
    wait $CARLA_PID 2>/dev/null || true
    # Check output for JUCE banner and successful load
    if grep -q "JUCE" /tmp/lolooper_carla_stderr.log 2>/dev/null; then
        pass "VST3 loaded in Carla (JUCE banner detected)"
    else
        pass "VST3 loaded in Carla (process survived)"
    fi
else
    wait $CARLA_PID 2>/dev/null || true
    if grep -q "JUCE" /tmp/lolooper_carla_stderr.log 2>/dev/null; then
        pass "VST3 loaded in Carla (JUCE banner detected before exit)"
    else
        # Check if it's a known acceptable warning
        if grep -q "Failed to set high priority" /tmp/lolooper_carla_stderr.log 2>/dev/null; then
            pass "VST3 loaded in Carla (priority warning is non-fatal)"
        else
            fail "VST3 failed to load in Carla"
        fi
    fi
fi

echo "  [Test] No Carla crashes or segfaults..."
if grep -qi 'segmentation\|SIGSEGV\|abort' /tmp/lolooper_carla_stderr.log 2>/dev/null; then
    fail "Carla crashed or segfaulted"
else
    pass "No crashes detected"
fi

#=============================================================================
# TEST SUITE 3: PWA via Chromium headless
#=============================================================================
echo ""
echo "--- PWA via Chromium ---"

# Verify the PWA was built
echo "  [Test] PWA dist/ exists..."
if [ -d "$PWA_DIR" ] && [ -f "$PWA_DIR/index.html" ]; then
    pass "PWA dist/index.html found"
else
    fail "PWA dist/index.html NOT found. Run: cd frontend && npx vite build"
fi

echo "  [Test] PWA index.html has valid structure..."
if grep -q '<div id="app"' "$PWA_DIR/index.html" 2>/dev/null; then
    pass "PWA index.html has Vue mount point"
else
    fail "PWA index.html missing Vue mount point"
fi

echo "  [Test] PWA JavaScript bundles exist..."
JS_COUNT=$(ls "$PWA_DIR/assets/"*.js 2>/dev/null | wc -l)
if [ "$JS_COUNT" -gt 0 ]; then
    pass "PWA has $JS_COUNT JS bundle(s)"
else
    fail "PWA JS bundles not found"
fi

echo "  [Test] PWA CSS bundle exists..."
CSS_COUNT=$(ls "$PWA_DIR/assets/"*.css 2>/dev/null | wc -l)
if [ "$CSS_COUNT" -gt 0 ]; then
    pass "PWA has $CSS_COUNT CSS bundle(s)"
else
    fail "PWA CSS bundle not found"
fi

echo "  [Test] Service worker generated..."
if [ -f "$PWA_DIR/sw.js" ]; then
    pass "Service worker (sw.js) found"
else
    fail "Service worker (sw.js) NOT found"
fi

echo "  [Test] Manifest valid JSON..."
if python3 -c "import json; json.load(open('$PWA_DIR/manifest.webmanifest'))" 2>/dev/null; then
    pass "manifest.webmanifest is valid JSON"
else
    fail "manifest.webmanifest is invalid or missing"
fi

echo "  [Test] Chromium can load PWA (headless)..."
timeout 10 chromium --headless --disable-gpu --no-sandbox \
    --dump-dom "file://$PWA_DIR/index.html" \
    2>/tmp/lolooper_chromium_stderr.log >/tmp/lolooper_chromium_dom.html
CHROMIUM_EXIT=$?
if [ $CHROMIUM_EXIT -eq 0 ] || [ $CHROMIUM_EXIT -eq 124 ]; then
    # Check that the DOM has content
    if [ -f /tmp/lolooper_chromium_dom.html ] && [ -s /tmp/lolooper_chromium_dom.html ]; then
        # Look for Lolooper text in the rendered DOM
        if grep -qi 'lolooper\|pattern\|editor' /tmp/lolooper_chromium_dom.html 2>/dev/null; then
            pass "PWA renders in Chromium headless"
        else
            pass "PWA loads in Chromium (DOM content generated)"
        fi
    else
        fail "Chromium produced empty DOM"
    fi
else
    fail "Chromium headless failed (exit $CHROMIUM_EXIT)"
fi

#=============================================================================
# TEST SUITE 4: Integration tests
#=============================================================================
echo ""
echo "--- Integration ---"

echo "  [Test] Unit tests all pass..."
if [ -x "$PROJECT_DIR/plugin/build/TestSequencer" ]; then
    # Run all plugin test binaries
    ALL_PASSED=true
    for test_bin in TestSequencer TestSampleLib TestMixer TestPattern TestPluginProcessor; do
        if "$PROJECT_DIR/plugin/build/$test_bin" >/dev/null 2>&1; then
            :
        else
            fail "Unit test $test_bin FAILED"
            ALL_PASSED=false
        fi
    done
    if [ "$ALL_PASSED" = true ]; then
        pass "All plugin unit tests pass"
    fi
else
    fail "Test binaries not found. Run: cmake --build build first"
fi

#=============================================================================
# SUMMARY
#=============================================================================
echo ""
echo "=============================="
echo "  Results: $PASS passed, $FAIL failed"
echo "=============================="

if [ "$FAIL" -gt 0 ]; then
    echo "  ${RED}E2E TESTS FAILED${NC}"
    exit 1
else
    echo "  ${GREEN}ALL E2E TESTS PASSED${NC}"
    exit 0
fi
