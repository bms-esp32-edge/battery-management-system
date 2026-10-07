// ============================================================================
// ARCHITECTURAL NOTE:
// This header declares the Pyro Fuse driver interface for the firmware layer.
// Unlike sensor drivers (Raw/Processed), this is a safety-critical actuator
// driver using Commanded/Actual — it interacts with GPIO output (commanding
// arm/trigger) and GPIO/ADC input (reading back confirmed state and the
// pre-fault continuity self-check).
// ============================================================================

#pragma once

#include <cstdint>
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "modules/drivers/pyro_fuse_data.hpp"

namespace bms::firmware::drivers {

    class PyroFuseDriver {
        public:
            PyroFuseDriver() noexcept = default;
            ~PyroFuseDriver() noexcept;

            // Sole owner of hardware handles: no copy, no move.
            PyroFuseDriver(const PyroFuseDriver &) = delete;
            PyroFuseDriver & operator=(const PyroFuseDriver &) = delete;

            PyroFuseDriver(PyroFuseDriver&&) noexcept = delete;
            PyroFuseDriver & operator=(PyroFuseDriver &&) noexcept = delete;

            /**
             * @brief Initializes the GPIO outputs (arm/trigger) and the ADC channel used
             *        for the continuity self-check.
             * @return true if initialization is successful, false otherwise.
             */
            // TODO: pins/channel should come from firmware/config/pins.hpp once it exists.
            [[nodiscard]] bool init() noexcept;

            /**
             * @brief Applies the commanded arm/trigger state to the physical GPIO outputs.
             * @param commanded The commanded state to apply.
             * @return true if command was applied successfully, false on hardware failure.
             */
            // NOTE: trigger_commanded_ must only ever be set by validated upstream
            // safety logic (e.g. protection.cpp) — this driver applies whatever it is
            // given and does not itself decide when to fire.
            [[nodiscard]] bool apply_command(
                const bms::modules::drivers::PyroFuseCommandedData &commanded) noexcept;

            /**
             * @brief Reads back confirmed arm/trigger state and the continuity self-check result.
             * @param out Output actual-state data container to be filled.
             * @return true if read was successful, false on hardware failure.
             */
            [[nodiscard]] bool read_feedback(
                bms::modules::drivers::PyroFuseActualData &out) noexcept;

        private:
            // TODO: these pins/channel belong in firmware/config/pins.hpp once it exists.
            static constexpr gpio_num_t kArmPin = GPIO_NUM_19;
            static constexpr gpio_num_t kTriggerPin = GPIO_NUM_20;
            static constexpr adc_channel_t kContinuityChannel = ADC_CHANNEL_0;

            // TODO: continuity-OK voltage threshold belongs in
            // firmware/config/thresholds.hpp once it exists. Placeholder only.
            static constexpr float kContinuityOkMinVoltage = 1.0f;

            bool is_initialized_{false};
            adc_oneshot_unit_handle_t adc_handle_{nullptr};
    };

} // namespace bms::firmware::drivers