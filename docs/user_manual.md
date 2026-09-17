# BMS User Manual & CLI Guide

## Interactive Serial Console Commands

Connect over Serial at **115200 Baud** (or launch native simulation).

| Command | Description | Example |
| :--- | :--- | :--- |
| `status` | Print current pack metrics, cell voltages, and temperatures | `status` |
| `bypass <cell_id>` | Manually isolate and bypass a specified cell (1-4) | `bypass 2` |
| `unbypass <cell_id>`| Restore an isolated cell to the active string | `unbypass 2` |
| `trip_test` | Trigger a safe simulated pyro-fuse trip sequence | `trip_test` |
| `reset_faults` | Clear latched non-critical fault flags | `reset_faults` |
| `dump_logs` | Export pre-fault circular log buffer from Flash | `dump_logs` |

---

## Live ASCII TUI Dashboard
When running in TUI mode, the terminal displays real-time ASCII bar graphs:

```
================================================================================
   ESP32-S3 SMART BATTERY MANAGEMENT SYSTEM - TELEMETRY DASHBOARD
================================================================================
 [PACK STATUS: NORMAL]      [TOPOLOGY: 4/4 ACTIVE]      [CONTACTOR: CLOSED]
 Total Voltage: 14.80 V      Pack Current: -2.35 A       Pack Power: 34.78 W
 SOC: [████████████████░░░░] 82.0%     SOH: 96.5%        Delta V: 0.012 V
--------------------------------------------------------------------------------
 CELL VOLTAGES:
   Cell 1 [ACTIVE ]: [████████████████████] 3.705 V  | Temp: 28.2 C | Swell: 0.8 N
   Cell 2 [ACTIVE ]: [████████████████████] 3.698 V  | Temp: 28.5 C | Swell: 0.7 N
   Cell 3 [ACTIVE ]: [████████████████████] 3.710 V  | Temp: 29.1 C | Swell: 0.9 N
   Cell 4 [ACTIVE ]: [████████████████████] 3.692 V  | Temp: 28.0 C | Swell: 0.6 N
--------------------------------------------------------------------------------
 EVENT LOG:
 [18:24:02.100] [INFO] System POST completed. All 4 cells online.
 [18:24:10.450] [INFO] Pack discharging at 2.35A.
================================================================================
```
