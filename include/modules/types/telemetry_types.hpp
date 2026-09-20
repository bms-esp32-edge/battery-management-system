/*
 * @file telemetry_types.hpp
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

#include "fault_masks.hpp"
#include "system_enums.hpp"

namespace bms::modules::types {

/* ============================================================================
 *  Telemetry Metric Freshness & Validity Flags
 * ============================================================================ */

/**
 * @brief Validity status bits for sensor telemetry.
 */
enum class MetricValidity : uint8_t {
    /// 0x01: Telemetry reading is fresh and passed sanity checks.
    VALID = 1U << 0,

    /// 0x02: Telemetry reading has not updated within expected loop deadline.
    STALE = 1U << 1,

    /// 0x04: Communication bus error / CRC error occurred during acquisition.
    FAULT_COMM = 1U << 2,

    /// 0x08: Sensor is undergoing offset/baseline zeroing.
    CALIBRATING = 1U << 3
};

/* ============================================================================
 *  Strongly Typed ValidityMask Wrapper
 * ============================================================================ */

/**
 * @brief Lightweight typed bitmask wrapper for MetricValidity flags.
 * @note  The telemetry producer module (ADC/AFE driver) owns state transitions between
 *        VALID, STALE, FAULT_COMM, and CALIBRATING.
 */
class ValidityMask {
public:
    constexpr ValidityMask() noexcept : mask_(static_cast<uint8_t>(MetricValidity::VALID)) {}
    explicit constexpr ValidityMask(uint8_t raw) noexcept : mask_(raw) {}
    constexpr ValidityMask(MetricValidity v) noexcept : mask_(static_cast<uint8_t>(v)) {}

    [[nodiscard]] constexpr bool has(MetricValidity v) const noexcept {
        return (mask_ & static_cast<uint8_t>(v)) != 0U;
    }

    constexpr void set(MetricValidity v) noexcept { mask_ |= static_cast<uint8_t>(v); }

    constexpr void clear(MetricValidity v) noexcept {
        mask_ &= static_cast<uint8_t>(~static_cast<uint8_t>(v));
    }

    constexpr void reset(MetricValidity v = MetricValidity::VALID) noexcept {
        mask_ = static_cast<uint8_t>(v);
    }

    [[nodiscard]] constexpr bool is_valid() const noexcept {
        return has(MetricValidity::VALID) && !has(MetricValidity::STALE) &&
               !has(MetricValidity::FAULT_COMM);
    }

    [[nodiscard]] constexpr uint8_t raw_value() const noexcept { return mask_; }

    constexpr ValidityMask operator|(MetricValidity rhs) const noexcept {
        return ValidityMask(static_cast<uint8_t>(mask_ | static_cast<uint8_t>(rhs)));
    }

    constexpr ValidityMask operator&(MetricValidity rhs) const noexcept {
        return ValidityMask(static_cast<uint8_t>(mask_ & static_cast<uint8_t>(rhs)));
    }

    constexpr ValidityMask operator~() const noexcept {
        return ValidityMask(static_cast<uint8_t>(~mask_));
    }

    constexpr ValidityMask& operator|=(MetricValidity rhs) noexcept {
        mask_ |= static_cast<uint8_t>(rhs);
        return *this;
    }

    constexpr ValidityMask& operator&=(MetricValidity rhs) noexcept {
        mask_ &= static_cast<uint8_t>(rhs);
        return *this;
    }

    constexpr bool operator==(ValidityMask rhs) const noexcept { return mask_ == rhs.mask_; }

    constexpr bool operator!=(ValidityMask rhs) const noexcept { return mask_ != rhs.mask_; }

private:
    uint8_t mask_{static_cast<uint8_t>(MetricValidity::VALID)};
};

/* ============================================================================
 *  Canonical Telemetry Data Structures
 * ============================================================================ */

/**
 * @brief Granular telemetry and health status for a single battery cell.
 *
 * @note  Trivially copyable and standard layout. Safe for cross-task sharing inside
 *        critical sections (taskENTER_CRITICAL / taskEXIT_CRITICAL).
 * @note  CRC-16-CCITT is computed over all bytes preceding the trailing crc16 field:
 *        `sizeof(CellMetrics) - sizeof(uint16_t)`.
 */
struct CellMetrics {
    /// Individual cell terminal potential (V), resolution: 0.1mV
    float voltage_v{0.0f};

    /// Thermistor measurement localized to cell casing (°C), resolution: 0.1°C
    float temperature_c{0.0f};

