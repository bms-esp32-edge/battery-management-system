// ============================================================
// ARCHITECTURAL NOTE:
// These classes are strictly pure data containers ("nouns") with zero logic.
// Validation, scaling (raw ADC to °C), and filtering are
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

class TemperatureRawData {
private:
    std::array<uint16_t, types::MAX_THERMISTORS>
        raw_counts_{};                    // Raw ADC counts per thermistor channel
    uint32_t acquisition_timestamp_{0U};  // Microsecond or millisecond timestamp
    bool is_conversion_valid_{false};     // Hardware communication status flag

public:
    constexpr TemperatureRawData() noexcept = default;

    // Getters
    [[nodiscard]] constexpr const std::array<uint16_t, types::MAX_THERMISTORS>& get_raw_counts()
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
        const std::array<uint16_t, types::MAX_THERMISTORS>& counts) noexcept {
        raw_counts_ = counts;
    }

    constexpr void set_acquisition_timestamp(uint32_t timestamp) noexcept {
        acquisition_timestamp_ = timestamp;
    }

    constexpr void set_conversion_valid(bool valid) noexcept { is_conversion_valid_ = valid; }
};

class TemperatureProcessedData {
private:
    std::array<float, types::MAX_THERMISTORS>
        cell_temps_c_{};                 // Calibrated temperature in Celsius, per thermistor
    float ambient_temp_c_{0.0f};         // Baseline ambient reference temperature
    float max_cell_temp_c_{0.0f};        // Highest reading across all thermistors
    float thermal_rise_rate_c_s_{0.0f};  // dT/dt — rate of temperature increase (°C/sec)

public:
    constexpr TemperatureProcessedData() noexcept = default;

    // Getters
    [[nodiscard]] constexpr const std::array<float, types::MAX_THERMISTORS>& get_cell_temps_c()
        const noexcept {
        return cell_temps_c_;
    }

    [[nodiscard]] constexpr float get_ambient_temp_c() const noexcept { return ambient_temp_c_; }

    [[nodiscard]] constexpr float get_max_cell_temp_c() const noexcept { return max_cell_temp_c_; }

    [[nodiscard]] constexpr float get_thermal_rise_rate_c_s() const noexcept {
        return thermal_rise_rate_c_s_;
    }

    // Setters
    constexpr void set_cell_temps_c(
        const std::array<float, types::MAX_THERMISTORS>& temps) noexcept {
        cell_temps_c_ = temps;
    }

    constexpr void set_ambient_temp_c(float ambient) noexcept { ambient_temp_c_ = ambient; }

    constexpr void set_max_cell_temp_c(float max_temp) noexcept { max_cell_temp_c_ = max_temp; }

    constexpr void set_thermal_rise_rate_c_s(float rate) noexcept { thermal_rise_rate_c_s_ = rate; }
};

// Compile-time safety assertions for hardware data layout mapping
static_assert(std::is_standard_layout_v<TemperatureRawData>,
              "TemperatureRawData must be standard layout");
static_assert(std::is_trivially_copyable_v<TemperatureRawData>,
              "TemperatureRawData must be trivially copyable");

static_assert(std::is_standard_layout_v<TemperatureProcessedData>,
              "TemperatureProcessedData must be standard layout");
static_assert(std::is_trivially_copyable_v<TemperatureProcessedData>,
              "TemperatureProcessedData must be trivially copyable");

}  // namespace bms::modules::drivers