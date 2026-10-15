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
| `dump_logs [N]` | Export latest $N$ flight records (or all active records) | `dump_logs 100` |
| `log_stats` | Inspect flash ring buffer pointer, drop counts, and freeze status | `log_stats` |
| `clear_freeze` | Authorize clearance of persistent post-crash evidence freeze | `clear_freeze` |
| `erase_logs` | Format log sectors 4..4093 (preserves frozen sectors and stats) | `erase_logs` |

---

## Black-Box Flight Log Analysis & Post-Crash Investigation

The BMS circular NOR flight recorder retains up to **261,760** records. In the event of a critical pack trip (contactor opening, cell bypass, or pyro-fuse detonation), telemetry immediately precedes and follows the trigger.

### Exporting Records via Serial CLI

Issuing `dump_logs` outputs CSV-formatted telemetry suitable for ingestion into Python, MATLAB, or Pandas:

```csv
seq,boot,timestamp_ms,v1_mv,v2_mv,v3_mv,v4_mv,current_ma,temp_max_c,swell_max_n,soc_pct,faults,crc_ok
10420,3,142050,3705,3698,3710,3692,-2350,28.2,0.8,82.0,0x0000,1
10421,3,142150,3704,3697,3709,3692,-2360,28.2,0.8,82.0,0x0000,1
10422,3,142250,3703,3695,4260,3691,-2400,28.5,0.9,81.9,0x0001,1
```

### Crash Freeze Evidence Retrieval

1. **Verify Evidence Freeze**:
   ```bash
   > log_stats
   [FLASH] Active Head Sector: 48 (Slot 12)
   [FLASH] Total Records Logged: 3,084
   [FLASH] Erased Sectors Ahead: 4
   [FLASH] Evidence Freeze: ACTIVE [Sectors 32..48, Fault Seq 10422, Boot 3]
   ```
2. **Export Frozen Incident Range**:
   Run `dump_logs 1024` to extract the full 16-sector window surrounding the fault.
3. **Release Lock After Review**:
   Once logs are safely archived, issue `clear_freeze` to release the sector lock and resume full circular wear leveling.

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
