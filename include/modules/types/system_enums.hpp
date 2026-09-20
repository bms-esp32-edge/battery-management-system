/*
 * @file system_enums.hpp
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

namespace bms::modules::types {

/* ============================================================================
 * High-Level System State Enum
 * ============================================================================ */

/**
 * @brief Governs high-level system execution state.
 */
enum class BmsState : uint8_t {
    /// Hardware initialization, self-test (POST), baseline calibration.
    INIT = 0U,

    /// Contactors open, no load/charge present; baseline monitoring active.
    STANDBY = 1U,

    /// Contactor closed to charger; positive current flow; charging protections active.
    CHARGING = 2U,

    /// Contactor closed to load; negative current flow; under-voltage monitoring active.
    DISCHARGING = 3U,

    /// Passive or active cell balance circuitry engaged in standby or charge maintenance.
    BALANCING = 4U,

    /// Recoverable fault (e.g. cell swelling warning, mild thermal excursion); reduced limits.
    FAULT_DEGRADED = 5U,

    /// Immediate software shutdown; main contactors opened immediately.
    FAULT_CRITICAL = 6U,

    /// Passive post-event latched state reflecting that the pyro-fuse has been blown.
    TRIP_DETONATED = 7U
};

/* ============================================================================
 *  Individual Cell Status Enum
 * ============================================================================ */

/**
 * @brief Granular status of an individual series cell.
 */
enum class CellStatus : uint8_t {
    /// Normal nominal operation.
    ACTIVE = 0U,

    /// Shunt transistor/bypass active to bleed off excess charge.
    BALANCING = 1U,

    /// Permanently isolated from string via bypass MOSFETs.
    BYPASSED = 2U,

    /// Strain gauge / force sensor reports physical expansion beyond baseline.
    SWELLING_WARN = 3U,

    /// Cell exceeds hard safety margins (UVP/OVP/OTC).
    FAULTED = 4U
};

/* ============================================================================
 *  Stringification & Validation Predicates
 * ============================================================================ */

/**
 * @brief Converts BmsState enum to human-readable string representation.
 */
[[nodiscard]] constexpr inline const char* to_string(BmsState state) noexcept {
    switch (state) {
        case BmsState::INIT:
            return "INIT";
        case BmsState::STANDBY:
            return "STANDBY";
        case BmsState::CHARGING:
            return "CHARGING";
        case BmsState::DISCHARGING:
            return "DISCHARGING";
        case BmsState::BALANCING:
            return "BALANCING";
        case BmsState::FAULT_DEGRADED:
            return "FAULT_DEGRADED";
        case BmsState::FAULT_CRITICAL:
            return "FAULT_CRITICAL";
        case BmsState::TRIP_DETONATED:
            return "TRIP_DETONATED";
        default:
            return "UNKNOWN_BMS_STATE";
    }
}

/**
 * @brief Converts CellStatus enum to human-readable string representation.
 */
[[nodiscard]] constexpr inline const char* to_string(CellStatus status) noexcept {
    switch (status) {
        case CellStatus::ACTIVE:
            return "ACTIVE";
        case CellStatus::BALANCING:
            return "BALANCING";
        case CellStatus::BYPASSED:
            return "BYPASSED";
        case CellStatus::SWELLING_WARN:
            return "SWELLING_WARN";
        case CellStatus::FAULTED:
            return "FAULTED";
        default:
            return "UNKNOWN_CELL_STATUS";
    }
}

/**
 * @brief Checks if a raw integer corresponds to a valid BmsState.
 */
[[nodiscard]] constexpr inline bool is_valid_bms_state(BmsState state) noexcept {
    return state >= BmsState::INIT && state <= BmsState::TRIP_DETONATED;
}

/**
 * @brief Checks if a raw integer corresponds to a valid CellStatus.
 */
[[nodiscard]] constexpr inline bool is_valid_cell_status(CellStatus status) noexcept {
    return status >= CellStatus::ACTIVE && status <= CellStatus::FAULTED;
}

/* ============================================================================
 *  Compile-Time Invariants & Static Assertions
 * ============================================================================ */

static_assert(sizeof(BmsState) == 1, "BmsState must be exactly 1 byte (uint8_t)");
static_assert(sizeof(CellStatus) == 1, "CellStatus must be exactly 1 byte (uint8_t)");

}  // namespace bms::modules::types