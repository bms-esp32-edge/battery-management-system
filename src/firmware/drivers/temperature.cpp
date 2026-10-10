// ============================================================================
// ARCHITECTURAL IMPLEMENTATION:
// TemperatureDriver implementation interacting with the hardware/ESP-IDF APIs
// across all thermistor channels, and populating the TemperatureRawData
// container. Conversion to Celsius uses the NTC Beta-equation.
// ============================================================================

#include "firmware/drivers/temperature.hpp"

#include <cmath>

#include "esp_log.h"

namespace {
constexpr const char* TAG = "TemperatureDriver";
}

namespace bms::firmware::drivers {

TemperatureDriver::~TemperatureDriver() noexcept {
    if (adc_handle_ != nullptr) {
        (void)adc_oneshot_del_unit(adc_handle_);
        adc_handle_ = nullptr;
    }
    is_initialized_ = false;
}

bool TemperatureDriver::init() noexcept {
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

    for (size_t i = 0; i < types::MAX_THERMISTORS; ++i) {
        if (adc_oneshot_config_channel(adc_handle_, kChannels[i], &chan_cfg) != ESP_OK) {
            ESP_LOGE(TAG, "Failed to configure thermistor channel %zu", i);
            (void)adc_oneshot_del_unit(adc_handle_);
            adc_handle_ = nullptr;
            return false;
        }
    }

    is_initialized_ = true;
    ESP_LOGI(TAG, "All %d thermistor channels initialized", types::MAX_THERMISTORS);
    return true;
}

bool TemperatureDriver::read_raw(bms::modules::drivers::TemperatureRawData& data) noexcept {
    if (!is_initialized_) {
        data.set_conversion_valid(false);
        return false;
    }

    std::array<uint16_t, types::MAX_THERMISTORS> counts{};

    for (size_t ch = 0; ch < types::MAX_THERMISTORS; ++ch) {
        int32_t sum = 0;
        for (int i = 0; i < kSamples; ++i) {
            int raw = 0;
            if (adc_oneshot_read(adc_handle_, kChannels[ch], &raw) != ESP_OK) {
                ESP_LOGE(TAG, "Failed to read thermistor channel %zu", ch);
                data.set_conversion_valid(false);
                return false;
            }
            sum += raw;
        }
        counts[ch] = static_cast<uint16_t>(sum / kSamples);
    }

    data.set_raw_counts(counts);
    data.set_conversion_valid(true);
    return true;
}

void TemperatureDriver::convert_to_celsius(
    const bms::modules::drivers::TemperatureRawData& raw,
    const bms::modules::drivers::TemperatureProcessedData& previous, float dt_seconds,
    bms::modules::drivers::TemperatureProcessedData& out) noexcept {
    constexpr float kAdcMaxCounts = 4095.0f;
    constexpr float kAdcMaxVoltage = 3.1f;
    constexpr float kKelvinOffset = 273.15f;
    constexpr float kSeriesResistorOhm = 10000.0f;  // voltage-divider pull-up resistor

    std::array<float, types::MAX_THERMISTORS> temps_c{};
    float max_temp = -1000.0f;

    const auto& counts = raw.get_raw_counts();
    for (size_t i = 0; i < types::MAX_THERMISTORS; ++i) {
        const float voltage = (static_cast<float>(counts[i]) / kAdcMaxCounts) * kAdcMaxVoltage;

        // Voltage divider: thermistor_resistance = series_r * (V / (Vmax - V))
        const float thermistor_r = kSeriesResistorOhm * (voltage / (kAdcMaxVoltage - voltage));

        // NTC Beta equation: 1/T = 1/T0 + (1/Beta) * ln(R/R0)
        const float ref_temp_k = kReferenceTempC + kKelvinOffset;
        const float inv_t =
            (1.0f / ref_temp_k) +
            (1.0f / kBetaCoefficient) * std::log(thermistor_r / kReferenceResistanceOhm);
        const float temp_c = (1.0f / inv_t) - kKelvinOffset;

        temps_c[i] = temp_c;
        if (temp_c > max_temp) {
            max_temp = temp_c;
        }
    }

    out.set_cell_temps_c(temps_c);
    out.set_max_cell_temp_c(max_temp);

    // dT/dt — guard against division by zero/garbage on the very first call.
    if (dt_seconds > 0.0f) {
        const float rate = (max_temp - previous.get_max_cell_temp_c()) / dt_seconds;
        out.set_thermal_rise_rate_c_s(rate);
    } else {
        out.set_thermal_rise_rate_c_s(0.0f);
    }
}

}  // namespace bms::firmware::drivers