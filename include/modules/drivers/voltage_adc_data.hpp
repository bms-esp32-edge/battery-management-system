// =========================================================================
// ARCHITECTURAL NOTE:
// These classes are strictly pure data containers ("nouns") with zero logic.
// Validation, bounds-checking (e.g., min/max ranges, delta verification), and
// business rules are intentionally delegated to the upper firmware processing layer.
// Setters perform direct assignment without runtime validation overhead.
// =========================================================================
#pragma once

#include <array>
#include <cstdint>
#include <type_traits>

#include "modules/types/telemetry_types.hpp"

namespace bms::modules::drivers {

namespace types = bms::modules::types;

class VoltageAdcRawData {
private:
    std::array<uint16_t, types::MAX_CELL_COUNT> raw_counts_{};  // Raw 16-bit ADC counts per channel
    uint32_t acquisition_timestamp_{0U};  // Microsecond or millisecond timestamp
    bool is_conversion_valid_{false};     // Hardware communication status flag

public:
    constexpr VoltageAdcRawData() noexcept = default;
    // Getters
    [[nodiscard]] constexpr const std::array<uint16_t, types::MAX_CELL_COUNT>& get_raw_counts()
        const noexcept {
        return raw_counts_;
    }
    [[nodiscard]] constexpr uint32_t get_acquisition_timestamp() const noexcept {
        return acquisition_timestamp_;
    }

    [[nodiscard]] constexpr bool is_conversion_valid() const noexcept {
        return is_conversion_valid_;
    }

    // Setters
    constexpr void set_raw_counts(
        const std::array<uint16_t, types::MAX_CELL_COUNT>& counts) noexcept {
        raw_counts_ = counts;
    }

    constexpr void set_acquisition_timestamp(uint32_t timestamp) noexcept {
        acquisition_timestamp_ = timestamp;
    }

    constexpr void set_conversion_valid(bool valid) noexcept { is_conversion_valid_ = valid; }
};

class VoltageProcessedData {
private:
    std::array<uint16_t, types::MAX_CELL_COUNT>
        cell_voltages_mv_{};            // Calibrated voltage in millivolts
    uint16_t min_voltage_mv_{0U};       // Pack minimum cell voltage
    uint16_t max_voltage_mv_{0U};       // Pack maximum cell voltage
    uint16_t delta_voltage_mv_{0U};     // Imbalance delta (max - min)
    float total_pack_voltage_v_{0.0f};  // Cumulative pack voltage

public:
    constexpr VoltageProcessedData() noexcept = default;
    // Getters
    [[nodiscard]] constexpr const std::array<uint16_t, types::MAX_CELL_COUNT>&
    get_cell_voltages_mv() const noexcept {
        return cell_voltages_mv_;
    }

    [[nodiscard]] constexpr uint16_t get_min_voltage_mv() const noexcept { return min_voltage_mv_; }

    [[nodiscard]] constexpr uint16_t get_max_voltage_mv() const noexcept { return max_voltage_mv_; }

    [[nodiscard]] constexpr uint16_t get_delta_voltage_mv() const noexcept {
        return delta_voltage_mv_;
    }

    [[nodiscard]] constexpr float get_total_pack_voltage_v() const noexcept {
        return total_pack_voltage_v_;
    }

    // Setters
    constexpr void set_cell_voltages_mv(
        const std::array<uint16_t, types::MAX_CELL_COUNT>& voltages) noexcept {
        cell_voltages_mv_ = voltages;
    }

    constexpr void set_min_voltage_mv(uint16_t min_v) noexcept { min_voltage_mv_ = min_v; }

    constexpr void set_max_voltage_mv(uint16_t max_v) noexcept { max_voltage_mv_ = max_v; }

    constexpr void set_delta_voltage_mv(uint16_t delta_v) noexcept { delta_voltage_mv_ = delta_v; }

    constexpr void set_total_pack_voltage_v(float total_v) noexcept {
        total_pack_voltage_v_ = total_v;
    }
};

// Compile-time safety assertions for hardware register/data layout mapping
static_assert(std::is_standard_layout_v<VoltageAdcRawData>,
              "VoltageAdcRawData must be standard layout");
static_assert(std::is_trivially_copyable_v<VoltageAdcRawData>,
              "VoltageAdcRawData must be trivially copyable");

static_assert(std::is_standard_layout_v<VoltageProcessedData>,
              "VoltageProcessedData must be standard layout");
static_assert(std::is_trivially_copyable_v<VoltageProcessedData>,
              "VoltageProcessedData must be trivially copyable");

}  // namespace bms::modules::drivers