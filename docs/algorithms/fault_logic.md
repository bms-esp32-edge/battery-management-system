# Fault Detection & Isolation Matrix

| FaultCode Enum | Bit Value | Trigger Condition | Detection Mechanism | System Action | Latching? |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `FaultCode::OVP` | `0x0001` | $V_{cell} > 4.25\text{V}$ | 16-Bit ADS1115 (3 consecutive reads) | Isolate faulted cell via MOSFET Bypass | Yes |
| `FaultCode::UVP` | `0x0002` | $V_{cell} < 2.80\text{V}$ | 16-Bit ADS1115 (3 consecutive reads) | Isolate faulted cell via MOSFET Bypass | Yes |
| `FaultCode::OCP_CHARGE` | `0x0004` | $I_{pack} > 15.0\text{A}$ (Charge) for $> 2\text{s}$ | ACS724 Continuous Average | Open Main Pack Contactor Relay | Yes |
| `FaultCode::OCP_DISCHARGE` | `0x0008` | $I_{pack} < -15.0\text{A}$ (Discharge) for $> 2\text{s}$ | ACS724 Continuous Average | Open Main Pack Contactor Relay | Yes |
| `FaultCode::OTC` | `0x0010` | $T_{cell} > 55.0^\circ\text{C}$ | NTC 10K Probe | Open Main Contactor / Bypass Cell | Yes |
| `FaultCode::UTC` | `0x0020` | $T_{cell} < 0.0^\circ\text{C}$ | NTC 10K Probe | Inhibit Charging (Open Charge Switch) | Yes |
| `FaultCode::SHORT_CIRCUIT` | `0x0040` | $I_{pack} > 50.0\text{A}$ | LM393 Hardware Comparator | Fire BT151 SCR via `PyroTriggerKey` | Permanent |
| `FaultCode::THERMAL_RUNAWAY` | `0x0080` | $dT/dt > 2.0^\circ\text{C/s}$ | Differential Thermistor Derivative | Fire BT151 SCR via `PyroTriggerKey` | Permanent |
| `FaultCode::SWELLING_CRITICAL` | `0x0100` | $P_{cell} > 12.0\text{N}$ | FSR 402 Pressure Map | Isolate faulted cell via MOSFET Bypass | Yes |
| `FaultCode::COMM_TIMEOUT` | `0x0200` | I2C Heartbeat Timeout | Hardware Timeout Watchdog (>500ms) | Open Main Pack Contactor Relay | Auto-recover |

