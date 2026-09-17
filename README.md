# ESP32-S3 Smart Battery Management System (BMS)

An edge-intelligent, fault-tolerant Battery Management System designed for multi-cell Lithium battery packs. Featuring per-cell active MOSFET bypass, sub-millisecond SCR pyro-fuse protection, dynamic thermal velocity analysis, TinyML structural anomaly detection, and a real-time ASCII Terminal User Interface (TUI).

---

## Key Capabilities

1. **Precision Data Acquisition**: Multi-point cell voltage ADC (ADS1115 / AFE), bi-directional Hall-effect current sensing (ACS724), multi-point thermistor array with thermal rise velocity ($dT/dt$), and FSR 402 casing swelling detection.
2. **Layered Safety Mechanisms**:
   - **Active MOSFET Bypass**: Dynamically isolates a single faulted cell while the remaining series pack continues to power loads uninterrupted.
   - **Emergency Pyro-Fuse**: Sub-millisecond capacitive discharge firing via BT151 SCR for catastrophic short-circuits (> 50A) or thermal runaway ($dT/dt > 2.0^\circ\text{C/s}$).
   - **Multi-Level Thresholds**: Overvoltage Protection (OVP > 4.25V), Undervoltage Protection (UVP < 2.80V), Sustained Overcurrent (OCP > 15A for 2s), Overtemperature (OTC > 55°C), and sensor heartbeat interlocks.
3. **Edge Intelligence**:
   - Coulomb counting with Open Circuit Voltage (OCV) table correction.
   - Dynamic Internal Resistance ($R = \Delta V / \Delta I$) State of Health (SOH) tracking.
   - TinyML vector anomaly detection for micro-shorts and structural degradation.
4. **Live TUI & Status Feedback**:
   - ANSI-colored live terminal dashboard with cell voltage graphs, power gauges, and event log.
   - Addressable WS2812B per-cell RGB LEDs with breathing charging animations and warning strobes.

---

## Project Structure

```
.
├── .github/                 # CI/CD workflows, PR & issue templates, CODEOWNERS
├── docs/                    # Architecture, hardware pinout, BOM, safety notes, algorithms
├── include/                 # Header files (config, drivers, core, algorithms, utils)
├── src/                     # Source implementations and main entry point
├── test/                    # Native unit tests, integration tests, and mocks
├── tools/                   # Python telemetry export & sensor calibration scripts
├── data/                    # OCV lookup tables and simulated fault vectors
└── platformio.ini           # PlatformIO dual-target configuration
```

---

## Hardware Specifications & Key Components

| Component | Part Number / Model | Description |
| :--- | :--- | :--- |
| **Microcontroller & ML Unit** | ESP32-S3-WROOM-1-N16R8 | Dual-core 240MHz, 16MB Flash, 8MB PSRAM |
| **Cell Voltage ADC** | ADS1115 / LTC6811 | 16-Bit High-Precision I2C ADC |
| **Current Sensor** | ACS724LLCTR-050B-T | ±50A Hall-Effect Bi-directional Sensor |
| **Temperature Array** | NTC 10K 3950 (1%) | Per-cell thermistor array + ambient reference |
| **Swelling Sensors** | Interlink FSR 402 | Force Sensitive Resistors for mechanical expansion |
| **Bypass MOSFETs** | IRF4905 & IRLB8721 | Single-cell isolation and bypass switch pairs |
| **Emergency Pyro-Fuse** | Littelfuse Pyro-Fuse | Sub-ms Busbar Severing Pyrotechnic Fuse |
| **Pyro SCR Driver** | BT151-500R + 4700µF Cap | Capacitive discharge firing circuit |
| **Status LEDs** | WS2812B Addressable RGB | Per-cell color, dimming, and strobe telemetry |

---

## Getting Started

### Prerequisites
- [VS Code](https://code.visualstudio.com/) + [PlatformIO IDE Extension](https://platformio.org/)
- Python 3.10+
- Clang-format (for formatting checks)

### Running Host Native Unit Tests (No Hardware Required)
```bash
pio test -e native --verbose
```

### Compiling Firmware for ESP32-S3
```bash
pio run -e esp32-s3-devkitc-1
```

### Flashing to ESP32-S3 Hardware
```bash
pio run -e esp32-s3-devkitc-1 -t upload
pio device monitor -b 115200
```

---

## 3-Person Team Module Ownership

- **Engineer 1 (Hardware & Drivers)**: Voltage ADC, Current, Thermistor, Swelling, MOSFETs, Pyro-Fuse SCR, Contactor, LEDs, SPI Flash.
- **Engineer 2 (Core Safety & Algorithms)**: Battery Pack Model, State Machine, Protection Logic, Passive Balancing, SOC / SOH / SOP, TinyML Detector.
- **Engineer 3 (TUI, Tools, CI/CD & Tests)**: ANSI TUI Dashboard, Serial CLI Parser, Python Telemetry, Unit/Integration Test Mocks.

---

## Documentation Links

- [System Architecture & State Machine](docs/architecture.md)
- [Hardware Pinout Mapping](docs/hardware/pinout.md)
- [Bill of Materials (BOM)](docs/hardware/bom.md)
- [Safety & Operating Protocols](docs/hardware/safety_notes.md)
- [SOC & SOH Mathematical Models](docs/algorithms/soc_soh.md)
- [Fault Logic & Mitigation Matrix](docs/algorithms/fault_logic.md)
- [Test Strategy & Test Plan](docs/testing/test_strategy.md)
- [User Manual & CLI Guide](docs/user_manual.md)
