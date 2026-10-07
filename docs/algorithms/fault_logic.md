# Fault Detection & Isolation Matrix

| FaultCode Enum | Bit Value | Trigger Condition | Detection Mechanism | System Action | Latching? |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `FaultCode::OVP` | `0x0001` | $V_{cell} > 4.25\text{V}$ | 16-Bit ADS1115 (3 consecutive reads) | Isolate faulted cell via MOSFET Bypass; Flush Flash Log | Yes |
| `FaultCode::UVP` | `0x0002` | $V_{cell} < 2.80\text{V}$ | 16-Bit ADS1115 (3 consecutive reads) | Isolate faulted cell via MOSFET Bypass; Flush Flash Log | Yes |
| `FaultCode::OCP_CHARGE` | `0x0004` | $I_{pack} > 15.0\text{A}$ (Charge) for $> 2\text{s}$ | ACS724 Continuous Average | Open Main Pack Contactor Relay; Flush Flash Log | Yes |
| `FaultCode::OCP_DISCHARGE` | `0x0008` | $I_{pack} < -15.0\text{A}$ (Discharge) for $> 2\text{s}$ | ACS724 Continuous Average | Open Main Pack Contactor Relay; Flush Flash Log | Yes |
| `FaultCode::OTC` | `0x0010` | $T_{cell} > 55.0^\circ\text{C}$ | NTC 10K Probe | Open Main Contactor / Bypass Cell; Flush Flash Log | Yes |
| `FaultCode::UTC` | `0x0020` | $T_{cell} < 0.0^\circ\text{C}$ | NTC 10K Probe | Inhibit Charging (Open Charge Switch); Flush Flash Log | Yes |
| `FaultCode::SHORT_CIRCUIT` | `0x0040` | $I_{pack} > 50.0\text{A}$ | LM393 Hardware Comparator | Fire BT151 SCR via `PyroTriggerKey`; Lock Evidence Freeze; Erase-Suspend Flush | Permanent |
| `FaultCode::THERMAL_RUNAWAY` | `0x0080` | $dT/dt > 2.0^\circ\text{C/s}$ | Differential Thermistor Derivative | Fire BT151 SCR via `PyroTriggerKey`; Lock Evidence Freeze; Erase-Suspend Flush | Permanent |
| `FaultCode::SWELLING_CRITICAL` | `0x0100` | $P_{cell} > 12.0\text{N}$ | FSR 402 Pressure Map | Isolate faulted cell via MOSFET Bypass; Lock Evidence Freeze; Flush Flash Log | Yes |
| `FaultCode::COMM_TIMEOUT` | `0x0200` | I2C Heartbeat Timeout | Hardware Timeout Watchdog (>500ms) | Open Main Pack Contactor Relay; Flush Flash Log | Auto-recover |

---

## Flash Black-Box Logging & Evidence Freeze Interlock

1. **Synchronous Fault Logging**: Whenever any `FaultCode` is asserted, the `FlashLogger` intercepts the event, drains all pending telemetry records from the FreeRTOS logging queue, and immediately commits the pre-fault and fault records to SPI NOR flash.
2. **Sub-millisecond Erase-Suspend**: If a background sector erase is active when a fault occurs, the driver issues command `0x75` (`W25Q_CMD_ERASE_SUSPEND`) to write the fault page within microseconds rather than waiting up to 1 second for sector erasure.
3. **Crash Evidence Freeze**: For safety-critical events (`SHORT_CIRCUIT`, `THERMAL_RUNAWAY`, `SWELLING_CRITICAL`), the logger writes a non-volatile `FreezeMarker` to Sector 4094 locking up to 16 sectors around the fault. This guarantees forensic flight data cannot be overwritten by circular wear-leveling until authorized service personnel clear the lock.


