// ============================================================================
// ARCHITECTURAL IMPLEMENTATION:
// ContactorDriver implementation interacting with a GPIO output to command
// the main pack contactor, and reading back confirmed state.
// ============================================================================

#include "firmware/drivers/contactor.hpp"

#include "esp_log.h"

namespace {
constexpr const char* TAG = "ContactorDriver";
}

namespace bms::firmware::drivers {

ContactorDriver::~ContactorDriver() noexcept {
    is_initialized_ = false;
}

bool ContactorDriver::init(gpio_num_t pin) noexcept {
    if (is_initialized_) {
        return true;
    }

    gpio_config_t io_cfg{};
    io_cfg.pin_bit_mask = (1ULL << pin);
    io_cfg.mode = GPIO_MODE_OUTPUT;
    io_cfg.pull_down_en = GPIO_PULLDOWN_ENABLE;  // default OPEN (pack disconnected)
    io_cfg.intr_type = GPIO_INTR_DISABLE;

    if (gpio_config(&io_cfg) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure contactor GPIO");
        return false;
    }

    pin_ = pin;
    (void)gpio_set_level(pin_, 0);  // start OPEN
    is_initialized_ = true;
    ESP_LOGI(TAG, "Contactor driver initialized (OPEN) on pin %d", static_cast<int>(pin_));
    return true;
}

bool ContactorDriver::apply_command(
    const bms::modules::drivers::ContactorCommandedData& commanded) noexcept {
    if (!is_initialized_) {
        return false;
    }

    if (gpio_set_level(pin_, commanded.get_close_commanded() ? 1 : 0) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set contactor GPIO");
        return false;
    }
    return true;
}

bool ContactorDriver::read_feedback(const bms::modules::drivers::ContactorCommandedData& commanded,
                                    bms::modules::drivers::ContactorActualData& out) noexcept {
    if (!is_initialized_) {
        return false;
    }

    // NOTE: same limitation as bypass_mosfet.cpp — gpio_get_level() on an
    // output pin reads back the driven level, not independent hardware
    // confirmation. Real stuck-fault detection needs a separate feedback
    // input (e.g. an auxiliary contact on the contactor) — not yet wired.
    const bool level = (gpio_get_level(pin_) != 0);
    out.set_is_closed(level);
    out.set_is_stuck_fault(level != commanded.get_close_commanded());
    return true;
}

}  // namespace bms::firmware::drivers