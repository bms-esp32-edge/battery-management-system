# System Architecture

## 1. Overview
The ESP32-S3 Smart Battery Management System (BMS) provides real-time multi-cell monitoring, proactive single-cell bypass fault isolation, sub-millisecond emergency pyro-fuse tripping, TinyML health estimation, and live ASCII telemetry.

```
                  +----------------------------------------------+
                  |               ESP32-S3 MCU                   |
                  |  - Dual Core 240MHz + 16MB Flash + 8MB PSRAM |
                  +----------------------------------------------+
                           |           |               |
              +------------+           |               +---------------+
              | I2C                    | SPI                           | GPIO / PWM
              v                        v                               v
    +-------------------+    +---------------------+        +--------------------+
    | ADS1115 16-Bit    |    | Winbond W25Q128JV   |        | WS2812B RGB LEDs   |
    | Precision ADC     |    | SPI NOR Flash       |        | (Per-cell status)  |
    +-------------------+    +---------------------+        +--------------------+
              |
              +--> [Cell 1..4 Voltage Sensing]
    
    Analog/Digital GPIO:
    - Current: ACS724 Hall Sensor (±50A bi-directional)
    - Temp: 4x NTC 10K Thermistors + 1x Ambient
    - Pressure: 4x Interlink FSR 402 Swelling Sensors
    - Bypass Drivers: 4x IRF4905 / IRLB8721 MOSFET pairs
    - Emergency Cutoff: BT151 SCR + 4700µF Cap Bank -> Littelfuse Pyro-Fuse
```

## 2. Finite State Machine (FSM)

```mermaid
stateDiagram-v2
    [*] --> INIT
    INIT --> STANDBY: Power-on Self Test (POST) Passed
    INIT --> FAULT_CRITICAL: Sensor/Communication Error

    STANDBY --> CHARGING: Current > +0.2A
    STANDBY --> DISCHARGING: Current < -0.2A
    STANDBY --> BALANCING: Delta Vcell > 20mV

    CHARGING --> STANDBY: Current ~ 0A or Full Charge
    DISCHARGING --> STANDBY: Current ~ 0A

    CHARGING --> FAULT_DEGRADED: Single Cell OVP (>4.25V) / Swell Trip
    DISCHARGING --> FAULT_DEGRADED: Single Cell UVP (<2.80V) / Swell Trip
    
    FAULT_DEGRADED --> STANDBY: Faulted Cell Isolated via Bypass MOS

    CHARGING --> FAULT_CRITICAL: Pack Overload (>15A for 2s) / Over-Temp (>55°C)
    DISCHARGING --> FAULT_CRITICAL: Pack Overload (>15A for 2s) / Over-Temp (>55°C)
    
    FAULT_CRITICAL --> TRIP_DETONATED: Short Circuit (>50A) or Thermal Runaway (dT/dt > 2°C/s)
    TRIP_DETONATED --> [*]: Hardware Pyro Busbar Severed
```
