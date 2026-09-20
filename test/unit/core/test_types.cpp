/**
 * @file test_types.cpp
 * @brief Comprehensive native unit tests for BMS core types, scoped enums,
 *        safety keys, FaultMask, ValidityMask, telemetry structs, and stringifiers.
 */

#include <cstdint>
#include <cstring>
#include <type_traits>

#include "firmware/core/types.hpp"

#include "unity.h"

using namespace bms::core;

/* ============================================================================
 * Suite 1: Memory Layout, Standard Layout, & Alignment
 * ============================================================================ */

void test_type_sizes_and_alignment(void) {
    TEST_ASSERT_EQUAL_INT(1, sizeof(BmsState));
    TEST_ASSERT_EQUAL_INT(1, sizeof(CellStatus));
    TEST_ASSERT_EQUAL_INT(2, sizeof(FaultCode));
    TEST_ASSERT_EQUAL_INT(1, sizeof(MetricValidity));
    TEST_ASSERT_EQUAL_INT(2, sizeof(FaultMask));
    TEST_ASSERT_EQUAL_INT(1, sizeof(ValidityMask));

    TEST_ASSERT_TRUE(std::is_standard_layout_v<CellMetrics>);
    TEST_ASSERT_TRUE(std::is_trivially_copyable_v<CellMetrics>);

    TEST_ASSERT_TRUE(std::is_standard_layout_v<PackMetrics>);
    TEST_ASSERT_TRUE(std::is_trivially_copyable_v<PackMetrics>);

    TEST_ASSERT_TRUE(std::is_standard_layout_v<PyroTriggerKey>);
    TEST_ASSERT_TRUE(std::is_trivially_copyable_v<PyroTriggerKey>);
}

/* ============================================================================
 * Suite 2: PyroTriggerKey Lifecycle & Exact Boundary Confirmation Window
 * ============================================================================ */

void test_pyro_trigger_key_lifecycle_and_timing(void) {
    PyroTriggerKey key{};

    // Initial state: uninitialized/unarmed
    TEST_ASSERT_FALSE(key.is_armed());
    TEST_ASSERT_FALSE(key.is_fire_authorized(0));
    TEST_ASSERT_FALSE(key.is_fire_authorized(1000));

    // Arming at t = 1000ms
    key.arm(1000U, 42U);
    TEST_ASSERT_TRUE(key.is_armed());
    TEST_ASSERT_EQUAL_INT(42, key.sequence_id);
    TEST_ASSERT_EQUAL_INT(1000, key.armed_at_ms);

    // Fire not authorized before fire token is supplied
    TEST_ASSERT_FALSE(key.is_fire_authorized(1010U));

    // Set valid fire token
    key.fire_key = PyroTriggerKey::FIRE_MAGIC;

    // Exact Boundary Authorization checks:
    // [0ms, 50ms] active window
    TEST_ASSERT_TRUE(
        key.is_fire_authorized(1000U));  // Exact lower edge (delta = 0ms) -> Authorized
    TEST_ASSERT_TRUE(key.is_fire_authorized(1025U));  // Mid window (delta = 25ms) -> Authorized
    TEST_ASSERT_TRUE(
        key.is_fire_authorized(1050U));  // Exact upper limit (delta = 50ms) -> Authorized

    // Exact Boundary Rejection checks:
    TEST_ASSERT_FALSE(
        key.is_fire_authorized(1051U));  // Exact first expired tick (delta = 51ms) -> Rejected
    TEST_ASSERT_FALSE(key.is_fire_authorized(1052U));  // delta = 52ms -> Rejected
    TEST_ASSERT_FALSE(key.is_fire_authorized(2000U));  // delta = 1000ms -> Rejected

    // Rejection on clock underflow / reversal (current_time_ms < armed_at_ms)
    TEST_ASSERT_FALSE(key.is_fire_authorized(999U));  // Exact 1 tick before armed -> Rejected
    TEST_ASSERT_FALSE(key.is_fire_authorized(0U));    // 0ms underflow -> Rejected

    // Disarm clears everything
    key.disarm();
    TEST_ASSERT_FALSE(key.is_armed());
    TEST_ASSERT_FALSE(key.is_fire_authorized(1000U));
    TEST_ASSERT_EQUAL_INT(0, key.arm_key);
    TEST_ASSERT_EQUAL_INT(0, key.fire_key);
    TEST_ASSERT_EQUAL_INT(0, key.armed_at_ms);
    TEST_ASSERT_EQUAL_INT(0, key.sequence_id);

    // Partial state: Valid arm key, corrupted fire key
    key.arm(5000U, 1U);
    key.fire_key = 0xDEADBEEFU;
    TEST_ASSERT_TRUE(key.is_armed());
    TEST_ASSERT_FALSE(key.is_fire_authorized(5010U));

    // Partial state: Corrupted arm key, valid fire key
    key.arm_key = 0xDEADBEEFU;
    key.fire_key = PyroTriggerKey::FIRE_MAGIC;
    key.armed_at_ms = 5000U;
    TEST_ASSERT_FALSE(key.is_armed());
    TEST_ASSERT_FALSE(key.is_fire_authorized(5010U));
}

