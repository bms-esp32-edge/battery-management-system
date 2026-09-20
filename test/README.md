# BMS Test Suite

This directory contains test suites for the ESP32-S3 Battery Management System, covering host-native simulation, unit tests, integration tests, hardware-in-the-loop (HIL) tests, and mock shims.

## Directory Layout

```
test/
├── README.md               # Test documentation and execution guides
├── unit/                   # Host-native unit tests (per module)
│   ├── drivers/            # Driver logic tests
│   ├── core/               # Core state machine, types, and protection tests
│   ├── algorithms/         # SoC, SoH, SoP, and TinyML anomaly tests
│   └── utils/              # Utility, filter, and logging tests
├── integration/            # Cross-module integration tests
│   ├── test_protection_chain.cpp
│   ├── test_full_state_machine.cpp
│   ├── test_logging_on_fault.cpp
│   └── test_led_status_sync.cpp
├── hardware/               # Real hardware / bench validation tests
│   ├── test_real_adc.cpp
│   ├── test_real_current.cpp
│   ├── test_real_leds.cpp
│   └── test_real_bypass.cpp
├── mocks/                  # Test framework shims and mock peripherals
│   ├── unity.h
│   └── mock_voltage_adc.hpp
└── helpers/                # Test assertion macros and data generators
    ├── assert_macros.hpp
    └── test_helpers.hpp
```

## Running Tests

### Running Native Host Unit Tests
```bash
pio test -e native --verbose
```

### Running Targeted Unit Test Suites
```bash
pio test -e native -f test_types
pio test -e native -f test_cell
```
