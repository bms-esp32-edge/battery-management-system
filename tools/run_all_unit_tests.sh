#!/usr/bin/env bash
# Script to run all unit tests natively
set -e

echo "=== Running BMS Unit Tests ==="
pio test -e native --verbose