/* ============================================================================
 * Suite 3: Scoped Enum Values & Bit Shifts
 * ============================================================================ */

void test_enum_values_and_constants(void) {
    // BmsState
    TEST_ASSERT_EQUAL_UINT8(0, static_cast<uint8_t>(BmsState::INIT));
    TEST_ASSERT_EQUAL_UINT8(1, static_cast<uint8_t>(BmsState::STANDBY));
    TEST_ASSERT_EQUAL_UINT8(2, static_cast<uint8_t>(BmsState::CHARGING));
    TEST_ASSERT_EQUAL_UINT8(3, static_cast<uint8_t>(BmsState::DISCHARGING));
    TEST_ASSERT_EQUAL_UINT8(4, static_cast<uint8_t>(BmsState::BALANCING));
    TEST_ASSERT_EQUAL_UINT8(5, static_cast<uint8_t>(BmsState::FAULT_DEGRADED));
    TEST_ASSERT_EQUAL_UINT8(6, static_cast<uint8_t>(BmsState::FAULT_CRITICAL));
    TEST_ASSERT_EQUAL_UINT8(7, static_cast<uint8_t>(BmsState::TRIP_DETONATED));

    // CellStatus
    TEST_ASSERT_EQUAL_UINT8(0, static_cast<uint8_t>(CellStatus::ACTIVE));
    TEST_ASSERT_EQUAL_UINT8(1, static_cast<uint8_t>(CellStatus::BALANCING));
    TEST_ASSERT_EQUAL_UINT8(2, static_cast<uint8_t>(CellStatus::BYPASSED));
    TEST_ASSERT_EQUAL_UINT8(3, static_cast<uint8_t>(CellStatus::SWELLING_WARN));
    TEST_ASSERT_EQUAL_UINT8(4, static_cast<uint8_t>(CellStatus::FAULTED));

    // FaultCode bit shifts
    TEST_ASSERT_EQUAL_INT(0x0000, static_cast<uint16_t>(FaultCode::NONE));
    TEST_ASSERT_EQUAL_INT(0x0001, static_cast<uint16_t>(FaultCode::OVP));
    TEST_ASSERT_EQUAL_INT(0x0002, static_cast<uint16_t>(FaultCode::UVP));
    TEST_ASSERT_EQUAL_INT(0x0004, static_cast<uint16_t>(FaultCode::OCP_CHARGE));
    TEST_ASSERT_EQUAL_INT(0x0008, static_cast<uint16_t>(FaultCode::OCP_DISCHARGE));
    TEST_ASSERT_EQUAL_INT(0x0010, static_cast<uint16_t>(FaultCode::OTC));
    TEST_ASSERT_EQUAL_INT(0x0020, static_cast<uint16_t>(FaultCode::UTC));
    TEST_ASSERT_EQUAL_INT(0x0040, static_cast<uint16_t>(FaultCode::SHORT_CIRCUIT));
    TEST_ASSERT_EQUAL_INT(0x0080, static_cast<uint16_t>(FaultCode::THERMAL_RUNAWAY));
    TEST_ASSERT_EQUAL_INT(0x0100, static_cast<uint16_t>(FaultCode::SWELLING_CRITICAL));
    TEST_ASSERT_EQUAL_INT(0x0200, static_cast<uint16_t>(FaultCode::COMM_TIMEOUT));

    // MetricValidity
    TEST_ASSERT_EQUAL_UINT8(0x01, static_cast<uint8_t>(MetricValidity::VALID));
    TEST_ASSERT_EQUAL_UINT8(0x02, static_cast<uint8_t>(MetricValidity::STALE));
    TEST_ASSERT_EQUAL_UINT8(0x04, static_cast<uint8_t>(MetricValidity::FAULT_COMM));
    TEST_ASSERT_EQUAL_UINT8(0x08, static_cast<uint8_t>(MetricValidity::CALIBRATING));
}

