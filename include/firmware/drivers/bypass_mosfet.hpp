// ============================================================================
// ARCHITECTURAL NOTE:
// This header declares the Bypass MOSFET driver interface for the firmware
// layer. Unlike sensor drivers (Raw/Processed), this is an actuator driver
// using Commanded/Actual — it interacts with GPIO output (commanding bypass
// switches) and GPIO/ADC input (reading back confirmed state).
// ============================================================================

#pragma once

#include <cstdint>

#include "driver/gpio.h"
#include "modules/drivers/bypass_mosfet_data.hpp"
#include "modules/types/telemetry_types.hpp"

namespace bms::firmware::drivers {

namespace types = bms::modules::types;

class BypassMosfetDriver {
public:
    BypassMosfetDriver() noexcept = default;
    ~BypassMosfetDriver() noexcept;

    BypassMosfetDriver(const BypassMosfetDriver&) = delete;
    BypassMosfetDriver& operator=(const BypassMosfetDriver&) = delete;

    BypassMosfetDriver(BypassMosfetDriver&&) noexcept = delete;
    BypassMosfetDriver& operator=(BypassMosfetDriver&&) noexcept = delete;

    /**
     * @brief Initializes GPIO outputs for all per-cell bypass MOSFET gate drivers.
     * @return true if initialization is successful, false otherwise.
     */
    // TODO: pin array should come from firmware/config/pins.hpp once it exists.
    [[nodiscard]] bool init() noexcept;

    /**
     * @brief Applies the commanded bypass state to the physical GPIO outputs.
     * @param commanded The commanded state to apply.
     * @return true if command was applied successfully, false on hardware failure.
     */
    [[nodiscard]] bool apply_command(
        const bms::modules::drivers::BypassMosfetCommandedData& commanded) noexcept;

    /**
     * @brief Reads back confirmed bypass state and checks for stuck/non-responsive switches.
     * @param commanded The most recently applied commanded state, used to detect mismatches.
     * @param out Output actual-state data container to be filled.
     * @return true if read was successful, false on hardware failure.
     */
    [[nodiscard]] bool read_feedback(
        const bms::modules::drivers::BypassMosfetCommandedData& commanded,
        bms::modules::drivers::BypassMosfetActualData& out) noexcept;

private:
    // TODO: replace with pins::BYPASS_MOSFET_PINS from firmware/config/pins.hpp once it exists.
    static constexpr gpio_num_t kPins[types::MAX_CELL_COUNT] = {
        GPIO_NUM_1,  GPIO_NUM_2,  GPIO_NUM_3,  GPIO_NUM_4,  GPIO_NUM_5,  GPIO_NUM_6,
        GPIO_NUM_7,  GPIO_NUM_8,  GPIO_NUM_9,  GPIO_NUM_10, GPIO_NUM_11, GPIO_NUM_12,
        GPIO_NUM_13, GPIO_NUM_14, GPIO_NUM_15, GPIO_NUM_16};

    bool is_initialized_{false};
};

}  // namespace bms::firmware::drivers