// =========================================================================
// ARCHITECTURAL NOTE:
// These classes are strictly pure data containers ("nouns") with zero logic.
// All fields are naturally aligned on 32-bit/16-bit boundaries.
// Zero dynamic memory allocations. Standard layout & trivially copyable.
// =========================================================================
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "modules/types/system_enums.hpp"

namespace bms::modules::drivers {

namespace types = bms::modules::types;

/* ============================================================================
 *  W25Q128JV SPI NOR Flash Opcodes & Geometry Constants
 * ============================================================================ */

namespace w25q128 {
inline constexpr uint8_t CMD_WRITE_ENABLE = 0x06;
inline constexpr uint8_t CMD_WRITE_DISABLE = 0x04;
inline constexpr uint8_t CMD_READ_STATUS_1 = 0x05;
inline constexpr uint8_t CMD_READ_STATUS_2 = 0x35;
inline constexpr uint8_t CMD_WRITE_STATUS_1 = 0x01;
inline constexpr uint8_t CMD_PAGE_PROGRAM = 0x02;
inline constexpr uint8_t CMD_SECTOR_ERASE_4K = 0x20;
inline constexpr uint8_t CMD_ERASE_SUSPEND = 0x75;
inline constexpr uint8_t CMD_ERASE_RESUME = 0x7A;
inline constexpr uint8_t CMD_BLOCK_ERASE_64K = 0xD8;
inline constexpr uint8_t CMD_CHIP_ERASE = 0xC7;
inline constexpr uint8_t CMD_READ_DATA = 0x03;
inline constexpr uint8_t CMD_FAST_READ = 0x0B;
inline constexpr uint8_t CMD_READ_JEDEC_ID = 0x9F;

// Status Register 1 Bitmasks
inline constexpr uint8_t STATUS_WIP = 1U << 0;  // Write In Progress
inline constexpr uint8_t STATUS_WEL = 1U << 1;  // Write Enable Latch
inline constexpr uint8_t STATUS_SUS = 1U << 7;  // Erase/Program Suspend (Status Register 2)

// Device Identification
inline constexpr uint32_t JEDEC_MANUFACTURER_WINBOND = 0xEF;
inline constexpr uint32_t JEDEC_W25Q128JV_ID = 0xEF4018;   // Winbond JV-IQ/JQ
inline constexpr uint32_t JEDEC_W25Q128JM_ID = 0xEF7018;   // Winbond JM/DTR

// Physical Geometry
inline constexpr size_t PAGE_SIZE_BYTES = 256U;
inline constexpr size_t SECTOR_SIZE_BYTES = 4096U;
inline constexpr size_t BLOCK_SIZE_BYTES = 65536U;
inline constexpr size_t TOTAL_CAPACITY_BYTES = 16777216U;  // 16 MB
inline constexpr size_t TOTAL_SECTOR_COUNT = 4096U;

// System Partition Boundaries
inline constexpr size_t CONFIG_SECTOR_A = 0U;
inline constexpr size_t CONFIG_SECTOR_B = 1U;
inline constexpr size_t STATS_SECTOR_A = 2U;
inline constexpr size_t STATS_SECTOR_B = 3U;

inline constexpr size_t LOG_START_SECTOR = 4U;  // Sectors 0-3 strictly reserved
inline constexpr size_t TOTAL_LOG_SECTORS = TOTAL_SECTOR_COUNT - LOG_START_SECTOR;  // 4092 sectors
inline constexpr size_t RECORD_SIZE_BYTES = 64U;
inline constexpr size_t RECORDS_PER_SECTOR = SECTOR_SIZE_BYTES / RECORD_SIZE_BYTES;  // 64 records
inline constexpr size_t RECORDS_PER_PAGE = PAGE_SIZE_BYTES / RECORD_SIZE_BYTES;      // 4 records
inline constexpr size_t MAX_FLASH_RECORDS = TOTAL_LOG_SECTORS * RECORDS_PER_SECTOR;  // 261,888

// Magic & Reserved Markers
inline constexpr uint16_t FLASH_LOG_MAGIC_HEADER = 0xAA55;
inline constexpr uint16_t FLASH_FREEZE_MAGIC_HEADER = 0x5A5A;
inline constexpr uint8_t FLASH_LOG_SCHEMA_VERSION = 0x01;
inline constexpr uint32_t SEQUENCE_ID_ERASED = 0xFFFFFFFF;
inline constexpr uint32_t SEQUENCE_ID_MAX_VALID = 0xFFFFFFFE;
// Freeze Marker Partition
inline constexpr size_t FREEZE_MARKER_SECTOR = 1U;
inline constexpr size_t FREEZE_MARKER_ADDRESS = FREEZE_MARKER_SECTOR * SECTOR_SIZE_BYTES;
}  // namespace w25q128

/* ============================================================================
 *  64-Byte Canonical Flash Log Record (POD, Zero Padding Holes)
 * ============================================================================ */

/**
 * @brief High-density packed 64-byte flight data record.
 * Naturally aligned (4-byte -> 2-byte -> 1-byte) with zero implicit compiler padding.
 *
 * NOTE: Sequence IDs represent write order on flash.
 * Offline log analysis should sort by (boot_count, timestamp_ms).
 * timestamp_ms wraps at approximately 49.7 days within a single boot epoch.
 */
struct FlashLogRecord {
    /// Monotonic log sequence index (assigned at dequeue, skips 0xFFFFFFFF)
    uint32_t sequence_id{0U};