/* ============================================================================
 * Suite 4: FaultMask Operations & Adjacent Bit Preservation
 * ============================================================================ */

void test_fault_mask_operations(void) {
    FaultMask mask{};
    TEST_ASSERT_TRUE(mask.is_empty());
    TEST_ASSERT_FALSE(mask.any());
    TEST_ASSERT_EQUAL_INT(0, mask.count());
    TEST_ASSERT_EQUAL_INT(0x0000, mask.raw_value());

    // Explicit raw constructor
    FaultMask raw_mask(0x0003U);
    TEST_ASSERT_TRUE(raw_mask.has(FaultCode::OVP));
    TEST_ASSERT_TRUE(raw_mask.has(FaultCode::UVP));
    TEST_ASSERT_EQUAL_INT(2, raw_mask.count());

    // Single fault conversion constructor
    FaultMask ovp_mask = FaultCode::OVP;
    TEST_ASSERT_TRUE(ovp_mask.has(FaultCode::OVP));
    TEST_ASSERT_FALSE(ovp_mask.has(FaultCode::UVP));
    TEST_ASSERT_EQUAL_INT(1, ovp_mask.count());

    // Setting multiple faults
    mask.set(FaultCode::OVP);
    mask.set(FaultCode::UVP);
    mask.set(FaultCode::SHORT_CIRCUIT);
    TEST_ASSERT_TRUE(mask.any());
    TEST_ASSERT_FALSE(mask.is_empty());
    TEST_ASSERT_EQUAL_INT(3, mask.count());
    TEST_ASSERT_TRUE(mask.has(FaultCode::OVP));
    TEST_ASSERT_TRUE(mask.has(FaultCode::UVP));
    TEST_ASSERT_TRUE(mask.has(FaultCode::SHORT_CIRCUIT));
    TEST_ASSERT_FALSE(mask.has(FaultCode::OTC));

    // Clear OVP and verify adjacent bits (UVP, SHORT_CIRCUIT) are strictly preserved
    mask.clear(FaultCode::OVP);
    TEST_ASSERT_FALSE(mask.has(FaultCode::OVP));
    TEST_ASSERT_TRUE(mask.has(FaultCode::UVP));
    TEST_ASSERT_TRUE(mask.has(FaultCode::SHORT_CIRCUIT));
    TEST_ASSERT_EQUAL_INT(2, mask.count());

    // Clear UVP
    mask.clear(FaultCode::UVP);
    TEST_ASSERT_FALSE(mask.has(FaultCode::UVP));
    TEST_ASSERT_TRUE(mask.has(FaultCode::SHORT_CIRCUIT));
    TEST_ASSERT_EQUAL_INT(1, mask.count());

    // Reset
    mask.reset();
    TEST_ASSERT_TRUE(mask.is_empty());
    TEST_ASSERT_EQUAL_INT(0, mask.count());

    // Bitwise operators with FaultCode & FaultMask
    FaultMask f1 = FaultCode::OVP | FaultCode::OTC;
    TEST_ASSERT_TRUE(f1.has(FaultCode::OVP));
    TEST_ASSERT_TRUE(f1.has(FaultCode::OTC));
    TEST_ASSERT_EQUAL_INT(2, f1.count());

    FaultMask f2 = FaultCode::OTC | FaultCode::UVP;
    FaultMask f_and = f1 & f2;
    TEST_ASSERT_TRUE(f_and.has(FaultCode::OTC));
    TEST_ASSERT_FALSE(f_and.has(FaultCode::OVP));
    TEST_ASSERT_FALSE(f_and.has(FaultCode::UVP));
    TEST_ASSERT_EQUAL_INT(1, f_and.count());

    FaultMask f_and_code = f1 & FaultCode::OVP;
    TEST_ASSERT_TRUE(f_and_code.has(FaultCode::OVP));
    TEST_ASSERT_FALSE(f_and_code.has(FaultCode::OTC));

    FaultMask f_xor = f1 ^ f2;
    TEST_ASSERT_TRUE(f_xor.has(FaultCode::OVP));
    TEST_ASSERT_TRUE(f_xor.has(FaultCode::UVP));
    TEST_ASSERT_FALSE(f_xor.has(FaultCode::OTC));
    TEST_ASSERT_EQUAL_INT(2, f_xor.count());

    FaultMask f_xor_code = f1 ^ FaultCode::OVP;
    TEST_ASSERT_FALSE(f_xor_code.has(FaultCode::OVP));
    TEST_ASSERT_TRUE(f_xor_code.has(FaultCode::OTC));

    FaultMask f_comp = ~FaultMask(FaultCode::OVP);
    TEST_ASSERT_FALSE(f_comp.has(FaultCode::OVP));
    TEST_ASSERT_TRUE(f_comp.has(FaultCode::UVP));

    // In-place operators with FaultCode
    FaultMask acc{};
    acc |= FaultCode::COMM_TIMEOUT;
    TEST_ASSERT_TRUE(acc.has(FaultCode::COMM_TIMEOUT));
    acc &= FaultCode::COMM_TIMEOUT;
    TEST_ASSERT_TRUE(acc.has(FaultCode::COMM_TIMEOUT));
    acc ^= FaultCode::COMM_TIMEOUT;
    TEST_ASSERT_FALSE(acc.has(FaultCode::COMM_TIMEOUT));

    // In-place operators with FaultMask
    acc |= FaultMask(FaultCode::OVP);
    TEST_ASSERT_TRUE(acc.has(FaultCode::OVP));
    acc &= FaultMask(FaultCode::OVP);
    TEST_ASSERT_TRUE(acc.has(FaultCode::OVP));
    acc ^= FaultMask(FaultCode::OVP);
    TEST_ASSERT_FALSE(acc.has(FaultCode::OVP));

    // Free operators
    FaultMask free_or = FaultCode::OVP | FaultCode::UVP;
    FaultMask free_and = FaultCode::OVP & FaultCode::UVP;
    FaultMask free_xor = FaultCode::OVP ^ FaultCode::UVP;
    TEST_ASSERT_TRUE(free_or.has(FaultCode::OVP));
    TEST_ASSERT_TRUE(free_or.has(FaultCode::UVP));
    TEST_ASSERT_TRUE(free_and.is_empty());
    TEST_ASSERT_TRUE(free_xor.has(FaultCode::OVP));
    TEST_ASSERT_TRUE(free_xor.has(FaultCode::UVP));

    // Equality operators
    FaultMask a(0x0005);
    FaultMask b(0x0005);
    FaultMask c(0x0006);
    TEST_ASSERT_TRUE(a == b);
    TEST_ASSERT_FALSE(a != b);
    TEST_ASSERT_TRUE(a != c);
}

