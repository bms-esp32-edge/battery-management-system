# PR #1: Common Types, Enums & Canonical Telemetry Definitions

- **Branch**: `vedant/common-types-enums`
- **Author**: Vedant Salunkhe ([vedaaasalunkhe@gmail.com](mailto:vedaaasalunkhe@gmail.com))
- **Status**: Completed & Verified
- **Target File**: `include/firmware/core/types.hpp`
- **Test File**: `test/unit/core/test_types.cpp`

---

## 1. Summary

This PR establishes the root, zero-dependency foundational types, scoped enums, bitmask classes, safety keys, canonical telemetry data structures, and stringification utilities for the ESP32-S3 Smart Battery Management System (BMS).

All types are designed to adhere to C++17 standards, MISRA C++:2008 oriented defensive programming (strongly typed scoped enums, fixed-width integers, zero dynamic memory allocation, no exceptions), and functional safety design patterns (two-stage guarded actuation, freshness tracking, data integrity).

---

## 2. Type of Change
- [x] New feature (Foundational type definitions)
- [x] Safety/protection logic change (Guarded pyro actuation key, fault bitmask)
- [x] Test-only change (100% covered unit test suite)
- [x] Docs (Architecture, fault matrix, and test plan updates)

---

## 3. Detailed Specifications

### 3.1 Scoped Enums
- **`BmsState : uint8_t`**:
  - `INIT = 0U`: Hardware self-test, POST, baseline sensor calibration.
  - `STANDBY = 1U`: Contactors open, no load/charge present; baseline monitoring active.
  - `CHARGING = 2U`: Contactor closed to charger; positive current flow.
  - `DISCHARGING = 3U`: Contactor closed to load; negative current flow.
  - `BALANCING = 4U`: Passive/active cell balance circuitry engaged.
  - `FAULT_DEGRADED = 5U`: Single faulted cell isolated via bypass MOSFET; reduced pack current.
  - `FAULT_CRITICAL = 6U`: Immediate software shutdown; main contactors opened immediately.
  - `TRIP_DETONATED = 7U`: Non-recoverable post-pyro latched state (busbar severed).
- **`CellStatus : uint8_t`**: `ACTIVE`, `BALANCING`, `BYPASSED`, `SWELLING_WARN`, `FAULTED`.
- **`FaultCode : uint16_t`**:
  - `NONE = 0x0000U`
  - `OVP = 1U << 0` (0x0001U)
  - `UVP = 1U << 1` (0x0002U)
  - `OCP_CHARGE = 1U << 2` (0x0004U)
  - `OCP_DISCHARGE = 1U << 3` (0x0008U)
  - `OTC = 1U << 4` (0x0010U)
  - `UTC = 1U << 5` (0x0020U)
  - `SHORT_CIRCUIT = 1U << 6` (0x0040U)
  - `THERMAL_RUNAWAY = 1U << 7` (0x0080U)
  - `SWELLING_CRITICAL = 1U << 8` (0x0100U)
  - `COMM_TIMEOUT = 1U << 9` (0x0200U)
- **`MetricValidity : uint8_t`**: `VALID = 1U << 0` (0x01U), `STALE = 1U << 1` (0x02U), `FAULT_COMM = 1U << 2` (0x04U), `CALIBRATING = 1U << 3` (0x08U).

### 3.2 Strongly Typed Bitmask Wrappers
- **`FaultMask`**: Encapsulates `uint16_t` bitfield operations (`has`, `set`, `clear`, `reset`, `any`, `is_empty`, `count` via `__builtin_popcount`, raw value) and complete operator overloads (`|`, `&`, `^`, `~`, `|=`, `&=`, `^=`, `==`, `!=`).
- **`ValidityMask`**: Lightweight bitmask wrapper managing `MetricValidity` states with `is_valid()` predicate.

