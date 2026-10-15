// ============================================================================
// ARCHITECTURAL IMPLEMENTATION:
// SwellingDriver implementation interacting with the hardware/ESP-IDF APIs
// across all per-cell FSR channels, and populating the SwellingRawData
// container.
// ============================================================================

#include "firmware/drivers/swelling.hpp"

#include "esp_log.h"

namespace {
constexpr const char* TAG = "SwellingDriver";
}

namespace bms::firmware::drivers {

SwellingDriver::~SwellingDriver() noexcept {
    if (adc_handle_ != nullptr) {
        (void)adc_oneshot_del_unit(adc_handle_);
        adc_handle_ = nullptr;
    }
    is_initialized_ = false;
}

bool SwellingDriver::init() noexcept {
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

    // NOTE: kChannels currently repeats ADC0-9 (only 10 distinct ADC1 channels
    // exist on ESP32-S3) to fill 16 cell slots. This is a known placeholder —
    // real hardware will need a multiplexer/expander. See .hpp TODO.
    for (size_t i = 0; i < types::MAX_CELL_COUNT; ++i) {
        if (adc_oneshot_config_channel(adc_handle_, kChannels[i], &chan_cfg) != ESP_OK) {
            ESP_LOGE(TAG, "Failed to configure swelling channel %zu", i);
            (void)adc_oneshot_del_unit(adc_handle_);
            adc_handle_ = nullptr;
            return false;
        }
    }

    is_initialized_ = true;
    ESP_LOGI(TAG, "All %d swelling channels initialized", types::MAX_CELL_COUNT);
    return true;
}

bool SwellingDriver::read_raw(bms::modules::drivers::SwellingRawData& data) noexcept {
    if (!is_initialized_) {
        data.set_conversion_valid(false);
        return false;
    }

    std::array<uint16_t, types::MAX_CELL_COUNT> counts{};

    for (size_t ch = 0; ch < types::MAX_CELL_COUNT; ++ch) {
        int32_t sum = 0;
        for (int i = 0; i < kSamples; ++i) {
            int raw = 0;
            if (adc_oneshot_read(adc_handle_, kChannels[ch], &raw) != ESP_OK) {
                ESP_LOGE(TAG, "Failed to read swelling channel %zu", ch);
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

void SwellingDriver::convert_to_pressure(
    const bms::modules::drivers::SwellingRawData& raw,
    bms::modules::drivers::SwellingProcessedData& out) noexcept {
    std::array<float, types::MAX_CELL_COUNT> pressures{};
    std::array<bool, types::MAX_CELL_COUNT> detected{};

    const auto& counts = raw.get_raw_counts();
    for (size_t i = 0; i < types::MAX_CELL_COUNT; ++i) {
        // TODO: placeholder linear mapping. Real FSR402 force-to-resistance curve
        // is non-linear; needs calibration against real sensor data.
        const float pressure_kpa =
            (static_cast<float>(counts[i]) / kAdcMaxCounts) * kMaxPressureKpa;
        pressures[i] = pressure_kpa;
        detected[i] = (pressure_kpa >= kSwellingThresholdKpa);
    }

    out.set_cell_pressure_kpa(pressures);
    out.set_is_swelling_detected(detected);
}

}  // namespace bms::firmware::drivers