/* ============================================================================
 * Suite 5: ValidityMask Operations
 * ============================================================================ */

void test_validity_mask_operations(void) {
    ValidityMask val{};
    TEST_ASSERT_TRUE(val.has(MetricValidity::VALID));
    TEST_ASSERT_FALSE(val.has(MetricValidity::STALE));
    TEST_ASSERT_TRUE(val.is_valid());
    TEST_ASSERT_EQUAL_UINT8(0x01, val.raw_value());

    // Explicit raw constructor
    ValidityMask raw_val(0x03U);
    TEST_ASSERT_TRUE(raw_val.has(MetricValidity::VALID));
    TEST_ASSERT_TRUE(raw_val.has(MetricValidity::STALE));
    TEST_ASSERT_FALSE(raw_val.is_valid());

    // Single enum constructor
    ValidityMask enum_val(MetricValidity::CALIBRATING);
    TEST_ASSERT_TRUE(enum_val.has(MetricValidity::CALIBRATING));
    TEST_ASSERT_FALSE(enum_val.has(MetricValidity::VALID));
    TEST_ASSERT_FALSE(enum_val.is_valid());

    // Mark as stale
    val.set(MetricValidity::STALE);
    TEST_ASSERT_TRUE(val.has(MetricValidity::STALE));
    TEST_ASSERT_FALSE(val.is_valid());

    // Clear stale
    val.clear(MetricValidity::STALE);
    TEST_ASSERT_FALSE(val.has(MetricValidity::STALE));
    TEST_ASSERT_TRUE(val.is_valid());

    // Mark communication fault
    val.set(MetricValidity::FAULT_COMM);
    TEST_ASSERT_FALSE(val.is_valid());

    // Reset to calibrating
    val.reset(MetricValidity::CALIBRATING);
    TEST_ASSERT_TRUE(val.has(MetricValidity::CALIBRATING));
    TEST_ASSERT_FALSE(val.has(MetricValidity::VALID));
    TEST_ASSERT_FALSE(val.is_valid());

    // Reset to default VALID
    val.reset();
    TEST_ASSERT_TRUE(val.has(MetricValidity::VALID));
    TEST_ASSERT_TRUE(val.is_valid());

    // Bitwise operators
    ValidityMask vm1 = MetricValidity::VALID;
    ValidityMask vm2 = vm1 | MetricValidity::STALE;
    TEST_ASSERT_TRUE(vm2.has(MetricValidity::VALID));
    TEST_ASSERT_TRUE(vm2.has(MetricValidity::STALE));

    ValidityMask vm_and = vm2 & MetricValidity::VALID;
    TEST_ASSERT_TRUE(vm_and.has(MetricValidity::VALID));
    TEST_ASSERT_FALSE(vm_and.has(MetricValidity::STALE));

    ValidityMask vm_comp = ~vm1;
    TEST_ASSERT_FALSE(vm_comp.has(MetricValidity::VALID));

    ValidityMask vm_acc{};
    vm_acc |= MetricValidity::STALE;
    TEST_ASSERT_TRUE(vm_acc.has(MetricValidity::STALE));
    vm_acc &= MetricValidity::VALID;
    TEST_ASSERT_FALSE(vm_acc.has(MetricValidity::STALE));

    ValidityMask v_a(0x01);
    ValidityMask v_b(0x01);
    ValidityMask v_c(0x02);
    TEST_ASSERT_TRUE(v_a == v_b);
    TEST_ASSERT_FALSE(v_a != v_b);
    TEST_ASSERT_TRUE(v_a != v_c);
}

