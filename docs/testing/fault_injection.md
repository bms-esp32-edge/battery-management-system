# Fault Injection Testing Procedures

## 1. Single-Cell Overvoltage (OVP) Injection
1. Connect programmable power supply to Cell 2 sensing line.
2. Step voltage from 3.70V to 4.30V.
3. **Expected Behavior**:
   - Within 30ms, Cell 2 MOSFET switches to BYPASS state.
   - Per-cell WS2812B LED 2 switches to Solid RED.
   - Pack continues supplying load across Cells 1, 3, and 4 (nominal pack voltage drops by ~3.7V).
   - Telemetry queue is flushed to SPI NOR Flash immediately.
4. **Post-Test Verification**:
   - Issue `dump_logs 10` on serial console.
   - Verify record sequence contains `fault_flags` bit `0x0001` (`FaultCode::OVP`) with valid CRC32.

## 2. Thermal Runaway ($dT/dt$) Injection
1. Apply rapid heat gun pulse to Cell 3 thermistor ($> 2.5^\circ\text{C/s}$).
2. **Expected Behavior**:
   - Firmware asserts `PYRO_SCR_GATE_PIN` within 500µs.
   - Main pack contactor opens immediately.
   - `FlashLogger` intercepts critical trip, executes synchronous erase-suspend (if erase active), flushes fault record, and commits persistent `FreezeMarker` to Sector 4094.
3. **Post-Test Verification**:
   - Connect serial console and issue `log_stats`.
   - Confirm `Evidence Freeze: ACTIVE` locking up to 16 sectors around the fault.
   - Issue `dump_logs 64` to export the high-rate pre-trip and post-trip trajectory.
   - Issue `clear_freeze` after saving telemetry data.

## 3. Fault During Active Background Sector Erase (Erase-Suspend)
1. Fill flash memory until the logger enters ring-wrap sector reclamation.
2. Trigger an artificial fault (`OVP` or `OCP`) while the W25Q128JV is actively erasing a sector.
3. **Expected Behavior**:
   - Flash driver issues `0x75` (`W25Q_CMD_ERASE_SUSPEND`) and enters suspended state in $< 20\mu\text{s}$.
   - Emergency fault record is programmed into a clean pre-erased sector.
   - Driver verifies write via readback and issues `0x7A` (`W25Q_CMD_ERASE_RESUME`).
   - Background sector erase completes with zero data corruption.