    /// Monotonic uptime in milliseconds at sample capture
    uint32_t timestamp_ms{0U};

    /// Instantaneous pack current in milliamps (+ charge, - discharge)
    int32_t pack_current_ma{0};

    /// Pack terminal voltage in millivolts
    uint32_t pack_voltage_mv{0U};

    /// Strongly typed active fault bitmask raw value
    uint32_t active_faults{0U};

    /// Individual cell terminal voltages (mV, 4S pack)
    std::array<uint16_t, 4> cell_voltages_mv{0U, 0U, 0U, 0U};

    /// Minimum cell potential in sampling window (mV)
    uint16_t min_cell_voltage_mv{0U};

    /// Maximum cell potential in sampling window (mV)
    uint16_t max_cell_voltage_mv{0U};

    /// Fixed-point temperature readings (0.1°C resolution: 4 cells + 1 ambient)
    std::array<int16_t, 5> temperatures_deci_c{0, 0, 0, 0, 0};

    /// State of Charge in permille (0 - 1000 => 0.0% - 100.0%)
    uint16_t soc_permille{0U};

    /// State of Health in permille (0 - 1000 => 0.0% - 100.0%)
    uint16_t soh_permille{0U};

    /// Raw casing mechanical expansion ADC readings (4 channels)
    std::array<uint16_t, 4> swelling_force_raw{0U, 0U, 0U, 0U};

    /// Fixed commit validation marker (0xAA55)
    uint16_t magic_header{w25q128::FLASH_LOG_MAGIC_HEADER};

    /// Binary layout schema version (0x01)
    uint8_t record_version{w25q128::FLASH_LOG_SCHEMA_VERSION};

    /// System execution state (BmsState enum value)
    uint8_t bms_state{static_cast<uint8_t>(types::BmsState::INIT)};

    /// Monotonic boot epoch counter (restored and incremented at boot)
    uint16_t boot_count{0U};

    /// Saturating counter of normal records dropped since last written record (0..255)
    uint8_t dropped_normal_count{0U};

    /// Saturating counter of fault records dropped since last written record (0..255)
    uint8_t dropped_fault_count{0U};