/* ============================================================================
 * Suite 6: Telemetry Structs Default Initialization, Memcpy, & Wrapper Semantics
 * ============================================================================ */

void test_struct_initialization_and_memcpy(void) {
    CellMetrics cell{};
    TEST_ASSERT_EQUAL_FLOAT(0.0f, cell.voltage_v);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, cell.temperature_c);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, cell.swelling_force_n);
    TEST_ASSERT_EQUAL_INT(0, cell.timestamp_ms);
    TEST_ASSERT_EQUAL_INT(0, cell.sequence_id);
    TEST_ASSERT_TRUE(cell.status == CellStatus::ACTIVE);
    TEST_ASSERT_TRUE(cell.validity.is_valid());
    TEST_ASSERT_FALSE(cell.is_balancing);
    TEST_ASSERT_EQUAL_INT(0, cell.crc16);

    // Populate CellMetrics with realistic telemetry data
    cell.voltage_v = 3.654f;
    cell.temperature_c = 28.5f;
    cell.swelling_force_n = 2.1f;
    cell.timestamp_ms = 123456U;
    cell.sequence_id = 99U;
    cell.status = CellStatus::BALANCING;
    cell.validity.reset(MetricValidity::VALID);
    cell.is_balancing = true;
    cell.crc16 = 0xABCDU;

    // Perform raw byte memcpy (simulating ISR/DMA snapshot or cross-task ring-buffer write)
    CellMetrics cell_copy{};
    std::memcpy(&cell_copy, &cell, sizeof(CellMetrics));

    // Validate primitive fields
    TEST_ASSERT_EQUAL_FLOAT(3.654f, cell_copy.voltage_v);
    TEST_ASSERT_EQUAL_FLOAT(28.5f, cell_copy.temperature_c);
    TEST_ASSERT_EQUAL_FLOAT(2.1f, cell_copy.swelling_force_n);
    TEST_ASSERT_EQUAL_INT(123456, cell_copy.timestamp_ms);
    TEST_ASSERT_EQUAL_INT(99, cell_copy.sequence_id);
    TEST_ASSERT_TRUE(cell_copy.status == CellStatus::BALANCING);
    TEST_ASSERT_TRUE(cell_copy.is_balancing);
    TEST_ASSERT_EQUAL_INT(0xABCD, cell_copy.crc16);

    // Validate wrapper methods on the memcpy'd copy behave identically
    TEST_ASSERT_TRUE(cell_copy.validity.is_valid());
    TEST_ASSERT_TRUE(cell_copy.validity.has(MetricValidity::VALID));
    TEST_ASSERT_FALSE(cell_copy.validity.has(MetricValidity::STALE));
    TEST_ASSERT_EQUAL_UINT8(cell.validity.raw_value(), cell_copy.validity.raw_value());

    // PackMetrics checks
    PackMetrics pack{};
    TEST_ASSERT_EQUAL_FLOAT(0.0f, pack.total_voltage_v);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, pack.pack_current_a);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, pack.delta_voltage_v);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, pack.max_cell_temp_c);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, pack.min_cell_temp_c);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, pack.state_of_charge_pct);
    TEST_ASSERT_EQUAL_INT(0, pack.timestamp_ms);
    TEST_ASSERT_EQUAL_INT(0, pack.sequence_id);
    TEST_ASSERT_TRUE(pack.active_faults.is_empty());
    TEST_ASSERT_EQUAL_UINT8(0, pack.active_cell_count);
    TEST_ASSERT_TRUE(pack.validity.is_valid());
    TEST_ASSERT_EQUAL_INT(0, pack.crc16);

    pack.total_voltage_v = 14.8f;
    pack.pack_current_a = -5.2f;
    pack.delta_voltage_v = 0.015f;
    pack.max_cell_temp_c = 34.2f;
    pack.min_cell_temp_c = 28.1f;
    pack.state_of_charge_pct = 82.5f;
    pack.timestamp_ms = 654321U;
    pack.sequence_id = 1001U;
    pack.active_cell_count = 4U;
    pack.active_faults = FaultCode::OVP | FaultCode::OTC;
    pack.validity.set(MetricValidity::STALE);
    pack.crc16 = 0x5678U;

    // Perform raw byte memcpy
    PackMetrics pack_copy{};
    std::memcpy(&pack_copy, &pack, sizeof(PackMetrics));

    // Validate primitive fields
    TEST_ASSERT_EQUAL_FLOAT(14.8f, pack_copy.total_voltage_v);
    TEST_ASSERT_EQUAL_FLOAT(-5.2f, pack_copy.pack_current_a);
    TEST_ASSERT_EQUAL_FLOAT(0.015f, pack_copy.delta_voltage_v);
    TEST_ASSERT_EQUAL_FLOAT(34.2f, pack_copy.max_cell_temp_c);
    TEST_ASSERT_EQUAL_FLOAT(28.1f, pack_copy.min_cell_temp_c);
    TEST_ASSERT_EQUAL_FLOAT(82.5f, pack_copy.state_of_charge_pct);
    TEST_ASSERT_EQUAL_INT(654321, pack_copy.timestamp_ms);
    TEST_ASSERT_EQUAL_INT(1001, pack_copy.sequence_id);
    TEST_ASSERT_EQUAL_UINT8(4, pack_copy.active_cell_count);
    TEST_ASSERT_EQUAL_INT(0x5678, pack_copy.crc16);

    // Validate FaultMask wrapper methods on memcpy'd copy
    TEST_ASSERT_FALSE(pack_copy.active_faults.is_empty());
    TEST_ASSERT_TRUE(pack_copy.active_faults.any());
    TEST_ASSERT_EQUAL_INT(2, pack_copy.active_faults.count());
    TEST_ASSERT_TRUE(pack_copy.active_faults.has(FaultCode::OVP));
    TEST_ASSERT_TRUE(pack_copy.active_faults.has(FaultCode::OTC));
    TEST_ASSERT_FALSE(pack_copy.active_faults.has(FaultCode::UVP));
    TEST_ASSERT_EQUAL_INT(pack.active_faults.raw_value(), pack_copy.active_faults.raw_value());

    // Validate ValidityMask wrapper methods on memcpy'd copy
    TEST_ASSERT_FALSE(pack_copy.validity.is_valid());
    TEST_ASSERT_TRUE(pack_copy.validity.has(MetricValidity::STALE));
    TEST_ASSERT_EQUAL_UINT8(pack.validity.raw_value(), pack_copy.validity.raw_value());
}

