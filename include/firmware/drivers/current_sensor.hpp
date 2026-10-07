// ============================================================================
// ARCHITECTURAL NOTE:
// This header declares the Current Sensor driver interface for the firmware layer.
// It interacts with the hardware/ESP-IDF ADC APIs and populates the pure 
// data container defined in modules/drivers/current_sensor_data.hpp.
// ============================================================================

#pragma once

#include <cstdint>
#include "esp_adc/adc_oneshot.h"
#include "modules/drivers/current_sensor_data.hpp"

namespace bms::firmware::drivers {

    class CurrentSensorDriver {
        public:
            CurrentSensorDriver() noexcept = default;
            ~CurrentSensorDriver() noexcept;

            // Sole owner of a hardware handle: no copy, no move.
            CurrentSensorDriver(const CurrentSensorDriver &) = delete;
            CurrentSensorDriver & operator=(const CurrentSensorDriver &) = delete;

            CurrentSensorDriver(CurrentSensorDriver&&) noexcept = delete;
            CurrentSensorDriver & operator=(CurrentSensorDriver &&) noexcept = delete;

            /**
             * @brief Initializes the ADC peripheral (ESP-IDF specific configuration).
             * @return true if initialization is successful, false otherwise.
             */
            // TODO: channel should come from firmware/config/pins.hpp once it exists.
            [[nodiscard]] bool init(adc_channel_t channel = ADC_CHANNEL_9) noexcept;

            /**
             * @brief Reads the current sample from the hardware and populates the raw data container.
             * @param data Reference to the CurrentSensorRawData pure data container to be filled.
             * @return true if read was successful, false if hardware failure occurred.
             */
            /// Reads one (averaged) sample. On failure, marks the container invalid.
            [[nodiscard]] bool read_current(bms::modules::drivers::CurrentSensorRawData &data) noexcept;

            /**
             * @brief Converts raw ADC counts to calibrated Amperes, including low-pass filtering.
             * @param raw Input raw data (already populated via read_current).
             * @param previous Previous processed reading, used for the low-pass filter.
             * @param out Output processed data container to be filled.
             */
            void convert_to_amperes(const bms::modules::drivers::CurrentSensorRawData &raw,
                                     const bms::modules::drivers::CurrentSensorProcessedData &previous,
                                     bms::modules::drivers::CurrentSensorProcessedData &out) noexcept;

        private:
            // Samples averaged per reading (reduces random noise by about sqrt(N)).
            // TODO: measure read time and noise, tune per voltage_adc.hpp's TODO guidance.
            static constexpr int kSamples = 8;

            // TODO: these calibration constants belong in firmware/config/thresholds.hpp.
            // ACS724LLCTR-050B-T is ±50A, ratiometric to 3.3V supply.
            static constexpr float kSensitivity_V_per_A = 3.3f / (2.0f * 50.0f);
            static constexpr float kZeroCurrentOffsetV = 3.3f / 2.0f;
            static constexpr float kLowPassAlpha = 0.2f;

            bool is_initialized_{false};
            adc_channel_t channel_{ADC_CHANNEL_9};
            adc_oneshot_unit_handle_t adc_handle_{nullptr};
    };

} // namespace bms::firmware::drivers