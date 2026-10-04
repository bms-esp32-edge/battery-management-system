// ============================================================
// ARCHITECTURAL NOTE:
// These classes are strictly pure data containers ("nouns") with zero logic.
// Commanded/Actual split, same as other actuator data structs.
// Validation and trigger-condition logic are delegated to the upper
// firmware processing/driver implementation layer.
// Setters perform direct assignment without runtime validation overhead.
// ============================================================
#pragma once

#include <cstdint>
#include <type_traits>

namespace bms::modules::drivers {

class PyroFuseCommandedData {
private:
    bool trigger_commanded_{false};   // true = command the pyro fuse to fire
    bool armed_commanded_{false};     // true = command the pyro fuse into armed state
    uint32_t command_timestamp_{0U};  // When this command was last issued

public:
    constexpr PyroFuseCommandedData() noexcept = default;

    // Getters
    [[nodiscard]] constexpr bool get_trigger_commanded() const noexcept {
        return trigger_commanded_;
    }

    [[nodiscard]] constexpr bool get_armed_commanded() const noexcept { return armed_commanded_; }

    [[nodiscard]] constexpr uint32_t get_command_timestamp() const noexcept {
        return command_timestamp_;
    }

    // Setters
    constexpr void set_trigger_commanded(bool trigger) noexcept { trigger_commanded_ = trigger; }

    constexpr void set_armed_commanded(bool armed) noexcept { armed_commanded_ = armed; }

    constexpr void set_command_timestamp(uint32_t timestamp) noexcept {
        command_timestamp_ = timestamp;
    }
};

class PyroFuseActualData {
private:
    bool is_armed_{false};            // Hardware-confirmed: fuse is currently armed
    bool is_triggered_{false};        // Hardware-confirmed: fuse has fired
    uint32_t trigger_timestamp_{0U};  // Timestamp of the actual trigger event, if any
    bool is_continuity_ok_{false};    // Hardware self-check: fuse circuit continuity intact

public:
    constexpr PyroFuseActualData() noexcept = default;

    // Getters
    [[nodiscard]] constexpr bool get_is_armed() const noexcept { return is_armed_; }

    [[nodiscard]] constexpr bool get_is_triggered() const noexcept { return is_triggered_; }

    [[nodiscard]] constexpr uint32_t get_trigger_timestamp() const noexcept {
        return trigger_timestamp_;
    }

    [[nodiscard]] constexpr bool get_is_continuity_ok() const noexcept { return is_continuity_ok_; }

    // Setters
    constexpr void set_is_armed(bool armed) noexcept { is_armed_ = armed; }

    constexpr void set_is_triggered(bool triggered) noexcept { is_triggered_ = triggered; }

    constexpr void set_trigger_timestamp(uint32_t timestamp) noexcept {
        trigger_timestamp_ = timestamp;
    }

    constexpr void set_is_continuity_ok(bool ok) noexcept { is_continuity_ok_ = ok; }
};

// Compile-time safety assertions for hardware data layout mapping
static_assert(std::is_standard_layout_v<PyroFuseCommandedData>,
              "PyroFuseCommandedData must be standard layout");
static_assert(std::is_trivially_copyable_v<PyroFuseCommandedData>,
              "PyroFuseCommandedData must be trivially copyable");

static_assert(std::is_standard_layout_v<PyroFuseActualData>,
              "PyroFuseActualData must be standard layout");
static_assert(std::is_trivially_copyable_v<PyroFuseActualData>,
              "PyroFuseActualData must be trivially copyable");

}  // namespace bms::modules::drivers