# Test Strategy

## Dual-Target Testing Architecture

```
                    +-----------------------------+
                    |       C++ BMS Codebase      |
                    +-----------------------------+
                             /            \
                            /              \
             [Host Native Platform]     [ESP32-S3 Hardware Target]
                    /                              \
       +-------------------------+      +-------------------------+
       | Fast CI Unity Runner    |      | Benchtop Testing (HIL)  |
       | Mock Drivers & Vectors  |      | Physical Scope / Load   |
       | pio test -e native      |      | pio run -e esp32-s3...  |
       +-------------------------+      +-------------------------+
```

1. **Host Native Mode (`[env:native]`)**:
   - Runs in GitHub Actions and local developer terminals.
   - Leverages `test/mocks/` to simulate sensor values, I2C timeouts, and short-circuit triggers without physical hardware.
2. **Hardware Target Mode (`[env:esp32-s3-devkitc-1]`)**:
   - Compiles against ESP-IDF / Arduino framework.
   - Validates memory layout (PSRAM, Flash) and real GPIO register operations.
