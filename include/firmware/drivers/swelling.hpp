// ============================================================================
// ARCHITECTURAL NOTE:
// This header declares the Swelling (FSR) driver interface for the firmware
// layer. It interacts with the hardware/ESP-IDF ADC APIs across per-cell FSR
// channels and populates the pure data container defined in
// modules/drivers/swelling_data.hpp.
// ============================================================================

#pragma once

#include <cstdint>
#include "esp_adc/adc_oneshot.h"
#include "modules/drivers/swelling_data.hpp"
#include "modules/types/telemetry_types.hpp"

namespace bms::firmware::drivers {

    namespace types = bms::modules::types;

    class SwellingDriver {
        public:
            SwellingDriver() noexcept = default;
            ~SwellingDriver() noexcept;

            SwellingDriver(const SwellingDriver &) = delete;
            SwellingDriver & operator=(const SwellingDriver &) = delete;

            SwellingDriver(SwellingDriver&&) noexcept = delete;
            SwellingDriver & operator=(SwellingDriver &&) noexcept = delete;

            /**
             * @brief Initializes the ADC peripheral for all per-cell FSR channels.
             * @return true if initialization is successful, false otherwise.
             */
            // TODO: channel array should come from firmware/config/pins.hpp once it exists.
            [[nodiscard]] bool init() noexcept;

            /**
             * @brief Reads raw ADC counts for all per-cell FSR channels.
             * @param data Reference to the SwellingRawData pure data container to be filled.
             * @return true if read was successful on all channels, false if any hardware failure occurred.
             */
            [[nodiscard]] bool read_raw(bms::modules::drivers::SwellingRawData &data) noexcept;

            /**
             * @brief Converts raw ADC counts to calibrated pressure (kPa) and checks swelling thresholds.
             * @param raw Input raw data (already populated via read_raw).
             * @param out Output processed data container to be filled.
             */
            // TODO: swelling threshold (kPa) belongs in firmware/config/thresholds.hpp once it exists.
            void convert_to_pressure(const bms::modules::drivers::SwellingRawData &raw,
                                      bms::modules::drivers::SwellingProcessedData &out) noexcept;

        private:
            // TODO: measure read time and noise, tune per voltage_adc.hpp's TODO guidance.
            static constexpr int kSamples = 8;

            // TODO: replace with pins::SWELLING_CHANNELS from firmware/config/pins.hpp once it exists.
            static constexpr adc_channel_t kChannels[types::MAX_CELL_COUNT] = {
                ADC_CHANNEL_0, ADC_CHANNEL_1, ADC_CHANNEL_2, ADC_CHANNEL_3,
                ADC_CHANNEL_4, ADC_CHANNEL_5, ADC_CHANNEL_6, ADC_CHANNEL_7,
                ADC_CHANNEL_8, ADC_CHANNEL_9, ADC_CHANNEL_0, ADC_CHANNEL_1,
                ADC_CHANNEL_2, ADC_CHANNEL_3, ADC_CHANNEL_4, ADC_CHANNEL_5
            };

            // TODO: FSR force-to-pressure calibration constants belong in
            // firmware/config/thresholds.hpp once it exists. Placeholder linear mapping.
            static constexpr float kAdcMaxCounts = 4095.0f;
            static constexpr float kMaxPressureKpa = 100.0f;

            // TODO: swelling detection threshold — placeholder, needs validation.
            static constexpr float kSwellingThresholdKpa = 50.0f;

            bool is_initialized_{false};
            adc_oneshot_unit_handle_t adc_handle_{nullptr};
    };

} // namespace bms::firmware::drivers