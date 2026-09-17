#pragma once

#include <cstdint>

namespace bms {
namespace thresholds {

// Voltage Limits (Per-Cell)
constexpr float CELL_OVERVOLTAGE_TRIP_V = 4.25f;
constexpr float CELL_OVERVOLTAGE_WARN_V = 4.20f;
constexpr float CELL_NOMINAL_VOLTAGE_V = 3.70f;
constexpr float CELL_UNDERVOLTAGE_WARN_V = 3.00f;
constexpr float CELL_UNDERVOLTAGE_TRIP_V = 2.80f;
constexpr float CELL_IMBALANCE_MAX_DELTA_V = 0.020f;  // 20mV passive balancing threshold

// Temperature Limits (Celsius)
constexpr float CELL_OVERTEMP_CRITICAL_C = 55.0f;
constexpr float CELL_OVERTEMP_WARN_C = 45.0f;
constexpr float CELL_UNDERTEMP_CHARGE_LOCK_C = 0.0f;
constexpr float THERMAL_RUNAWAY_RISE_RATE_C_PER_S = 2.0f;  // dT/dt pyro trip

// Current Limits (Pack Level)
constexpr float PACK_CONTINUOUS_MAX_CURRENT_A = 15.0f;
constexpr uint32_t PACK_OVERCURRENT_PERSIST_MS = 2000;  // 2s soft trip
constexpr float PACK_SHORT_CIRCUIT_CRITICAL_A = 50.0f;  // Instantaneous pyro trip

// Mechanical / Swelling Limits (FSR Pressure)
constexpr float CELL_SWELLING_PRESSURE_WARN_N = 5.0f;
constexpr float CELL_SWELLING_PRESSURE_TRIP_N = 12.0f;

// Communication & Heartbeat
constexpr uint32_t SENSOR_HEARTBEAT_TIMEOUT_MS = 500;

}  // namespace thresholds
}  // namespace bms
