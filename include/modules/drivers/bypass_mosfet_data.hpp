// ============================================================
// ARCHITECTURAL NOTE:
// These classes are strictly pure data containers ("nouns") with zero logic.
// Unlike sensor data (Raw/Processed), actuator data uses Commanded/Actual —
// what we're telling the hardware to do vs. what it's confirmed to be doing.
// Validation and control logic are delegated to the upper firmware layer.
// Setters perform direct assignment without runtime validation overhead.
// ============================================================
#pragma once

#include <array>
#include <cstdint>
#include <type_traits>

#include "modules/types/telemetry_types.hpp"

namespace bms::modules::drivers {

namespace types = bms::modules::types;

class BypassMosfetCommandedData {
private:
    std::array<bool, types::MAX_CELL_COUNT>
        bypass_commanded_{};          // Per-cell: true = command this cell's bypass ON
    uint32_t command_timestamp_{0U};  // When this command set was last issued

public:
    constexpr BypassMosfetCommandedData() noexcept = default;

    // Getters
    [[nodiscard]] constexpr const std::array<bool, types::MAX_CELL_COUNT>& get_bypass_commanded()
        const noexcept {
        return bypass_commanded_;
    }

    [[nodiscard]] constexpr uint32_t get_command_timestamp() const noexcept {
        return command_timestamp_;
    }

    // Setters
    constexpr void set_bypass_commanded(
        const std::array<bool, types::MAX_CELL_COUNT>& commanded) noexcept {
        bypass_commanded_ = commanded;
    }

    constexpr void set_command_timestamp(uint32_t timestamp) noexcept {
        command_timestamp_ = timestamp;
    }
};

class BypassMosfetActualData {
private:
    std::array<bool, types::MAX_CELL_COUNT>
        bypass_confirmed_{};  // Per-cell: true = hardware confirms bypass is actually ON
    std::array<bool, types::MAX_CELL_COUNT>
        is_stuck_fault_{};             // Per-cell: true = switch not responding to commands
    uint32_t feedback_timestamp_{0U};  // When this feedback was last read

public:
    constexpr BypassMosfetActualData() noexcept = default;

    // Getters
    [[nodiscard]] constexpr const std::array<bool, types::MAX_CELL_COUNT>& get_bypass_confirmed()
        const noexcept {
        return bypass_confirmed_;
    }

    [[nodiscard]] constexpr const std::array<bool, types::MAX_CELL_COUNT>& get_is_stuck_fault()
        const noexcept {
        return is_stuck_fault_;
    }

    [[nodiscard]] constexpr uint32_t get_feedback_timestamp() const noexcept {
        return feedback_timestamp_;
    }

    // Setters
    constexpr void set_bypass_confirmed(
        const std::array<bool, types::MAX_CELL_COUNT>& confirmed) noexcept {
        bypass_confirmed_ = confirmed;
    }

    constexpr void set_is_stuck_fault(
        const std::array<bool, types::MAX_CELL_COUNT>& stuck) noexcept {
        is_stuck_fault_ = stuck;
    }

    constexpr void set_feedback_timestamp(uint32_t timestamp) noexcept {
        feedback_timestamp_ = timestamp;
    }
};

// Compile-time safety assertions for hardware data layout mapping
static_assert(std::is_standard_layout_v<BypassMosfetCommandedData>,
              "BypassMosfetCommandedData must be standard layout");
static_assert(std::is_trivially_copyable_v<BypassMosfetCommandedData>,
              "BypassMosfetCommandedData must be trivially copyable");

static_assert(std::is_standard_layout_v<BypassMosfetActualData>,
              "BypassMosfetActualData must be standard layout");
static_assert(std::is_trivially_copyable_v<BypassMosfetActualData>,
              "BypassMosfetActualData must be trivially copyable");

}  // namespace bms::modules::drivers