    /// CRC-16-CCITT checksum calculated strictly over bytes 0..61
    uint16_t crc16{0U};
};

/* ============================================================================
 *  Freeze Marker Record for Persistent Evidence Protection
 * ============================================================================ */

struct FreezeMarker {
    uint16_t magic_header{w25q128::FLASH_FREEZE_MAGIC_HEADER};
    uint16_t frozen_start_sector{0U};
    uint16_t frozen_end_sector{0U};
    uint16_t reserved{0U};
    uint32_t critical_sequence_id{0U};
    uint32_t timestamp_ms{0U};
};

/* ============================================================================
 *  32-Byte Log-Structured Lifetime Stats & Config PODs (128 records / sector)
 * ============================================================================ */

struct LifetimeStatsRecord {
    uint32_t sequence_id{0U};
    uint32_t total_operating_time_s{0U};
    uint32_t total_charge_mah{0U};
    uint32_t total_discharge_mah{0U};
    uint32_t total_cycle_count_centi{0U};
    int16_t max_cell_temp_deci_c{0};
    int16_t min_cell_temp_deci_c{0};
    uint16_t max_pack_voltage_mv{0U};
    uint16_t min_pack_voltage_mv{0U};
    uint16_t reserved{0U};
    uint16_t crc16{0U};
};

struct SystemConfigRecord {
    uint32_t config_version{1U};
    uint32_t sequence_id{0U};
    uint16_t ovp_threshold_mv{4250U};
    uint16_t uvp_threshold_mv{2800U};
    uint16_t ocp_charge_ma{20000U};
    uint16_t ocp_discharge_ma{40000U};
    int16_t otp_charge_deci_c{450};
    int16_t otp_discharge_deci_c{600};
    int16_t utp_charge_deci_c{0};
    int16_t utp_discharge_deci_c{-200};
    uint16_t max_cell_imbalance_mv{50U};
    uint16_t nominal_capacity_mah{3000U};
    uint16_t reserved{0U};
    uint16_t crc16{0U};
};

/* ============================================================================
 *  Raw Device Telemetry & Runtime Diagnostic Container
 * ============================================================================ */

class FlashLoggerRawData {
private:
    uint32_t jedec_id_{0U};
    uint32_t total_records_logged_{0U};
    uint32_t active_write_address_{w25q128::LOG_START_SECTOR * w25q128::SECTOR_SIZE_BYTES};
    uint32_t torn_records_detected_{0U};
    uint32_t dropped_normal_records_{0U};
    uint32_t dropped_fault_records_{0U};
    uint32_t verify_errors_{0U};
    uint16_t current_boot_count_{1U};
    uint8_t status_register_1_{0U};
    bool is_healthy_{true};
    bool ever_failed_{false};
    bool is_initialized_{false};
    bool is_frozen_{false};

public:
    constexpr FlashLoggerRawData() noexcept = default;

    // Getters
    [[nodiscard]] constexpr uint32_t get_jedec_id() const noexcept { return jedec_id_; }
    [[nodiscard]] constexpr uint32_t get_total_records_logged() const noexcept {
        return total_records_logged_;
    }
    [[nodiscard]] constexpr uint32_t get_active_write_address() const noexcept {
        return active_write_address_;
    }
    [[nodiscard]] constexpr uint32_t get_torn_records_detected() const noexcept {
        return torn_records_detected_;
    }
    [[nodiscard]] constexpr uint32_t get_dropped_normal_records() const noexcept {
        return dropped_normal_records_;
    }
    [[nodiscard]] constexpr uint32_t get_dropped_fault_records() const noexcept {
        return dropped_fault_records_;
    }
    [[nodiscard]] constexpr uint32_t get_verify_errors() const noexcept {
        return verify_errors_;
    }
    [[nodiscard]] constexpr uint16_t get_current_boot_count() const noexcept {
        return current_boot_count_;
    }
    [[nodiscard]] constexpr uint8_t get_status_register_1() const noexcept {
        return status_register_1_;
    }
    [[nodiscard]] constexpr bool is_healthy() const noexcept { return is_healthy_; }
    [[nodiscard]] constexpr bool ever_failed() const noexcept { return ever_failed_; }
    [[nodiscard]] constexpr bool is_initialized() const noexcept { return is_initialized_; }
    [[nodiscard]] constexpr bool is_frozen() const noexcept { return is_frozen_; }

