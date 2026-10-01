// ============================================================
// ARCHITECTURAL NOTE:
// These classes are strictly pure data containers ("nouns") with zero logic.
// Validation, scaling (raw ADC to pressure units), and threshold checks are
// delegated to the upper firmware processing/driver implementation layer.
// Setters perform direct assignment without runtime validation overhead.
// ============================================================
#pragma once

#include <array>
#include <cstdint>
#include <type_traits>

#include "modules/types/telemetry_types.hpp"

namespace bms::modules::drivers {

namespace types = bms::modules::types;

class SwellingRawData {
private:
    std::array<uint16_t, types::MAX_CELL_COUNT> raw_counts_{};  // Raw FSR ADC counts per cell
    uint32_t acquisition_timestamp_{0U};  // Microsecond or millisecond timestamp
    bool is_conversion_valid_{false};     // Hardware communication status flag

public:
    constexpr SwellingRawData() noexcept = default;

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

class SwellingProcessedData {
private:
    std::array<float, types::MAX_CELL_COUNT>
        cell_pressure_kpa_{};  // Calibrated swelling pressure per cell, in kPa
    std::array<bool, types::MAX_CELL_COUNT>
        is_swelling_detected_{};  // Per-cell flag: pressure exceeded safety threshold

public:
    constexpr SwellingProcessedData() noexcept = default;

    // Getters
    [[nodiscard]] constexpr const std::array<float, types::MAX_CELL_COUNT>& get_cell_pressure_kpa()
        const noexcept {
        return cell_pressure_kpa_;
    }

    [[nodiscard]] constexpr const std::array<bool, types::MAX_CELL_COUNT>&
    get_is_swelling_detected() const noexcept {
        return is_swelling_detected_;
    }

    // Setters
    constexpr void set_cell_pressure_kpa(
        const std::array<float, types::MAX_CELL_COUNT>& pressures) noexcept {
        cell_pressure_kpa_ = pressures;
    }

    constexpr void set_is_swelling_detected(
        const std::array<bool, types::MAX_CELL_COUNT>& detected) noexcept {
        is_swelling_detected_ = detected;
    }
};

// Compile-time safety assertions for hardware data layout mapping
static_assert(std::is_standard_layout_v<SwellingRawData>,
              "SwellingRawData must be standard layout");
static_assert(std::is_trivially_copyable_v<SwellingRawData>,
              "SwellingRawData must be trivially copyable");

static_assert(std::is_standard_layout_v<SwellingProcessedData>,
              "SwellingProcessedData must be standard layout");
static_assert(std::is_trivially_copyable_v<SwellingProcessedData>,
              "SwellingProcessedData must be trivially copyable");

}  // namespace bms::modules::drivers