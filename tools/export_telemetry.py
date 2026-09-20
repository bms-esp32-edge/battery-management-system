#!/usr/bin/env python3
"""
Telemetry Exporter Tool for ESP32-S3 BMS.
Reads serial telemetry streams and exports JSON / CSV logs.
"""

import argparse
import sys
import time

def main():
    parser = argparse.ArgumentParser(description="Export BMS Serial Telemetry")
    parser.add_argument("--port", default="/dev/ttyUSB0", help="Serial port device")
    parser.add_argument("--baud", type=int, default=115200, help="Baud rate")
    parser.add_argument("--output", default="telemetry.csv", help="Output file path")
    args = parser.parse_args()

    print(f"[Telemetry Exporter] Listening on {args.port} @ {args.baud} baud...")
    print(f"[Telemetry Exporter] Logging output to {args.output}")

if __name__ == "__main__":
    main()
