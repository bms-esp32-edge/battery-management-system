# Hardware Pinout & Interface Mapping (ESP32-S3-WROOM-1-N16R8)

| GPIO Pin | Function / Net Name | Target Device / Subsystem | Notes |
| :--- | :--- | :--- | :--- |
| **GPIO 8** | `I2C_SDA` | ADS1115 Voltage ADC (0x48) | 4.7kΩ Pull-up to 3.3V |
| **GPIO 9** | `I2C_SCL` | ADS1115 Voltage ADC (0x48) | 4.7kΩ Pull-up to 3.3V |
| **GPIO 10**| `FLASH_CS` | Winbond W25Q128JV NOR Flash | Active LOW Chip Select |
| **GPIO 11**| `SPI_MOSI` | Winbond W25Q128JV NOR Flash | High-speed SPI Data Out |
| **GPIO 12**| `SPI_SCK` | Winbond W25Q128JV NOR Flash | SPI Clock (40MHz) |
| **GPIO 13**| `SPI_MISO` | Winbond W25Q128JV NOR Flash | High-speed SPI Data In |
| **GPIO 4** | `CURRENT_ADC` | ACS724LLCTR-050B-T | Bi-directional Analog Voltage Output |
| **GPIO 14**| `PYRO_SCR_GATE` | BT151-500R SCR Thyristor | Sub-ms Detonation Gate Pulse |
| **GPIO 15**| `CONTACTOR_DRV` | TE Connectivity EV200 SSR/Relay | High-Voltage Main Pack Isolation |
| **GPIO 0** | `ESTOP_IN` | Emergency Stop Push-Button | Hardware Interrupt Line |
| **GPIO 16**| `BYPASS_GATE_1` | Cell 1 P/N MOSFET Pair Driver | TC4427A Driver Input |
| **GPIO 17**| `BYPASS_GATE_2` | Cell 2 P/N MOSFET Pair Driver | TC4427A Driver Input |
| **GPIO 18**| `BYPASS_GATE_3` | Cell 3 P/N MOSFET Pair Driver | TC4427A Driver Input |
| **GPIO 19**| `BYPASS_GATE_4` | Cell 4 P/N MOSFET Pair Driver | TC4427A Driver Input |
| **GPIO 38**| `BLEED_GATE_1` | Cell 1 2N7002 Bleed Switch | 39Ω 2W Bleed Resistor |
| **GPIO 39**| `BLEED_GATE_2` | Cell 2 2N7002 Bleed Switch | 39Ω 2W Bleed Resistor |
| **GPIO 40**| `BLEED_GATE_3` | Cell 3 2N7002 Bleed Switch | 39Ω 2W Bleed Resistor |
| **GPIO 41**| `BLEED_GATE_4` | Cell 4 2N7002 Bleed Switch | 39Ω 2W Bleed Resistor |
| **GPIO 48**| `WS2812B_DIN` | WS2812B Addressable LED Chain | Per-Cell RGB Telemetry |
