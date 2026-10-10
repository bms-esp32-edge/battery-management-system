// ============================================================================
// ARCHITECTURAL IMPLEMENTATION:
// LedsDriver implementation driving WS2812B addressable RGB LEDs via the
// ESP-IDF RMT peripheral. Renders per-cell pattern/brightness/color commands.
//
// NOTE: Full WS2812B RMT bit-timing encoding is non-trivial and is left as a
// TODO here — this implementation covers the driver's structure, animation
// phase tracking, and color/brightness composition, but the actual RMT
// symbol encoding (converting GRB bits to RMT pulse durations) needs to be
// filled in, ideally using ESP-IDF's led_strip component rather than hand-
// rolled RMT symbols.
// ============================================================================

#include "firmware/drivers/leds.hpp"

#include "esp_log.h"

namespace {
constexpr const char* TAG = "LedsDriver";
}

namespace bms::firmware::drivers {

LedsDriver::~LedsDriver() noexcept {
    if (rmt_channel_ != nullptr) {
        (void)rmt_disable(rmt_channel_);
        (void)rmt_del_channel(rmt_channel_);
        rmt_channel_ = nullptr;
    }
    is_initialized_ = false;
}

bool LedsDriver::init(gpio_num_t data_pin) noexcept {
    if (is_initialized_) {
        return true;
    }

    rmt_tx_channel_config_t tx_cfg{};
    tx_cfg.gpio_num = data_pin;
    tx_cfg.clk_src = RMT_CLK_SRC_DEFAULT;
    tx_cfg.resolution_hz = 10000000;  // 10 MHz, 0.1 us resolution (standard for WS2812B timing)
    tx_cfg.mem_block_symbols = 64;
    tx_cfg.trans_queue_depth = 4;

    if (rmt_new_tx_channel(&tx_cfg, &rmt_channel_) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create RMT TX channel");
        rmt_channel_ = nullptr;
        return false;
    }

    if (rmt_enable(rmt_channel_) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable RMT channel");
        (void)rmt_del_channel(rmt_channel_);
        rmt_channel_ = nullptr;
        return false;
    }

    data_pin_ = data_pin;
    is_initialized_ = true;
    ESP_LOGI(TAG, "LED driver initialized on pin %d", static_cast<int>(data_pin_));
    return true;
}

bool LedsDriver::update(const bms::modules::drivers::LedsCommandedData& commanded) noexcept {
    if (!is_initialized_) {
        return false;
    }

    advance_animation_phase();

    // TODO: compose final per-cell GRB bytes here, applying pattern_ (SOLID,
    // BREATHING, SLOW_BLINK, RAPID_STROBE, SYSTEM_TRIP) as a modulation of
    // brightness_pct_ and color_rgb_ based on animation_phase_, then encode
    // the resulting byte stream into RMT symbols and transmit via
    // rmt_transmit(). Left unimplemented pending the led_strip component
    // integration decision — see class-level NOTE above.
    (void)commanded;

    ESP_LOGW(TAG, "LedsDriver::update() not yet fully implemented — see TODO");
    return false;
}

void LedsDriver::advance_animation_phase() noexcept {
    animation_phase_++;
}

}  // namespace bms::firmware::drivers