// =========================================================================
// ARCHITECTURAL NOTE:
// These classes are strictly pure data containers ("nouns") with zero logic.
// Validation, scaling (mV to Amperes conversion), and filtering are
// delegated to the upper firmware processing/driver implementation layer.
// Setters perform direct assignment without runtime validation overhead.
// =========================================================================
#pragma once

#include <cstdint>
#include <type_traits>

namespace bms::modules::drivers {

class CurrentSensorRawData {
private:
    uint16_t raw_count_{0U};              // Raw ADC count from the current sensor channel
    uint32_t acquisition_timestamp_{0U};  // Microsecond or millisecond timestamp
    bool is_conversion_valid_{false};     // Hardware communication status flag

public:
    constexpr CurrentSensorRawData() noexcept = default;

    // Getters
    [[nodiscard]] constexpr uint16_t get_raw_count() const noexcept { return raw_count_; }

    [[nodiscard]] constexpr uint32_t get_acquisition_timestamp() const noexcept {
        return acquisition_timestamp_;
    }

    [[nodiscard]] constexpr bool is_conversion_valid() const noexcept {
        return is_conversion_valid_;
    }

    // Setters
    constexpr void set_raw_count(uint16_t count) noexcept { raw_count_ = count; }

    constexpr void set_acquisition_timestamp(uint32_t timestamp) noexcept {
        acquisition_timestamp_ = timestamp;
    }

    constexpr void set_conversion_valid(bool valid) noexcept { is_conversion_valid_ = valid; }
};

class CurrentSensorProcessedData {
private:
    float current_amperes_{
        0.0f};  // Calibrated current in Amperes (+ for charging, - for discharging)
    float filtered_current_amperes_{0.0f};  // Low-pass filtered current value for stability

public:
    constexpr CurrentSensorProcessedData() noexcept = default;

    // Getters
    [[nodiscard]] constexpr float get_current_amperes() const noexcept { return current_amperes_; }

    [[nodiscard]] constexpr float get_filtered_current_amperes() const noexcept {
        return filtered_current_amperes_;
    }

    // Setters
    constexpr void set_current_amperes(float current) noexcept { current_amperes_ = current; }

    constexpr void set_filtered_current_amperes(float filtered) noexcept {
        filtered_current_amperes_ = filtered;
    }
};

// Compile-time safety assertions for hardware data layout mapping
static_assert(std::is_standard_layout_v<CurrentSensorRawData>,
              "CurrentSensorRawData must be standard layout");
static_assert(std::is_trivially_copyable_v<CurrentSensorRawData>,
              "CurrentSensorRawData must be trivially copyable");

static_assert(std::is_standard_layout_v<CurrentSensorProcessedData>,
              "CurrentSensorProcessedData must be standard layout");
static_assert(std::is_trivially_copyable_v<CurrentSensorProcessedData>,
              "CurrentSensorProcessedData must be trivially copyable");

}  // namespace bms::modules::drivers