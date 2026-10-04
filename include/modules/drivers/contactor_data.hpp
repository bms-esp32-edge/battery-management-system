// ============================================================
// ARCHITECTURAL NOTE:
// These classes are strictly pure data containers ("nouns") with zero logic.
// Commanded/Actual split, same as other actuator data structs.
// Validation and control logic are delegated to the upper
// firmware processing/driver implementation layer.
// Setters perform direct assignment without runtime validation overhead.
// ============================================================
#pragma once

#include <cstdint>
#include <type_traits>

namespace bms::modules::drivers {

class ContactorCommandedData {
private:
    bool close_commanded_{false};     // true = command the main contactor to CLOSE (connect pack)
    uint32_t command_timestamp_{0U};  // When this command was last issued

public:
    constexpr ContactorCommandedData() noexcept = default;

    // Getters
    [[nodiscard]] constexpr bool get_close_commanded() const noexcept { return close_commanded_; }

    [[nodiscard]] constexpr uint32_t get_command_timestamp() const noexcept {
        return command_timestamp_;
    }

    // Setters
    constexpr void set_close_commanded(bool close) noexcept { close_commanded_ = close; }

    constexpr void set_command_timestamp(uint32_t timestamp) noexcept {
        command_timestamp_ = timestamp;
    }
};

class ContactorActualData {
private:
    bool is_closed_{false};            // Hardware-confirmed: contactor is currently closed
    bool is_stuck_fault_{false};       // true = contactor not responding to commands
    uint32_t feedback_timestamp_{0U};  // When this feedback was last read

public:
    constexpr ContactorActualData() noexcept = default;

    // Getters
    [[nodiscard]] constexpr bool get_is_closed() const noexcept { return is_closed_; }

    [[nodiscard]] constexpr bool get_is_stuck_fault() const noexcept { return is_stuck_fault_; }

    [[nodiscard]] constexpr uint32_t get_feedback_timestamp() const noexcept {
        return feedback_timestamp_;
    }

    // Setters
    constexpr void set_is_closed(bool closed) noexcept { is_closed_ = closed; }

    constexpr void set_is_stuck_fault(bool stuck) noexcept { is_stuck_fault_ = stuck; }

    constexpr void set_feedback_timestamp(uint32_t timestamp) noexcept {
        feedback_timestamp_ = timestamp;
    }
};

// Compile-time safety assertions for hardware data layout mapping
static_assert(std::is_standard_layout_v<ContactorCommandedData>,
              "ContactorCommandedData must be standard layout");
static_assert(std::is_trivially_copyable_v<ContactorCommandedData>,
              "ContactorCommandedData must be trivially copyable");

static_assert(std::is_standard_layout_v<ContactorActualData>,
              "ContactorActualData must be standard layout");
static_assert(std::is_trivially_copyable_v<ContactorActualData>,
              "ContactorActualData must be trivially copyable");

}  // namespace bms::modules::drivers