    /// Mechanical force or strain measurement in Newtons (N), resolution: 0.05N
    float swelling_force_n{0.0f};

    /// Monotonic system millisecond timestamp at sample capture
    uint32_t timestamp_ms{0U};

    /// Monotonic rolling sample counter
    uint16_t sequence_id{0U};

    /// Current classification of cell health
    CellStatus status{CellStatus::ACTIVE};

    /// Data freshness and validity status
    ValidityMask validity{};

    /// Flag representing active state of shunt resistor/MOSFET
    bool is_balancing{false};

    /// Trailing checksum (CRC-16-CCITT across preceding bytes)
    uint16_t crc16{0U};
};

/**
 * @brief Aggregate pack-level telemetry, thermal envelope, and fault bitmask.
 *
 * @note  Trivially copyable and standard layout. Safe for cross-task sharing inside
 *        critical sections (taskENTER_CRITICAL / taskEXIT_CRITICAL).
 * @note  CRC-16-CCITT is computed over all bytes preceding the trailing crc16 field:
 *        `sizeof(PackMetrics) - sizeof(uint16_t)`.
 */
struct PackMetrics {
    /// Sum of series cell potentials or pack-level terminal voltage (V)
    float total_voltage_v{0.0f};

    /// Instantaneous shunt/Hall sensor reading (A) (+ charge, - discharge)
    float pack_current_a{0.0f};

    /// Disparity between highest and lowest cell: Vmax - Vmin (V)
    float delta_voltage_v{0.0f};

    /// Worst-case highest thermistor reading in the pack (°C)
    float max_cell_temp_c{0.0f};

    /// Lowest thermistor reading in the pack (°C)
    float min_cell_temp_c{0.0f};

    /// Calculated pack State of Charge (0.0% - 100.0%)
    float state_of_charge_pct{0.0f};

    /// Monotonic system millisecond timestamp at sample capture
    uint32_t timestamp_ms{0U};

    /// Monotonic rolling pack telemetry sequence counter
    uint16_t sequence_id{0U};

    /// Strongly typed bitfield of active FaultCode flags
    FaultMask active_faults{};

    /// Total operational cells contributing to the stack
    uint8_t active_cell_count{0U};

    /// Telemetry validity status
    ValidityMask validity{};

    /// Trailing checksum (CRC-16-CCITT across preceding bytes)
    uint16_t crc16{0U};
};

/* ============================================================================
 *  Stringification & Validation Predicates
 * ============================================================================ */

/**
 * @brief Converts MetricValidity enum to human-readable string representation.
 */
[[nodiscard]] constexpr inline const char* to_string(MetricValidity validity) noexcept {
    switch (validity) {
        case MetricValidity::VALID:
            return "VALID";
        case MetricValidity::STALE:
            return "STALE";
        case MetricValidity::FAULT_COMM:
            return "FAULT_COMM";
        case MetricValidity::CALIBRATING:
            return "CALIBRATING";
        default:
            return "UNKNOWN_VALIDITY";
    }
}

/* ============================================================================
 *  Compile-Time Invariants & Static Assertions
 * ============================================================================ */

static_assert(sizeof(MetricValidity) == 1, "MetricValidity must be exactly 1 byte (uint8_t)");
static_assert(sizeof(ValidityMask) == 1, "ValidityMask must be exactly 1 byte");

// Orthogonality / Non-overlapping bit checks for MetricValidity
static_assert((static_cast<uint8_t>(MetricValidity::VALID) &
               static_cast<uint8_t>(MetricValidity::STALE)) == 0,
              "MetricValidity VALID and STALE bits must be strictly non-overlapping");
static_assert((static_cast<uint8_t>(MetricValidity::FAULT_COMM) &
               static_cast<uint8_t>(MetricValidity::CALIBRATING)) == 0,
              "MetricValidity FAULT_COMM and CALIBRATING bits must be strictly non-overlapping");

// Memory layout guarantees for safe FreeRTOS task / ISR memcpy and deterministic serialization
static_assert(std::is_standard_layout_v<CellMetrics>, "CellMetrics must have standard layout");
static_assert(std::is_trivially_copyable_v<CellMetrics>, "CellMetrics must be trivially copyable");

static_assert(std::is_standard_layout_v<PackMetrics>, "PackMetrics must have standard layout");
static_assert(std::is_trivially_copyable_v<PackMetrics>, "PackMetrics must be trivially copyable");

}  // namespace bms::modules::types