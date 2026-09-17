#pragma once

#include <cstdint>

namespace bms {
namespace config {

// Pack Topology
constexpr uint8_t TOTAL_CELL_COUNT = 4;  // 4S Li-Ion topology
constexpr float CELL_NOMINAL_CAPACITY_AH = 2.5f;

// Loop Timing & Task Frequencies (Milliseconds)
constexpr uint32_t TELEMETRY_LOOP_INTERVAL_MS = 100;
constexpr uint32_t PROTECTION_LOOP_INTERVAL_MS = 10;
constexpr uint32_t TUI_REFRESH_INTERVAL_MS = 250;
constexpr uint32_t ALGORITHM_UPDATE_INTERVAL_MS = 500;

// Logging Buffer
constexpr uint32_t PRE_FAULT_LOG_BUFFER_SIZE = 128;

}  // namespace config
}  // namespace bms
