// ============================================================================
// ARCHITECTURAL NOTE:
// This header declares the Per-Cell Status LED driver interface for the
// firmware layer. Drives WS2812B addressable RGB LEDs via the ESP-IDF RMT
// peripheral. Commanded-only (no hardware feedback exists for LED state).
// ============================================================================

#pragma once

#include <cstdint>

#include "driver/rmt_tx.h"
#include "modules/drivers/leds_data.hpp"
#include "modules/types/telemetry_types.hpp"

namespace bms::firmware::drivers {

namespace types = bms::modules::types;

class LedsDriver {
public:
    LedsDriver() noexcept = default;
    ~LedsDriver() noexcept;

    LedsDriver(const LedsDriver&) = delete;
    LedsDriver& operator=(const LedsDriver&) = delete;

    LedsDriver(LedsDriver&&) noexcept = delete;
    LedsDriver& operator=(LedsDriver&&) noexcept = delete;

    /**
     * @brief Initializes the RMT peripheral for driving the WS2812B LED strip.
     * @return true if initialization is successful, false otherwise.
     */
    // TODO: data pin should come from firmware/config/pins.hpp once it exists.
    [[nodiscard]] bool init(gpio_num_t data_pin = GPIO_NUM_18) noexcept;

    /**
     * @brief Pushes the commanded per-cell pattern/brightness/color out to the LED strip.
     * @param commanded The commanded state to render.
     * @return true if the update was transmitted successfully, false on hardware failure.
     */
    [[nodiscard]] bool update(const bms::modules::drivers::LedsCommandedData& commanded) noexcept;

private:
    // Called internally by update() each frame to advance BREATHING/STROBE
    // animation phase without needing external timing input.
    void advance_animation_phase() noexcept;

    bool is_initialized_{false};
    gpio_num_t data_pin_{GPIO_NUM_18};
    rmt_channel_handle_t rmt_channel_{nullptr};
    uint32_t animation_phase_{0U};
};

}  // namespace bms::firmware::drivers