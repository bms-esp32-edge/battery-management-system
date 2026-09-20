#!/usr/bin/env bash
# Execute native test suite via PlatformIO
set -e

echo "[BMS Test] Running host-native unit and integration tests..."
pio test -e native --verbose
echo "[BMS Test] All tests passed!"
