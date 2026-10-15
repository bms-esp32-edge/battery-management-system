// ============================================================================
// ARCHITECTURAL IMPLEMENTATION:
// VoltageAdcDriver implementation interacting with the hardware/ESP-IDF APIs
// and populating the VoltageAdcRawData container.
// ============================================================================

#include "firmware/drivers/voltage_adc.hpp"

#include "esp_log.h"

namespace {
constexpr const char* TAG = "VoltageAdcDriver";
}

namespace bms::firmware::drivers {

VoltageAdcDriver::~VoltageAdcDriver() noexcept {
    if (adc_handle_ != nullptr) {
        (void)adc_oneshot_del_unit(adc_handle_);
        adc_handle_ = nullptr;
    }
    is_initialized_ = false;
}

bool VoltageAdcDriver::init(adc_channel_t channel) noexcept {
    if (is_initialized_) {
        return true;
    }

    adc_oneshot_unit_init_cfg_t unit_cfg{};
    unit_cfg.unit_id = ADC_UNIT_1;  // ADC2 conflicts with Wi-Fi
    unit_cfg.ulp_mode = ADC_ULP_MODE_DISABLE;

    if (adc_oneshot_new_unit(&unit_cfg, &adc_handle_) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize ADC oneshot unit");
        adc_handle_ = nullptr;
        return false;
    }

    adc_oneshot_chan_cfg_t chan_cfg{};
    chan_cfg.atten = ADC_ATTEN_DB_12;  // ~0-3.1 V usable; never exceed 3.3 V at the pin
    chan_cfg.bitwidth = ADC_BITWIDTH_DEFAULT;

    if (adc_oneshot_config_channel(adc_handle_, channel, &chan_cfg) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure ADC channel");
        (void)adc_oneshot_del_unit(adc_handle_);
        adc_handle_ = nullptr;
        return false;
    }

    channel_ = channel;
    is_initialized_ = true;
    ESP_LOGI(TAG, "ADC1 channel %d initialized", static_cast<int>(channel_));
    return true;
}

bool VoltageAdcDriver::read_voltage(bms::modules::drivers::VoltageAdcRawData& data) noexcept {
    if (!is_initialized_) {
        data.set_conversion_valid(false);
        return false;
    }

    int32_t sum = 0;
    for (int i = 0; i < kSamples; ++i) {
        int raw = 0;
        if (adc_oneshot_read(adc_handle_, channel_, &raw) != ESP_OK) {
            ESP_LOGE(TAG, "Failed to read raw ADC value");
            data.set_conversion_valid(false);  // never leave stale "valid" data
            return false;
        }
        sum += raw;
    }

    auto counts = data.get_raw_counts();
    counts[0] = static_cast<uint16_t>(sum / kSamples);
    data.set_raw_counts(counts);
    data.set_conversion_valid(true);
    return true;
}

}  // namespace bms::firmware::drivers