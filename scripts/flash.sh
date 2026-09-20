#!/usr/bin/env bash
# Flash firmware to ESP32-S3 target
set -e

echo "[BMS Build] Compiling and uploading firmware to ESP32-S3..."
pio run -e esp32-s3-devkitc-1 -t upload
echo "[BMS Build] Flash complete!"
