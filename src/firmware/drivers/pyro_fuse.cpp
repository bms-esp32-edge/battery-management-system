// ============================================================================
// ARCHITECTURAL IMPLEMENTATION:
// PyroFuseDriver implementation interacting with GPIO outputs (arm/trigger)
// and an ADC continuity self-check input.
// ============================================================================

#include "firmware/drivers/pyro_fuse.hpp"

#include "esp_log.h"

namespace {
constexpr const char* TAG = "PyroFuseDriver";
}

namespace bms::firmware::drivers {

PyroFuseDriver::~PyroFuseDriver() noexcept {
    if (adc_handle_ != nullptr) {
        (void)adc_oneshot_del_unit(adc_handle_);
        adc_handle_ = nullptr;
    }
    is_initialized_ = false;
}

bool PyroFuseDriver::init() noexcept {
    if (is_initialized_) {
        return true;
    }

    gpio_config_t arm_cfg{};
    arm_cfg.pin_bit_mask = (1ULL << kArmPin);
    arm_cfg.mode = GPIO_MODE_OUTPUT;
    arm_cfg.pull_down_en = GPIO_PULLDOWN_ENABLE;  // default DISARMED
    arm_cfg.intr_type = GPIO_INTR_DISABLE;
    if (gpio_config(&arm_cfg) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure arm GPIO");
        return false;
    }

    gpio_config_t trigger_cfg{};
    trigger_cfg.pin_bit_mask = (1ULL << kTriggerPin);
    trigger_cfg.mode = GPIO_MODE_OUTPUT;
    trigger_cfg.pull_down_en = GPIO_PULLDOWN_ENABLE;  // default not triggered
    trigger_cfg.intr_type = GPIO_INTR_DISABLE;
    if (gpio_config(&trigger_cfg) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure trigger GPIO");
        return false;
    }

    (void)gpio_set_level(kArmPin, 0);
    (void)gpio_set_level(kTriggerPin, 0);

    adc_oneshot_unit_init_cfg_t unit_cfg{};
    unit_cfg.unit_id = ADC_UNIT_1;
    unit_cfg.ulp_mode = ADC_ULP_MODE_DISABLE;
    if (adc_oneshot_new_unit(&unit_cfg, &adc_handle_) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize continuity-check ADC unit");
        adc_handle_ = nullptr;
        return false;
    }

    adc_oneshot_chan_cfg_t chan_cfg{};
    chan_cfg.atten = ADC_ATTEN_DB_12;
    chan_cfg.bitwidth = ADC_BITWIDTH_DEFAULT;
    if (adc_oneshot_config_channel(adc_handle_, kContinuityChannel, &chan_cfg) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure continuity-check ADC channel");
        (void)adc_oneshot_del_unit(adc_handle_);
        adc_handle_ = nullptr;
        return false;
    }

    is_initialized_ = true;
    ESP_LOGI(TAG, "Pyro fuse driver initialized (DISARMED)");
    return true;
}

bool PyroFuseDriver::apply_command(
    const bms::modules::drivers::PyroFuseCommandedData& commanded) noexcept {
    if (!is_initialized_) {
        return false;
    }

    // Arm must be applied before trigger can take effect — hardware interlock
    // should also enforce this independently; this ordering is defense in depth.
    if (gpio_set_level(kArmPin, commanded.get_armed_commanded() ? 1 : 0) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set arm GPIO");
        return false;
    }

    if (gpio_set_level(kTriggerPin, commanded.get_trigger_commanded() ? 1 : 0) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set trigger GPIO");
        return false;
    }

    if (commanded.get_trigger_commanded()) {
        ESP_LOGE(TAG, "PYRO FUSE TRIGGER COMMAND APPLIED");  // always log at error level: this is
                                                             // irreversible
    }

    return true;
}

bool PyroFuseDriver::read_feedback(bms::modules::drivers::PyroFuseActualData& out) noexcept {
    if (!is_initialized_) {
        return false;
    }

    out.set_is_armed(gpio_get_level(kArmPin) != 0);
    out.set_is_triggered(gpio_get_level(kTriggerPin) != 0);

    int raw = 0;
    if (adc_oneshot_read(adc_handle_, kContinuityChannel, &raw) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read continuity-check ADC");
        out.set_is_continuity_ok(false);
        return false;
    }

    constexpr float kAdcMaxCounts = 4095.0f;
    constexpr float kAdcMaxVoltage = 3.1f;
    const float voltage = (static_cast<float>(raw) / kAdcMaxCounts) * kAdcMaxVoltage;
    out.set_is_continuity_ok(voltage >= kContinuityOkMinVoltage);

    return true;
}

}  // namespace bms::firmware::drivers