### 3.3 Safety Key: `PyroTriggerKey`
- Requires two distinct high Hamming-distance magic patterns: `ARM_MAGIC = 0x5A5AA5A5U` and `FIRE_MAGIC = 0xC3C33C3CU`.
- Enforces an active confirmation time window of $\le 50\text{ms}$ (`ARM_WINDOW_TIMEOUT_MS = 50U`) and guards against timestamp underflow.
- Documents explicit preconditions requiring independent hardware analog comparator trips prior to invoking `arm()`.

### 3.4 Canonical Telemetry Structs
- **`CellMetrics`**: Per-cell voltage ($V$), temperature ($^\circ\text{C}$), swelling force ($N$), capture timestamp ($ms$), sequence counter, cell status, validity mask, balancing flag, and trailing `crc16` (CRC-16-CCITT).
- **`PackMetrics`**: Pack total voltage ($V$), current ($A$), cell imbalance delta ($V$), max/min temperature ($^\circ\text{C}$), state of charge ($\%$), timestamp ($ms$), sequence counter, active faults mask, active cell count, validity mask, and trailing `crc16`.
- Guaranteed `std::is_standard_layout_v` and `std::is_trivially_copyable_v` for deterministic FreeRTOS critical-section memory operations.

### 3.5 Stringifiers & Predicates
- High-performance, zero-allocation `constexpr inline const char* to_string(...) noexcept` for all enums with safe `"UNKNOWN_..."` fallback on corrupted byte values.
- Bounds validation predicates: `is_valid_bms_state` and `is_valid_cell_status`.

---

## 4. Safety Impact & Risk Analysis

| Subsystem | Risk Addressed | Mitigation Implemented |
| :--- | :--- | :--- |
| **Pyro-Fuse Actuation** | Inadvertent firing caused by bit-flip, ESD, brownout, or stray memory write | Two-stage arm+fire protocol with high Hamming-distance tokens, 50ms time window, and hardware interlock gating |
| **Telemetry Staleness** | Decisions made on dead ADC / hung I2C bus | Monotonic `timestamp_ms` and `ValidityMask` (`STALE`/`FAULT_COMM`) checked before state updates |
| **Fault Mask Corruption** | Bit-shift promotion bugs and adjacent bit loss | Strongly typed `FaultMask` class with unit-tested adjacent bit preservation |
| **Task Concurrency** | Partial / torn telemetry reads across tasks and ISRs | Standard-layout and trivially copyable POD structs for deterministic critical-section `memcpy` |

---

## 5. Verification & Code Coverage

### Automated Test Results
All 7 unit test suites in `test/unit/core/test_types.cpp` compiled under strict flags (`-std=c++17 -Wall -Wextra -Wpedantic -Werror -fno-exceptions -fno-rtti`) and passed:
- `test_type_sizes_and_alignment` $\to$ **PASSED**
- `test_pyro_trigger_key_lifecycle_and_timing` $\to$ **PASSED** (Exact boundaries: 0ms, 25ms, 50ms authorized; 51ms, 52ms, 2000ms rejected; underflow rejected)
- `test_enum_values_and_constants` $\to$ **PASSED**
- `test_fault_mask_operations` $\to$ **PASSED** (Adjacent bit preservation: clearing `OVP` preserves `UVP` & `SHORT_CIRCUIT`)
- `test_validity_mask_operations` $\to$ **PASSED**
- `test_struct_initialization_and_memcpy` $\to$ **PASSED** (Raw `memcpy` preserves all wrapper semantics)
- `test_stringification_and_fuzzing` $\to$ **PASSED** (Out-of-range byte casts return deterministic unknown strings)

### Gcov Code Coverage
```
File 'include/firmware/core/types.hpp'
Lines executed: 100.00% of 146
Branches executed: 100.00% of 44
Taken at least once: 100.00% of 44
Calls executed: 100.00% of 13
```

---

## 6. Checklist
- [x] Code follows project formatting rules (`.clang-format`)
- [x] Scoped enums with explicit underlying types (`uint8_t`, `uint16_t`)
- [x] Zero dynamic allocations (`malloc`/`new`) and zero C++ exceptions (`noexcept`)
- [x] 100% statement and branch coverage on header methods
- [x] Updated project documentation in `docs/`
