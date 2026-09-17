# Hardware Safety & Operating Protocols

> [!CAUTION]
> **High Energy & Pyrotechnic Device Warnings**
> 1. **Pyro-Fuse Ignition Circuit**: The BT151 SCR is linked to a 4700µF capacitor bank charged to trigger the pyrotechnic fuse. Always discharge the capacitor through a 1kΩ bleed resistor before handling or reprogramming the board.
> 2. **Bypass MOSFET Dead-Time**: The firmware must enforce a minimum 10µs dead-time between turning OFF the pass MOSFET and turning ON the bypass MOSFET to prevent bridge shoot-through short circuits.
> 3. **Manual E-Stop**: The manual emergency stop button is wired directly to an active-low hardware interrupt on GPIO 0 and opens the main pack contactor unconditionally.
