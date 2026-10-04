// ============================================================
// ARCHITECTURAL NOTE:
// These classes are strictly pure data containers ("nouns") with zero logic.
// Pattern selection and color/brightness calculation are delegated to the
// upper firmware processing/driver implementation layer.
// Setters perform direct assignment without runtime validation overhead.
//
// LedPattern is defined locally here (not in system_enums.hpp) since it is
// purely a visual/display concern, not a core system or cell state.
// ============================================================
#pragma once

#include <array>
#include <cstdint>
#include <type_traits>

#include "modules/types/telemetry_types.hpp"

namespace bms::modules::drivers {

namespace types = bms::modules::types;

/**
 * @brief Visual display pattern for a per-cell status LED.
 */
enum class LedPattern : uint8_t {
    OFF = 0U,
    SOLID = 1U,
    BREATHING = 2U,     // Smooth pulse during charging
    SLOW_BLINK = 3U,    // Overvoltage/undervoltage warning
    RAPID_STROBE = 4U,  // Swelling / mechanical fault
    SYSTEM_TRIP = 5U    // Complete shutoff on pyro fuse detonation
};

class LedsCommandedData {
private:
    std::array<LedPattern, types::MAX_CELL_COUNT> pattern_{};      // Per-cell: pattern to display
    std::array<uint8_t, types::MAX_CELL_COUNT> brightness_pct_{};  // Per-cell: brightness 0-100%
    std::array<uint32_t, types::MAX_CELL_COUNT>
        color_rgb_{};  // Per-cell: packed RGB color value (e.g. 0xFF0000 = red)

public:
    constexpr LedsCommandedData() noexcept = default;

    // Getters
    [[nodiscard]] constexpr const std::array<LedPattern, types::MAX_CELL_COUNT>& get_pattern()
        const noexcept {
        return pattern_;
    }

    [[nodiscard]] constexpr const std::array<uint8_t, types::MAX_CELL_COUNT>& get_brightness_pct()
        const noexcept {
        return brightness_pct_;
    }

    [[nodiscard]] constexpr const std::array<uint32_t, types::MAX_CELL_COUNT>& get_color_rgb()
        const noexcept {
        return color_rgb_;
    }

    // Setters
    constexpr void set_pattern(
        const std::array<LedPattern, types::MAX_CELL_COUNT>& pattern) noexcept {
        pattern_ = pattern;
    }

    constexpr void set_brightness_pct(
        const std::array<uint8_t, types::MAX_CELL_COUNT>& brightness) noexcept {
        brightness_pct_ = brightness;
    }

    constexpr void set_color_rgb(
        const std::array<uint32_t, types::MAX_CELL_COUNT>& color) noexcept {
        color_rgb_ = color;
    }
};

// Compile-time safety assertions for hardware data layout mapping
static_assert(std::is_standard_layout_v<LedsCommandedData>,
              "LedsCommandedData must be standard layout");
static_assert(std::is_trivially_copyable_v<LedsCommandedData>,
              "LedsCommandedData must be trivially copyable");

}  // namespace bms::modules::drivers