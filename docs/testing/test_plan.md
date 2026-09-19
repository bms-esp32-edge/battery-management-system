# BMS Test Plan

## Subsystem Test Coverage

| Test Suite | Location | Verification Target |
| :--- | :--- | :--- |
| **Common Types & Enums** | `test/unit/core/test_types.cpp` | Memory layouts, bitwise operators, timed pyro key, stringifiers |
| **Cell Model** | `test/unit/core/test_cell.cpp` | Configuration limits, cell state transitions |
| **Voltage ADC Driver** | `test/unit/drivers/test_voltage_adc.cpp` | ADS1115 conversion formula, filtering |
| **Current Sensor** | `test/unit/drivers/test_current_sensor.cpp` | ACS724 zero-offset calibration & scaling |
| **Protection Engine** | `test/unit/core/test_protection.cpp` | OVP, UVP, OCP, Swelling, and Pyro trips |
| **Bypass Isolation** | `test/unit/core/test_balancing.cpp` | MOSFET dead-time & active cell re-indexing |
| **SOC Estimator** | `test/unit/algorithms/test_soc.cpp` | Coulomb counting precision & OCV convergence |
| **Full Protection Chain**| `test/integration/test_protection_chain.cpp` | End-to-end trip from raw ADC to relay trip |

