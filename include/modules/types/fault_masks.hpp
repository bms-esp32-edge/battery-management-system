/*
 * @file fault_masks.hpp
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
 *  Fault Code Bitmask Enum
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
 *  Strongly Typed FaultMask Wrapper
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
 *  Stringification & Validation Predicates
 * ============================================================================ */

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

/* ============================================================================
 *  Compile-Time Invariants & Static Assertions
 * ============================================================================ */

static_assert(sizeof(FaultCode) == 2, "FaultCode must be exactly 2 bytes (uint16_t)");
static_assert(sizeof(FaultMask) == 2, "FaultMask must be exactly 2 bytes");

// Orthogonality / Non-overlapping bit checks for FaultCode
static_assert((static_cast<uint16_t>(FaultCode::OVP) & static_cast<uint16_t>(FaultCode::UVP)) == 0,
              "OVP and UVP bits must be strictly non-overlapping");
static_assert((static_cast<uint16_t>(FaultCode::OCP_CHARGE) &
               static_cast<uint16_t>(FaultCode::OCP_DISCHARGE)) == 0,
              "OCP_CHARGE and OCP_DISCHARGE bits must be strictly non-overlapping");
static_assert((static_cast<uint16_t>(FaultCode::OTC) & static_cast<uint16_t>(FaultCode::UTC)) == 0,
              "OTC and UTC bits must be strictly non-overlapping");
static_assert((static_cast<uint16_t>(FaultCode::SHORT_CIRCUIT) &
               static_cast<uint16_t>(FaultCode::THERMAL_RUNAWAY)) == 0,
              "SHORT_CIRCUIT and THERMAL_RUNAWAY bits must be strictly non-overlapping");
static_assert((static_cast<uint16_t>(FaultCode::SWELLING_CRITICAL) &
               static_cast<uint16_t>(FaultCode::COMM_TIMEOUT)) == 0,
              "SWELLING_CRITICAL and COMM_TIMEOUT bits must be strictly non-overlapping");

}  // namespace bms::modules::types