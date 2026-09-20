/*
 * @file safety_keys.hpp
 * @brief Common types, enums, fault bitmasks, and telemetry definitions for the BMS.
 * @author Vedant Salunkhe (vedaaasalunkhe@gmail.com)
 *
 * Standards Compliance:
 * - C++17 compliant (-std=c++17)
 * - MISRA C++:2008 oriented (explicit fixed-width integers, scoped enums, no exceptions)
 * - Zero dynamic memory allocations
 * - Safe for cross-task/ISR synchronization via critical sections
 */

#pragma once

#include <cstdint>
#include <type_traits>

namespace bms::modules::types {

/* ============================================================================
 * Pyrotechnic Actuation Guard Key
 * ============================================================================ */

/**
 * @brief Two-stage guarded key pattern to prevent accidental pyro-fuse firing.
 *
 * Requires high Hamming-distance magic words and a bounded confirmation time window.
 *
 * @note PRECONDITION: The caller MUST gate invocation of arm() behind an independent
 *       hardware fault confirmation (e.g. analog comparator LM393 trip + software validation),
 *       not software state machine variables alone.
 */
struct PyroTriggerKey {
    /// Alternating bit pattern (Hamming distance = 16)
    static constexpr uint32_t ARM_MAGIC = 0x5A5AA5A5U;

    /// Orthogonal bit pattern (Hamming distance = 16)
    static constexpr uint32_t FIRE_MAGIC = 0xC3C33C3CU;

    /// 50ms maximum time-to-fire window after arming
    static constexpr uint32_t ARM_WINDOW_TIMEOUT_MS = 50U;

    uint32_t arm_key{0U};
    uint32_t fire_key{0U};
    uint32_t armed_at_ms{0U};
    uint32_t sequence_id{0U};

    /**
     * @brief Arms the pyrotechnic trigger.
     * @param timestamp_ms Current monotonic system timestamp in milliseconds.
     * @param seq_id Monotonic sequence counter.
     */
    constexpr void arm(uint32_t timestamp_ms, uint32_t seq_id = 0U) noexcept {
        arm_key = ARM_MAGIC;
        fire_key = 0U;
        armed_at_ms = timestamp_ms;
        sequence_id = seq_id;
    }

    /**
     * @brief Clears all arming and firing tokens, invalidating any pending actuation.
     */
    constexpr void disarm() noexcept {
        arm_key = 0U;
        fire_key = 0U;
        armed_at_ms = 0U;
        sequence_id = 0U;
    }

    /**
     * @brief Checks if the trigger is in the armed state.
     */
    [[nodiscard]] constexpr bool is_armed() const noexcept { return arm_key == ARM_MAGIC; }

    /**
     * @brief Evaluates whether firing is authorized within the active time window.
     * @param current_time_ms Current monotonic system timestamp in milliseconds.
     * @return true if both keys are valid and the armed time window has not expired.
     */
    [[nodiscard]] constexpr bool is_fire_authorized(uint32_t current_time_ms) const noexcept {
        if (arm_key != ARM_MAGIC || fire_key != FIRE_MAGIC) {
            return false;
        }
        if (current_time_ms < armed_at_ms) {
            return false;  // Clock reversal / underflow guard
        }
        return (current_time_ms - armed_at_ms) <= ARM_WINDOW_TIMEOUT_MS;
    }
};

/* ============================================================================
 *  Compile-Time Invariants & Static Assertions
 * ============================================================================ */

static_assert(std::is_standard_layout_v<PyroTriggerKey>,
              "PyroTriggerKey must have standard layout");
static_assert(std::is_trivially_copyable_v<PyroTriggerKey>,
              "PyroTriggerKey must be trivially copyable");

}  // namespace bms::modules::types