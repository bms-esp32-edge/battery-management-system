#!/usr/bin/env bash
# Open serial monitor for ESP32-S3
set -e

BAUD=${1:-115200}
echo "[BMS Monitor] Connecting to serial monitor at ${BAUD} baud..."
pio device monitor -b "${BAUD}"
