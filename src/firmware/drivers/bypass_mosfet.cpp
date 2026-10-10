// ============================================================================
// ARCHITECTURAL IMPLEMENTATION:
// BypassMosfetDriver implementation interacting with GPIO outputs to command
// per-cell bypass switches, and reading back confirmed state.
// ============================================================================

#include "firmware/drivers/bypass_mosfet.hpp"

#include "esp_log.h"

namespace {
constexpr const char* TAG = "BypassMosfetDriver";
}

namespace bms::firmware::drivers {

BypassMosfetDriver::~BypassMosfetDriver() noexcept {
    is_initialized_ = false;
}

bool BypassMosfetDriver::init() noexcept {
    if (is_initialized_) {
        return true;
    }

    for (size_t i = 0; i < types::MAX_CELL_COUNT; ++i) {
        gpio_config_t io_cfg{};
        io_cfg.pin_bit_mask = (1ULL << kPins[i]);
        io_cfg.mode = GPIO_MODE_OUTPUT;
        io_cfg.pull_up_en = GPIO_PULLUP_DISABLE;
        io_cfg.pull_down_en = GPIO_PULLDOWN_ENABLE;  // default OFF (bypass not active)
        io_cfg.intr_type = GPIO_INTR_DISABLE;

        if (gpio_config(&io_cfg) != ESP_OK) {
            ESP_LOGE(TAG, "Failed to configure bypass GPIO for cell %zu", i);
            return false;
        }
        (void)gpio_set_level(kPins[i], 0);  // start all bypass switches OFF
    }

    is_initialized_ = true;
    ESP_LOGI(TAG, "All %d bypass MOSFET GPIOs initialized", types::MAX_CELL_COUNT);
    return true;
}

bool BypassMosfetDriver::apply_command(
    const bms::modules::drivers::BypassMosfetCommandedData& commanded) noexcept {
    if (!is_initialized_) {
        return false;
    }

    const auto& commands = commanded.get_bypass_commanded();
    for (size_t i = 0; i < types::MAX_CELL_COUNT; ++i) {
        if (gpio_set_level(kPins[i], commands[i] ? 1 : 0) != ESP_OK) {
            ESP_LOGE(TAG, "Failed to set bypass GPIO for cell %zu", i);
            return false;
        }
    }
    return true;
}

bool BypassMosfetDriver::read_feedback(
    const bms::modules::drivers::BypassMosfetCommandedData& commanded,
    bms::modules::drivers::BypassMosfetActualData& out) noexcept {
    if (!is_initialized_) {
        return false;
    }

    // NOTE: gpio_get_level() on an output pin reads back the driven level, not
    // independent hardware confirmation. True stuck-fault detection requires a
    // separate feedback input (e.g. a sense pin reading the MOSFET drain) —
    // not yet wired. This is a placeholder until that circuit exists.
    std::array<bool, types::MAX_CELL_COUNT> confirmed{};
    std::array<bool, types::MAX_CELL_COUNT> stuck{};

    const auto& commands = commanded.get_bypass_commanded();
    for (size_t i = 0; i < types::MAX_CELL_COUNT; ++i) {
        const bool level = (gpio_get_level(kPins[i]) != 0);
        confirmed[i] = level;
        stuck[i] = (level != commands[i]);  // mismatch vs. last commanded state
    }

    out.set_bypass_confirmed(confirmed);
    out.set_is_stuck_fault(stuck);
    return true;
}

}  // namespace bms::firmware::drivers