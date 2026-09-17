#pragma once

#include <cstdint>

namespace bms {
namespace pins {

// Communication Busses
constexpr uint8_t I2C_SDA_PIN = 8;
constexpr uint8_t I2C_SCL_PIN = 9;
constexpr uint8_t SPI_MOSI_PIN = 11;
constexpr uint8_t SPI_MISO_PIN = 13;
constexpr uint8_t SPI_SCK_PIN = 12;
constexpr uint8_t FLASH_CS_PIN = 10;

// Analog Sensing Pins (ESP32-S3 ADC channels or I2C addresses)
constexpr uint8_t CURRENT_SENSOR_ADC_PIN = 4;
constexpr uint8_t ADS1115_I2C_ADDR = 0x48;

// Safety Actuation & Gate Drivers
constexpr uint8_t PYRO_FUSE_SCR_GATE_PIN = 14;
constexpr uint8_t MAIN_CONTACTOR_RELAY_PIN = 15;
constexpr uint8_t HARDWARE_ESTOP_PIN = 0;

// Bypass MOSFET Gate Controls (1 Pass + 1 Bypass per cell)
constexpr uint8_t CELL_1_BYPASS_PIN = 16;
constexpr uint8_t CELL_2_BYPASS_PIN = 17;
constexpr uint8_t CELL_3_BYPASS_PIN = 18;
constexpr uint8_t CELL_4_BYPASS_PIN = 19;

// Passive Balancing Bleed MOSFETs
constexpr uint8_t CELL_1_BLEED_PIN = 38;
constexpr uint8_t CELL_2_BLEED_PIN = 39;
constexpr uint8_t CELL_3_BLEED_PIN = 40;
constexpr uint8_t CELL_4_BLEED_PIN = 41;

// Per-Cell Addressable RGB LEDs
constexpr uint8_t WS2812B_LED_DATA_PIN = 48;

}  // namespace pins
}  // namespace bms
