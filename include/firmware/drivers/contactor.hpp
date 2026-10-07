// ============================================================================
// ARCHITECTURAL NOTE:
// This header declares the Main Contactor driver interface for the firmware
// layer. Commanded/Actual pattern, single component (one contactor for the
// whole pack), unlike the per-cell array drivers.
// ============================================================================

#pragma once

#include <cstdint>
#include "driver/gpio.h"
#include "modules/drivers/contactor_data.hpp"

namespace bms::firmware::drivers {

    class ContactorDriver {
        public:
            ContactorDriver() noexcept = default;
            ~ContactorDriver() noexcept;

            ContactorDriver(const ContactorDriver &) = delete;
            ContactorDriver & operator=(const ContactorDriver &) = delete;

            ContactorDriver(ContactorDriver&&) noexcept = delete;
            ContactorDriver & operator=(ContactorDriver &&) noexcept = delete;

            /**
             * @brief Initializes the GPIO output for the main contactor coil driver.
             * @return true if initialization is successful, false otherwise.
             */
            // TODO: pin should come from firmware/config/pins.hpp once it exists.
            [[nodiscard]] bool init(gpio_num_t pin = GPIO_NUM_17) noexcept;

            /**
             * @brief Applies the commanded close/open state to the physical GPIO output.
             * @param commanded The commanded state to apply.
             * @return true if command was applied successfully, false on hardware failure.
             */
            [[nodiscard]] bool apply_command(
                const bms::modules::drivers::ContactorCommandedData &commanded) noexcept;

            /**
             * @brief Reads back confirmed contactor state and checks for a stuck/non-responsive contactor.
             * @param commanded The most recently applied commanded state, used to detect mismatches.
             * @param out Output actual-state data container to be filled.
             * @return true if read was successful, false on hardware failure.
             */
            [[nodiscard]] bool read_feedback(
                const bms::modules::drivers::ContactorCommandedData &commanded,
                bms::modules::drivers::ContactorActualData &out) noexcept;

        private:
            bool is_initialized_{false};
            gpio_num_t pin_{GPIO_NUM_17};
    };

} // namespace bms::firmware::drivers