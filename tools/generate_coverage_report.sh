#!/usr/bin/env bash
# Generate gcov / lcov test coverage report
set -e

echo "=== Generating BMS Test Coverage Report ==="
mkdir -p coverage
lcov --capture --directory .pio/build/native --output-file coverage/coverage.info --ignore-errors gcov,source || echo "lcov generated with warnings"
genhtml coverage/coverage.info --output-directory coverage/html || echo "genhtml generated"
echo "Report generated at coverage/html/index.html"
