/**
 * @file types.hpp
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

namespace bms {
namespace core {

/* ============================================================================
 * 1. Pyrotechnic Actuation Guard Key
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
 * 2. High-Level System State Enum
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
 * 3. Individual Cell Status Enum
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
 * 4. Fault Code Bitmask Enum
 * ============================================================================ */

/**
 * @brief Bitmask-compatible error tracking codes.
 */
enum class FaultCode : uint16_t {
    /// No fault active.
    NONE = 0x0000U,

    /// 0x0001: Over-Voltage Protection tripped (>4.25V).
    OVP = 1U << 0,

    /// 0x0002: Under-Voltage Protection tripped (<2.80V).
    UVP = 1U << 1,

    /// 0x0004: Over-Current Protection during charging.
    OCP_CHARGE = 1U << 2,

    /// 0x0008: Over-Current Protection during discharge.
    OCP_DISCHARGE = 1U << 3,

    /// 0x0010: Over-Temperature in Cell (>55°C).
    OTC = 1U << 4,

    /// 0x0020: Under-Temperature in Cell (<0°C, inhibits charging).
    UTC = 1U << 5,

    /// 0x0040: Instantaneous hardware comparator threshold trip (>50A).
    SHORT_CIRCUIT = 1U << 6,

    /// 0x0080: Thermal rise velocity exceeds catastrophic threshold (dT/dt > 2.0°C/s).
    THERMAL_RUNAWAY = 1U << 7,

    /// 0x0100: Mechanical cell swelling exceeds safe limit (>12N).
    SWELLING_CRITICAL = 1U << 8,

    /// 0x0200: Loss of telemetry from AFE/internal ADC or bus (>500ms).
    COMM_TIMEOUT = 1U << 9
};

/* ============================================================================
 * 5. Telemetry Metric Freshness & Validity Flags
 * ============================================================================ */

/**
 * @brief Validity status bits for sensor telemetry.
 */
enum class MetricValidity : uint8_t {
    /// Telemetry reading is fresh and passed sanity checks.
    VALID = 0x01U,

    /// Telemetry reading has not updated within expected loop deadline.
    STALE = 0x02U,

    /// Communication bus error / CRC error occurred during acquisition.
    FAULT_COMM = 0x04U,

    /// Sensor is undergoing offset/baseline zeroing.
    CALIBRATING = 0x08U
};

/* ============================================================================
 * 6. Strongly Typed FaultMask Wrapper
 * ============================================================================ */

/**
 * @brief Strongly typed wrapper for FaultCode bitmask operations.
 */
class FaultMask {
public:
    constexpr FaultMask() noexcept : mask_(0U) {}
    explicit constexpr FaultMask(uint16_t raw) noexcept : mask_(raw) {}

    /**
     * @brief Intentional non-explicit converting constructor for ergonomic assignments:
     *        `FaultMask mask = FaultCode::OVP;`
     */
    constexpr FaultMask(FaultCode code) noexcept : mask_(static_cast<uint16_t>(code)) {}

    [[nodiscard]] constexpr bool has(FaultCode code) const noexcept {
        return (mask_ & static_cast<uint16_t>(code)) != 0U;
    }

    constexpr void set(FaultCode code) noexcept { mask_ |= static_cast<uint16_t>(code); }

    constexpr void clear(FaultCode code) noexcept {
        mask_ &= static_cast<uint16_t>(~static_cast<uint16_t>(code));
    }

    constexpr void reset() noexcept { mask_ = 0U; }

    [[nodiscard]] constexpr bool any() const noexcept { return mask_ != 0U; }

    [[nodiscard]] constexpr bool is_empty() const noexcept { return mask_ == 0U; }

    /**
     * @brief Returns total number of active fault flags.
     * @note  Uses __builtin_popcount (supported natively across GCC Xtensa and host GCC/Clang).
     */
    [[nodiscard]] constexpr uint8_t count() const noexcept {
        return static_cast<uint8_t>(__builtin_popcount(mask_));
    }

    [[nodiscard]] constexpr uint16_t raw_value() const noexcept { return mask_; }

    constexpr FaultMask operator|(FaultCode rhs) const noexcept {
        return FaultMask(static_cast<uint16_t>(mask_ | static_cast<uint16_t>(rhs)));
    }

    constexpr FaultMask operator|(FaultMask rhs) const noexcept {
        return FaultMask(static_cast<uint16_t>(mask_ | rhs.mask_));
    }

    constexpr FaultMask operator&(FaultCode rhs) const noexcept {
        return FaultMask(static_cast<uint16_t>(mask_ & static_cast<uint16_t>(rhs)));
    }

    constexpr FaultMask operator&(FaultMask rhs) const noexcept {
        return FaultMask(static_cast<uint16_t>(mask_ & rhs.mask_));
    }

    constexpr FaultMask operator^(FaultCode rhs) const noexcept {
        return FaultMask(static_cast<uint16_t>(mask_ ^ static_cast<uint16_t>(rhs)));
    }

    constexpr FaultMask operator^(FaultMask rhs) const noexcept {
        return FaultMask(static_cast<uint16_t>(mask_ ^ rhs.mask_));
    }

    constexpr FaultMask operator~() const noexcept {
        return FaultMask(static_cast<uint16_t>(~mask_));
    }

    constexpr FaultMask& operator|=(FaultCode rhs) noexcept {
        mask_ |= static_cast<uint16_t>(rhs);
        return *this;
    }

    constexpr FaultMask& operator|=(FaultMask rhs) noexcept {
        mask_ |= rhs.mask_;
        return *this;
    }

