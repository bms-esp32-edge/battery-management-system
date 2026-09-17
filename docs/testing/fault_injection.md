# Fault Injection Testing Procedures

## 1. Single-Cell Overvoltage (OVP) Injection
1. Connect programmable power supply to Cell 2 sensing line.
2. Step voltage from 3.70V to 4.30V.
3. **Expected Behavior**:
   - Within 30ms, Cell 2 MOSFET switches to BYPASS state.
   - Per-cell WS2812B LED 2 switches to Solid RED.
   - Pack continues supplying load across Cells 1, 3, and 4 (nominal pack voltage drops by ~3.7V).
   - Event logged to SPI NOR Flash.

## 2. Thermal Runaway ($dT/dt$) Injection
1. Apply rapid heat gun pulse to Cell 3 thermistor ($> 2.5^\circ\text{C/s}$).
2. **Expected Behavior**:
   - Firmware asserts `PYRO_SCR_GATE_PIN` within 500µs.
   - Main pack contactor is disabled immediately.
   - Pyro detonation event is latched in flash log.
