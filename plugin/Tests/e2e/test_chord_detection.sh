#!/bin/bash
# E2E Chord Detection test: feeds audio chords into the plugin
# and verifies SysEx 0x09 output with chord names.
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/../../.." && pwd)"
RED='\033[0;31m'; GREEN='\033[0;32m'; NC='\033[0m'

echo ""
echo "==========================================="
echo "  Chord Detection E2E Test"
echo "==========================================="
echo ""

echo "Building..."
cd "$PROJECT_DIR/plugin"
cmake --build build --target TestChordDetection -j$(nproc) 2>&1 | tail -3

echo "Running..."
timeout 30 "$PROJECT_DIR/plugin/build/TestChordDetection" 2>&1