/* ============================================================================
 * Suite 7: Stringification & Out-of-Range Boundary Fuzzing
 * ============================================================================ */

void test_stringification_and_fuzzing(void) {
    // Valid BmsState strings
    TEST_ASSERT_EQUAL_INT(0, std::strcmp("INIT", to_string(BmsState::INIT)));
    TEST_ASSERT_EQUAL_INT(0, std::strcmp("STANDBY", to_string(BmsState::STANDBY)));
    TEST_ASSERT_EQUAL_INT(0, std::strcmp("CHARGING", to_string(BmsState::CHARGING)));
    TEST_ASSERT_EQUAL_INT(0, std::strcmp("DISCHARGING", to_string(BmsState::DISCHARGING)));
    TEST_ASSERT_EQUAL_INT(0, std::strcmp("BALANCING", to_string(BmsState::BALANCING)));
    TEST_ASSERT_EQUAL_INT(0, std::strcmp("FAULT_DEGRADED", to_string(BmsState::FAULT_DEGRADED)));
    TEST_ASSERT_EQUAL_INT(0, std::strcmp("FAULT_CRITICAL", to_string(BmsState::FAULT_CRITICAL)));
    TEST_ASSERT_EQUAL_INT(0, std::strcmp("TRIP_DETONATED", to_string(BmsState::TRIP_DETONATED)));

    // Out-of-range BmsState
    TEST_ASSERT_EQUAL_INT(0,
                          std::strcmp("UNKNOWN_BMS_STATE", to_string(static_cast<BmsState>(99))));
    TEST_ASSERT_EQUAL_INT(0,
                          std::strcmp("UNKNOWN_BMS_STATE", to_string(static_cast<BmsState>(255))));
    TEST_ASSERT_TRUE(is_valid_bms_state(BmsState::INIT));
    TEST_ASSERT_TRUE(is_valid_bms_state(BmsState::TRIP_DETONATED));
    TEST_ASSERT_FALSE(is_valid_bms_state(static_cast<BmsState>(8)));
    TEST_ASSERT_FALSE(is_valid_bms_state(static_cast<BmsState>(200)));

    // Valid CellStatus strings
    TEST_ASSERT_EQUAL_INT(0, std::strcmp("ACTIVE", to_string(CellStatus::ACTIVE)));
    TEST_ASSERT_EQUAL_INT(0, std::strcmp("BALANCING", to_string(CellStatus::BALANCING)));
    TEST_ASSERT_EQUAL_INT(0, std::strcmp("BYPASSED", to_string(CellStatus::BYPASSED)));
    TEST_ASSERT_EQUAL_INT(0, std::strcmp("SWELLING_WARN", to_string(CellStatus::SWELLING_WARN)));
    TEST_ASSERT_EQUAL_INT(0, std::strcmp("FAULTED", to_string(CellStatus::FAULTED)));

    // Out-of-range CellStatus
    TEST_ASSERT_EQUAL_INT(
        0, std::strcmp("UNKNOWN_CELL_STATUS", to_string(static_cast<CellStatus>(5))));
    TEST_ASSERT_EQUAL_INT(
        0, std::strcmp("UNKNOWN_CELL_STATUS", to_string(static_cast<CellStatus>(255))));
    TEST_ASSERT_TRUE(is_valid_cell_status(CellStatus::ACTIVE));
    TEST_ASSERT_TRUE(is_valid_cell_status(CellStatus::FAULTED));
    TEST_ASSERT_FALSE(is_valid_cell_status(static_cast<CellStatus>(5)));

    // Valid FaultCode strings
    TEST_ASSERT_EQUAL_INT(0, std::strcmp("NONE", to_string(FaultCode::NONE)));
    TEST_ASSERT_EQUAL_INT(0, std::strcmp("OVP", to_string(FaultCode::OVP)));
    TEST_ASSERT_EQUAL_INT(0, std::strcmp("UVP", to_string(FaultCode::UVP)));
    TEST_ASSERT_EQUAL_INT(0, std::strcmp("OCP_CHARGE", to_string(FaultCode::OCP_CHARGE)));
    TEST_ASSERT_EQUAL_INT(0, std::strcmp("OCP_DISCHARGE", to_string(FaultCode::OCP_DISCHARGE)));
    TEST_ASSERT_EQUAL_INT(0, std::strcmp("OTC", to_string(FaultCode::OTC)));
    TEST_ASSERT_EQUAL_INT(0, std::strcmp("UTC", to_string(FaultCode::UTC)));
    TEST_ASSERT_EQUAL_INT(0, std::strcmp("SHORT_CIRCUIT", to_string(FaultCode::SHORT_CIRCUIT)));
    TEST_ASSERT_EQUAL_INT(0, std::strcmp("THERMAL_RUNAWAY", to_string(FaultCode::THERMAL_RUNAWAY)));
    TEST_ASSERT_EQUAL_INT(
        0, std::strcmp("SWELLING_CRITICAL", to_string(FaultCode::SWELLING_CRITICAL)));
    TEST_ASSERT_EQUAL_INT(0, std::strcmp("COMM_TIMEOUT", to_string(FaultCode::COMM_TIMEOUT)));

    // Out-of-range FaultCode
    TEST_ASSERT_EQUAL_INT(0,
                          std::strcmp("UNKNOWN_FAULT", to_string(static_cast<FaultCode>(0x8000))));

    // MetricValidity strings
    TEST_ASSERT_EQUAL_INT(0, std::strcmp("VALID", to_string(MetricValidity::VALID)));
    TEST_ASSERT_EQUAL_INT(0, std::strcmp("STALE", to_string(MetricValidity::STALE)));
    TEST_ASSERT_EQUAL_INT(0, std::strcmp("FAULT_COMM", to_string(MetricValidity::FAULT_COMM)));
    TEST_ASSERT_EQUAL_INT(0, std::strcmp("CALIBRATING", to_string(MetricValidity::CALIBRATING)));
    TEST_ASSERT_EQUAL_INT(
        0, std::strcmp("UNKNOWN_VALIDITY", to_string(static_cast<MetricValidity>(0x80))));
}

/* ============================================================================
 * Core Types Test Suite Runner
 * ============================================================================ */

void run_types_tests(void) {
    RUN_TEST(test_type_sizes_and_alignment);
    RUN_TEST(test_pyro_trigger_key_lifecycle_and_timing);
    RUN_TEST(test_enum_values_and_constants);
    RUN_TEST(test_fault_mask_operations);
    RUN_TEST(test_validity_mask_operations);
    RUN_TEST(test_struct_initialization_and_memcpy);
    RUN_TEST(test_stringification_and_fuzzing);
}