    constexpr FaultMask& operator&=(FaultCode rhs) noexcept {
        mask_ &= static_cast<uint16_t>(rhs);
        return *this;
    }

    constexpr FaultMask& operator&=(FaultMask rhs) noexcept {
        mask_ &= rhs.mask_;
        return *this;
    }

    constexpr FaultMask& operator^=(FaultCode rhs) noexcept {
        mask_ ^= static_cast<uint16_t>(rhs);
        return *this;
    }

    constexpr FaultMask& operator^=(FaultMask rhs) noexcept {
        mask_ ^= rhs.mask_;
        return *this;
    }

    constexpr bool operator==(FaultMask rhs) const noexcept { return mask_ == rhs.mask_; }

    constexpr bool operator!=(FaultMask rhs) const noexcept { return mask_ != rhs.mask_; }

private:
    uint16_t mask_{0U};
};

/**
 * @brief Free operator to allow `FaultCode::OVP | FaultCode::UVP -> FaultMask`
 */
constexpr inline FaultMask operator|(FaultCode lhs, FaultCode rhs) noexcept {
    return FaultMask(lhs) | rhs;
}

/**
 * @brief Free operator to allow `FaultCode::OVP & FaultCode::UVP -> FaultMask`
 */
constexpr inline FaultMask operator&(FaultCode lhs, FaultCode rhs) noexcept {
    return FaultMask(lhs) & rhs;
}

/**
 * @brief Free operator to allow `FaultCode::OVP ^ FaultCode::UVP -> FaultMask`
 */
constexpr inline FaultMask operator^(FaultCode lhs, FaultCode rhs) noexcept {
    return FaultMask(lhs) ^ rhs;
}

/* ============================================================================
 * 7. Strongly Typed ValidityMask Wrapper
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
 * 8. Canonical Telemetry Data Structures
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
 * 9. Stringification & Validation Predicates
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
 * @brief Converts FaultCode enum to human-readable string representation.
 */
[[nodiscard]] constexpr inline const char* to_string(FaultCode fault) noexcept {
    switch (fault) {
        case FaultCode::NONE:
            return "NONE";
        case FaultCode::OVP:
            return "OVP";
        case FaultCode::UVP:
            return "UVP";
        case FaultCode::OCP_CHARGE:
            return "OCP_CHARGE";
        case FaultCode::OCP_DISCHARGE:
            return "OCP_DISCHARGE";
        case FaultCode::OTC:
            return "OTC";
        case FaultCode::UTC:
            return "UTC";
        case FaultCode::SHORT_CIRCUIT:
            return "SHORT_CIRCUIT";
        case FaultCode::THERMAL_RUNAWAY:
            return "THERMAL_RUNAWAY";
        case FaultCode::SWELLING_CRITICAL:
            return "SWELLING_CRITICAL";
        case FaultCode::COMM_TIMEOUT:
            return "COMM_TIMEOUT";
        default:
            return "UNKNOWN_FAULT";
    }
}

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
 * 10. Compile-Time Invariants & Static Assertions
 * ============================================================================ */

static_assert(sizeof(BmsState) == 1, "BmsState must be exactly 1 byte (uint8_t)");
static_assert(sizeof(CellStatus) == 1, "CellStatus must be exactly 1 byte (uint8_t)");
static_assert(sizeof(FaultCode) == 2, "FaultCode must be exactly 2 bytes (uint16_t)");
static_assert(sizeof(MetricValidity) == 1, "MetricValidity must be exactly 1 byte (uint8_t)");
static_assert(sizeof(FaultMask) == 2, "FaultMask must be exactly 2 bytes");
static_assert(sizeof(ValidityMask) == 1, "ValidityMask must be exactly 1 byte");

// Orthogonality / Non-overlapping bit checks for FaultCode
static_assert((static_cast<uint16_t>(FaultCode::OVP) & static_cast<uint16_t>(FaultCode::UVP)) == 0,
              "FaultCode bits must be strictly non-overlapping");
static_assert((static_cast<uint16_t>(FaultCode::OCP_CHARGE) &
               static_cast<uint16_t>(FaultCode::OCP_DISCHARGE)) == 0,
              "FaultCode bits must be strictly non-overlapping");
static_assert((static_cast<uint16_t>(FaultCode::OTC) & static_cast<uint16_t>(FaultCode::UTC)) == 0,
              "FaultCode bits must be strictly non-overlapping");
static_assert((static_cast<uint16_t>(FaultCode::SHORT_CIRCUIT) &
               static_cast<uint16_t>(FaultCode::THERMAL_RUNAWAY)) == 0,
              "FaultCode bits must be strictly non-overlapping");
static_assert((static_cast<uint16_t>(FaultCode::SWELLING_CRITICAL) &
               static_cast<uint16_t>(FaultCode::COMM_TIMEOUT)) == 0,
              "FaultCode bits must be strictly non-overlapping");

// Memory layout guarantees for safe FreeRTOS task / ISR memcpy and deterministic serialization
static_assert(std::is_standard_layout_v<CellMetrics>, "CellMetrics must have standard layout");
static_assert(std::is_trivially_copyable_v<CellMetrics>, "CellMetrics must be trivially copyable");

static_assert(std::is_standard_layout_v<PackMetrics>, "PackMetrics must have standard layout");
static_assert(std::is_trivially_copyable_v<PackMetrics>, "PackMetrics must be trivially copyable");

static_assert(std::is_standard_layout_v<PyroTriggerKey>,
              "PyroTriggerKey must have standard layout");
static_assert(std::is_trivially_copyable_v<PyroTriggerKey>,
              "PyroTriggerKey must be trivially copyable");

}  // namespace core
}  // namespace bms
