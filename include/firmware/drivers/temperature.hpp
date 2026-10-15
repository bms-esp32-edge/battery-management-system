// ============================================================================
// ARCHITECTURAL NOTE:
// This header declares the Temperature driver interface for the firmware layer.
// It interacts with the hardware/ESP-IDF ADC APIs across multiple thermistor
// channels and populates the pure data container defined in
// modules/drivers/temperature_data.hpp.
// ============================================================================

#pragma once

#include <cstdint>

#include "esp_adc/adc_oneshot.h"
#include "modules/drivers/temperature_data.hpp"
#include "modules/types/telemetry_types.hpp"

namespace bms::firmware::drivers {

namespace types = bms::modules::types;

class TemperatureDriver {
public:
    TemperatureDriver() noexcept = default;
    ~TemperatureDriver() noexcept;

    TemperatureDriver(const TemperatureDriver&) = delete;
    TemperatureDriver& operator=(const TemperatureDriver&) = delete;

    TemperatureDriver(TemperatureDriver&&) noexcept = delete;
    TemperatureDriver& operator=(TemperatureDriver&&) noexcept = delete;

    /**
     * @brief Initializes the ADC peripheral for all thermistor channels.
     * @return true if initialization is successful, false otherwise.
     */
    // TODO: channel array should come from firmware/config/pins.hpp once it exists.
    [[nodiscard]] bool init() noexcept;

    /**
     * @brief Reads raw ADC counts for all thermistor channels.
     * @param data Reference to the TemperatureRawData pure data container to be filled.
     * @return true if read was successful on all channels, false if any hardware failure occurred.
     */
    [[nodiscard]] bool read_raw(bms::modules::drivers::TemperatureRawData& data) noexcept;

    /**
     * @brief Converts raw ADC counts to calibrated Celsius values and derived metrics (dT/dt).
     * @param raw Input raw data (already populated via read_raw).
     * @param previous Previous processed reading, used to compute dT/dt.
     * @param dt_seconds Time elapsed since the previous reading, in seconds.
     * @param out Output processed data container to be filled.
     */
    void convert_to_celsius(const bms::modules::drivers::TemperatureRawData& raw,
                            const bms::modules::drivers::TemperatureProcessedData& previous,
                            float dt_seconds,
                            bms::modules::drivers::TemperatureProcessedData& out) noexcept;

private:
    // TODO: measure read time and noise, tune per voltage_adc.hpp's TODO guidance.
    static constexpr int kSamples = 8;

    // TODO: replace with pins::THERMISTOR_CHANNELS from firmware/config/pins.hpp once it exists.
    static constexpr adc_channel_t kChannels[types::MAX_THERMISTORS] = {
        ADC_CHANNEL_1, ADC_CHANNEL_2, ADC_CHANNEL_3, ADC_CHANNEL_4,
        ADC_CHANNEL_5, ADC_CHANNEL_6, ADC_CHANNEL_7, ADC_CHANNEL_8};

    // TODO: NTC Beta-equation constants belong in firmware/config/thresholds.hpp once it exists.
    static constexpr float kBetaCoefficient = 3950.0f;
    static constexpr float kReferenceTempC = 25.0f;
    static constexpr float kReferenceResistanceOhm = 10000.0f;

    bool is_initialized_{false};
    adc_oneshot_unit_handle_t adc_handle_{nullptr};
};

}  // namespace bms::firmware::drivers