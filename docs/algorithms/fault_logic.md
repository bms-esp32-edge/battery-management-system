# Fault Detection & Isolation Matrix

| Fault Code | Condition | Detection Mechanism | System Action | Latching? |
| :--- | :--- | :--- | :--- | :--- |
| `ERR_OVP` | $V_{cell} > 4.25\text{V}$ | 16-Bit ADS1115 (3 consecutive reads) | Isolate faulted cell via MOSFET Bypass | Yes |
| `ERR_UVP` | $V_{cell} < 2.80\text{V}$ | 16-Bit ADS1115 (3 consecutive reads) | Isolate faulted cell via MOSFET Bypass | Yes |
| `ERR_SWELL` | $P_{cell} > 12.0\text{N}$ | FSR 402 Pressure Map | Isolate faulted cell via MOSFET Bypass | Yes |
| `ERR_OCP` | $I_{pack} > 15.0\text{A}$ for $> 2\text{s}$ | ACS724 Continuous Average | Open Main Pack Contactor Relay | Yes |
| `ERR_SHORT` | $I_{pack} > 50.0\text{A}$ | LM393 Hardware Comparator | Fire BT151 SCR (Detonate Pyro-Fuse) | Permanent |
| `ERR_RUNAWAY`| $dT/dt > 2.0^\circ\text{C/s}$ | Differential Thermistor Derivative | Fire BT151 SCR (Detonate Pyro-Fuse) | Permanent |
| `ERR_OTC` | $T_{cell} > 55.0^\circ\text{C}$ | NTC 10K Probe | Open Main Contactor / Bypass Cell | Yes |
| `ERR_COMM` | I2C Heartbeat Timeout | Hardware Timeout Watchdog (>500ms) | Open Main Pack Contactor Relay | Auto-recover |