    // Setters
    constexpr void set_jedec_id(uint32_t id) noexcept { jedec_id_ = id; }
    constexpr void set_total_records_logged(uint32_t count) noexcept {
        total_records_logged_ = count;
    }
    constexpr void set_active_write_address(uint32_t addr) noexcept {
        active_write_address_ = addr;
    }
    constexpr void set_torn_records_detected(uint32_t count) noexcept {
        torn_records_detected_ = count;
    }
    constexpr void increment_torn_records() noexcept { ++torn_records_detected_; }
    constexpr void increment_dropped_normal() noexcept { ++dropped_normal_records_; }
    constexpr void increment_dropped_fault() noexcept { ++dropped_fault_records_; }
    constexpr void increment_verify_errors() noexcept { ++verify_errors_; }
    constexpr void set_dropped_normal_records(uint32_t count) noexcept {
        dropped_normal_records_ = count;
    }
    constexpr void set_dropped_fault_records(uint32_t count) noexcept {
        dropped_fault_records_ = count;
    }
    constexpr void set_current_boot_count(uint16_t count) noexcept {
        current_boot_count_ = count;
    }
    constexpr void set_status_register_1(uint8_t status) noexcept {
        status_register_1_ = status;
    }
    constexpr void set_healthy(bool healthy) noexcept {
        is_healthy_ = healthy;
        if (!healthy) ever_failed_ = true;
    }
    constexpr void set_initialized(bool init) noexcept { is_initialized_ = init; }
    constexpr void set_frozen(bool frozen) noexcept { is_frozen_ = frozen; }
};

/* ============================================================================
 *  Compile-Time Safety & Layout Invariants
 * ============================================================================ */

static_assert(w25q128::LOG_START_SECTOR == 4U, "Sectors 0-3 must be strictly reserved");
static_assert(w25q128::MAX_FLASH_RECORDS == 261888U, "Flash log capacity calculation mismatch");
static_assert(sizeof(FlashLogRecord) == 64U, "FlashLogRecord must be exactly 64 bytes");
static_assert(offsetof(FlashLogRecord, boot_count) == 58U, "boot_count offset must be exactly 58");
static_assert(offsetof(FlashLogRecord, dropped_normal_count) == 60U,
              "dropped_normal_count offset must be exactly 60");
static_assert(offsetof(FlashLogRecord, dropped_fault_count) == 61U,
              "dropped_fault_count offset must be exactly 61");
static_assert(offsetof(FlashLogRecord, crc16) == 62U, "crc16 offset must be exactly 62");
static_assert(std::is_standard_layout_v<FlashLogRecord>, "Must be standard layout");
static_assert(std::is_trivially_copyable_v<FlashLogRecord>, "Must be trivially copyable");
static_assert(std::has_unique_object_representations_v<FlashLogRecord>,
              "FlashLogRecord must contain zero implicit padding bits for deterministic hashing");

static_assert(sizeof(LifetimeStatsRecord) == 32U, "LifetimeStatsRecord must be exactly 32 bytes");
static_assert(offsetof(LifetimeStatsRecord, crc16) == 30U, "crc16 offset must be exactly 30");
static_assert(std::is_standard_layout_v<LifetimeStatsRecord>, "Must be standard layout");
static_assert(std::is_trivially_copyable_v<LifetimeStatsRecord>, "Must be trivially copyable");

static_assert(sizeof(SystemConfigRecord) == 32U, "SystemConfigRecord must be exactly 32 bytes");
static_assert(offsetof(SystemConfigRecord, crc16) == 30U, "crc16 offset must be exactly 30");
static_assert(std::is_standard_layout_v<SystemConfigRecord>, "Must be standard layout");
static_assert(std::is_trivially_copyable_v<SystemConfigRecord>, "Must be trivially copyable");

static_assert(std::is_standard_layout_v<FlashLoggerRawData>, "Must be standard layout");
static_assert(std::is_trivially_copyable_v<FlashLoggerRawData>, "Must be trivially copyable");

}  // namespace bms::modules::drivers
