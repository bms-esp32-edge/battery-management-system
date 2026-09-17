# Bill of Materials (BOM)

| Subsystem | Component / Part Number | Description | Qty |
| :--- | :--- | :--- | :--- |
| **Main Processing Unit** | ESP32-S3-WROOM-1-N16R8 | Dual-core 240MHz MCU, 16MB Flash, 8MB PSRAM | 1 |
| **Cell Voltage ADC** | ADS1115 / LTC6811 | 16-Bit I2C ADC with programmable gain | 1 |
| **Current Sensor** | Allegro ACS724LLCTR-050B-T | ±50A Galvanically Isolated Hall-Effect Sensor | 1 |
| **Cell Thermistors** | NTC 10K 3950 (1% Tolerance) | High-precision thermal sensing probes | 5 |
| **Swelling Sensors** | Interlink Electronics FSR 402 | Force Sensitive Resistor for cell expansion | 4 |
| **Bypass MOSFETs** | IRF4905 & IRLB8721 | High-current P-Channel (Bypass) & N-Channel (Pass) | 4 pairs |
| **Gate Driver ICs** | Microchip TC4427A / TLP250 | High-speed Dual MOSFET Gate Drivers | 2 |
| **Emergency Pyro-Fuse** | Littelfuse Pyro-Fuse (Squib) | Sub-ms Busbar Severing Pyrotechnic Fuse | 1 |
| **Pyro Discharge Driver**| BT151-500R SCR Thyristor | 12A 500V High-surge SCR trigger | 1 |
| **Discharge Capacitor** | 4700µF 35V Electrolytic Cap | Pyro ignition energy reservoir | 1 |
| **Passive Bleed Sw.** | 2N7002 + 39Ω 2W Resistors | Imbalance energy bleed dissipation network | 4 |
| **Main Pack Contactor** | TE Connectivity EV200HAANA | 500A High-voltage contactor relay | 1 |
| **RGB LED Telemetry** | WS2812B 5050 RGB LEDs | Addressable multi-color status indicators | 4 |
| **Black-Box Flash** | Winbond W25Q128JV (SOIC-8) | 128Mb (16MB) High-speed SPI NOR Flash | 1 |
| **Analog Comparator** | LM393 Dual Comparator | Sub-millisecond hardware short-circuit trip | 1 |
| **Power Supply** | LM2596 HV / MP2307 | DC-DC Step-Down Buck Converter (36V -> 5V/3.3V) | 1 |
