// ============================================================================
// ARCHITECTURAL IMPLEMENTATION:
// CurrentSensorDriver implementation interacting with the hardware/ESP-IDF APIs
// and populating the CurrentSensorRawData container.
// ============================================================================

#include "firmware/drivers/current_sensor.hpp"

#include "esp_log.h"

namespace {
constexpr const char* TAG = "CurrentSensorDriver";
}

namespace bms::firmware::drivers {

CurrentSensorDriver::~CurrentSensorDriver() noexcept {
    if (adc_handle_ != nullptr) {
        (void)adc_oneshot_del_unit(adc_handle_);
        adc_handle_ = nullptr;
    }
    is_initialized_ = false;
}

bool CurrentSensorDriver::init(adc_channel_t channel) noexcept {
    if (is_initialized_) {
        return true;
    }

    adc_oneshot_unit_init_cfg_t unit_cfg{};
    unit_cfg.unit_id = ADC_UNIT_1;
    unit_cfg.ulp_mode = ADC_ULP_MODE_DISABLE;

    if (adc_oneshot_new_unit(&unit_cfg, &adc_handle_) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize ADC oneshot unit");
        adc_handle_ = nullptr;
        return false;
    }

    adc_oneshot_chan_cfg_t chan_cfg{};
    chan_cfg.atten = ADC_ATTEN_DB_12;
    chan_cfg.bitwidth = ADC_BITWIDTH_DEFAULT;

    if (adc_oneshot_config_channel(adc_handle_, channel, &chan_cfg) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure ADC channel");
        (void)adc_oneshot_del_unit(adc_handle_);
        adc_handle_ = nullptr;
        return false;
    }

    channel_ = channel;
    is_initialized_ = true;
    ESP_LOGI(TAG, "ADC1 channel %d initialized for current sensing", static_cast<int>(channel_));
    return true;
}

bool CurrentSensorDriver::read_current(bms::modules::drivers::CurrentSensorRawData& data) noexcept {
    if (!is_initialized_) {
        data.set_conversion_valid(false);
        return false;
    }

    int32_t sum = 0;
    for (int i = 0; i < kSamples; ++i) {
        int raw = 0;
        if (adc_oneshot_read(adc_handle_, channel_, &raw) != ESP_OK) {
            ESP_LOGE(TAG, "Failed to read raw ADC value");
            data.set_conversion_valid(false);
            return false;
        }
        sum += raw;
    }

    data.set_raw_count(static_cast<uint16_t>(sum / kSamples));
    data.set_conversion_valid(true);
    return true;
}

void CurrentSensorDriver::convert_to_amperes(
    const bms::modules::drivers::CurrentSensorRawData& raw,
    const bms::modules::drivers::CurrentSensorProcessedData& previous,
    bms::modules::drivers::CurrentSensorProcessedData& out) noexcept {
    constexpr float kAdcMaxCounts = 4095.0f;
    constexpr float kAdcMaxVoltage = 3.1f;

    const float measured_v =
        (static_cast<float>(raw.get_raw_count()) / kAdcMaxCounts) * kAdcMaxVoltage;
    const float instant_amps = (measured_v - kZeroCurrentOffsetV) / kSensitivity_V_per_A;

    out.set_current_amperes(instant_amps);

    const float filtered = (kLowPassAlpha * instant_amps) +
                           ((1.0f - kLowPassAlpha) * previous.get_filtered_current_amperes());
    out.set_filtered_current_amperes(filtered);
}

}  // namespace bms::firmware::drivers