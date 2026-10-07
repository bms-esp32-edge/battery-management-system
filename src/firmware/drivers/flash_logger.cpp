#include "firmware/drivers/flash_logger.hpp"

#include <algorithm>
#include <cstring>

namespace bms::firmware::drivers {

namespace w25q = bms::modules::drivers::w25q128;

FlashLogger::FlashLogger(ISpiFlash& flash_driver) noexcept : flash_(flash_driver) {}

uint16_t FlashLogger::calculate_crc16(const uint8_t* data, size_t length) noexcept {
    // CRC-16-CCITT (Polynomial: 0x1021, Initial value: 0xFFFF)
    uint16_t crc = 0xFFFFU;
    for (size_t i = 0; i < length; ++i) {
        crc ^= static_cast<uint16_t>(data[i]) << 8U;
        for (uint8_t bit = 0; bit < 8U; ++bit) {
            if ((crc & 0x8000U) != 0U) {
                crc = static_cast<uint16_t>((crc << 1U) ^ 0x1021U);
            } else {
                crc = static_cast<uint16_t>(crc << 1U);
            }
        }
    }
    return crc;
}

bool FlashLogger::is_slot_empty(const uint8_t* slot_bytes) noexcept {
    for (size_t i = 0; i < w25q::RECORD_SIZE_BYTES; ++i) {
        if (slot_bytes[i] != 0xFFU) {
            return false;
        }
    }
    return true;
}

bool FlashLogger::is_slot_valid(const data::FlashLogRecord& record) noexcept {
    if (record.magic_header != w25q::FLASH_LOG_MAGIC_HEADER) {
        return false;
    }
    const auto* byte_ptr = reinterpret_cast<const uint8_t*>(&record);
    const uint16_t calculated = calculate_crc16(byte_ptr, offsetof(data::FlashLogRecord, crc16));
    return calculated == record.crc16;
}

uint32_t FlashLogger::get_computed_record_count() const noexcept {
    if (!raw_data_.is_initialized())
        return 0;
    uint32_t total_sectors = 0;
    if (head_sector_ >= oldest_sector_) {
        total_sectors = head_sector_ - oldest_sector_;
    } else {
        total_sectors = w25q::TOTAL_LOG_SECTORS - (oldest_sector_ - head_sector_);
    }
    return (total_sectors * w25q::RECORDS_PER_SECTOR) + head_slot_;
}

bool FlashLogger::is_sector_clean(uint32_t sector_index) noexcept {
    if (sector_index < w25q::LOG_START_SECTOR || sector_index >= w25q::TOTAL_SECTOR_COUNT) {
        return false;
    }
    alignas(4) uint8_t chk_buf[w25q::PAGE_SIZE_BYTES];
    for (size_t p = 0; p < (w25q::SECTOR_SIZE_BYTES / w25q::PAGE_SIZE_BYTES); ++p) {
        const uint32_t p_addr =
            (sector_index * w25q::SECTOR_SIZE_BYTES) + (p * w25q::PAGE_SIZE_BYTES);
        if (flash_.read(p_addr, chk_buf, sizeof(chk_buf)) != SpiFlashStatus::OK) {
            raw_data_.set_healthy(false);
            return false;
        }
        for (size_t b = 0; b < w25q::PAGE_SIZE_BYTES; ++b) {
            if (chk_buf[b] != 0xFFU) {
                return false;
            }
        }
    }
    return true;
}

bool FlashLogger::reclaim_sector(uint32_t sector_index, bool& was_erased) noexcept {
    was_erased = false;
    if (sector_index < w25q::LOG_START_SECTOR || sector_index >= w25q::TOTAL_SECTOR_COUNT) {
        return false;
    }

    if (is_frozen_ && sector_index >= frozen_start_sector_ && sector_index <= frozen_end_sector_) {
        return false;
    }

    if (!is_sector_clean(sector_index)) {
        if (flash_.erase_sector(sector_index * w25q::SECTOR_SIZE_BYTES) != SpiFlashStatus::OK) {
            raw_data_.set_healthy(false);
            return false;
        }
        was_erased = true;
        // Advance oldest_sector_ ONLY if an erase destroyed data at oldest_sector_
        if (sector_index == oldest_sector_) {
            oldest_sector_ = (oldest_sector_ + 1U >= w25q::TOTAL_SECTOR_COUNT)
                                 ? w25q::LOG_START_SECTOR
                                 : (oldest_sector_ + 1U);
        }
    }
    return true;
}

void FlashLogger::advance_head(uint32_t n_slots, bool slots_are_good) noexcept {
    if (slots_are_good) {
        raw_data_.set_total_records_logged(raw_data_.get_total_records_logged() + n_slots);
    }
    head_slot_ += n_slots;
    if (head_slot_ >= w25q::RECORDS_PER_SECTOR) {
        uint32_t next_sec = head_sector_ + 1U;
        if (next_sec >= w25q::TOTAL_SECTOR_COUNT) {
            next_sec = w25q::LOG_START_SECTOR;
        }
        if (is_frozen_ && next_sec >= frozen_start_sector_ && next_sec <= frozen_end_sector_) {
            // Stop head from entering persistently frozen evidence sectors
            head_sector_ = next_sec;
            head_slot_ = 0;
            return;
        }
        head_sector_ = next_sec;
        head_slot_ = 0;
        head_sector_clean_ = next_sector_erased_;
        next_sector_erased_ = false;
    }
    raw_data_.set_active_write_address((head_sector_ * w25q::SECTOR_SIZE_BYTES) +
                                       (head_slot_ * w25q::RECORD_SIZE_BYTES));
}

bool FlashLogger::freeze_evidence(uint16_t start_sec, uint16_t end_sec, uint32_t critical_seq,
                                  uint32_t timestamp_ms) noexcept {
    data::FreezeMarker marker{};
    marker.magic_header = w25q::FLASH_FREEZE_MAGIC_HEADER;
    marker.frozen_start_sector = start_sec;
    marker.frozen_end_sector = end_sec;
    marker.critical_sequence_id = critical_seq;
    marker.timestamp_ms = timestamp_ms;

    if (flash_.program_page(w25q::FREEZE_MARKER_ADDRESS, reinterpret_cast<const uint8_t*>(&marker),
                            sizeof(marker)) != SpiFlashStatus::OK) {
        raw_data_.set_healthy(false);
        return false;
    }
    is_frozen_ = true;
    frozen_start_sector_ = start_sec;
    frozen_end_sector_ = end_sec;
    raw_data_.set_frozen(true);
    return true;
}

bool FlashLogger::clear_freeze() noexcept {
    if (flash_.erase_sector(w25q::FREEZE_MARKER_ADDRESS) != SpiFlashStatus::OK) {
        raw_data_.set_healthy(false);
        return false;
    }
    is_frozen_ = false;
    frozen_start_sector_ = 0;
    frozen_end_sector_ = 0;
    raw_data_.set_frozen(false);
    return true;
}

bool FlashLogger::scan_sector_heads(uint32_t& best_sector, uint32_t& best_seq,
                                    uint32_t& lowest_sector, uint32_t& lowest_seq,
                                    uint16_t& highest_epoch, bool& found_valid) noexcept {
    found_valid = false;
    best_sector = w25q::LOG_START_SECTOR;
    best_seq = 0;
    lowest_sector = w25q::LOG_START_SECTOR;
    lowest_seq = 0;
    highest_epoch = 0;

    alignas(4) data::FlashLogRecord page0_records[w25q::RECORDS_PER_PAGE];

    // Helper to inspect page 0 of a given sector
    auto inspect_sector = [&](uint32_t s, data::FlashLogRecord& out_rec) -> bool {
        const uint32_t addr = s * w25q::SECTOR_SIZE_BYTES;
        if (flash_.read(addr, reinterpret_cast<uint8_t*>(page0_records), sizeof(page0_records)) !=
            SpiFlashStatus::OK) {
            raw_data_.set_healthy(false);
            return false;
        }
        for (size_t r = 0; r < w25q::RECORDS_PER_PAGE; ++r) {
            if (is_slot_valid(page0_records[r])) {
                out_rec = page0_records[r];
                return true;
            }
        }
        return false;
    };

    // Full linear scan fallback lambda
    auto run_full_scan = [&]() -> bool {
        found_valid = false;
        for (uint32_t s = w25q::LOG_START_SECTOR; s < w25q::TOTAL_SECTOR_COUNT; ++s) {
            data::FlashLogRecord valid_rec{};
            if (inspect_sector(s, valid_rec)) {
                if (!found_valid) {
                    found_valid = true;
                    best_sector = s;
                    best_seq = valid_rec.sequence_id;
                    lowest_sector = s;
                    lowest_seq = valid_rec.sequence_id;
                    highest_epoch = valid_rec.boot_count;
                } else {
                    if (static_cast<int32_t>(valid_rec.sequence_id - best_seq) > 0) {
                        best_sector = s;
                        best_seq = valid_rec.sequence_id;
                        highest_epoch = valid_rec.boot_count;
                    }
                    if (static_cast<int32_t>(valid_rec.sequence_id - lowest_seq) < 0) {
                        lowest_sector = s;
                        lowest_seq = valid_rec.sequence_id;
                    }
                }
            }
        }
        return true;
    };

    // Attempt accelerated binary search across sectors 4..4095
    data::FlashLogRecord start_rec{};
    data::FlashLogRecord end_rec{};
    const bool start_valid = inspect_sector(w25q::LOG_START_SECTOR, start_rec);
    const bool end_valid = inspect_sector(w25q::TOTAL_SECTOR_COUNT - 1U, end_rec);

    if (!start_valid && !end_valid) {
        // Flash might be entirely empty, or wrapped with empty boundary. Check full scan.
        return run_full_scan();
    }

    if (start_valid && !end_valid) {
        // Unwrapped monotonic log: binary search for transition from valid to invalid
        uint32_t low = w25q::LOG_START_SECTOR;
        uint32_t high = w25q::TOTAL_SECTOR_COUNT - 1U;
        uint32_t last_valid = low;
        data::FlashLogRecord last_valid_rec = start_rec;

        while (low <= high) {
            uint32_t mid = low + ((high - low) / 2U);
            data::FlashLogRecord mid_rec{};
            if (inspect_sector(mid, mid_rec)) {
                last_valid = mid;
                last_valid_rec = mid_rec;
                low = mid + 1U;
            } else {
                if (mid == 0)
                    break;
                high = mid - 1U;
            }
        }

        // Validate monotonicity
        if (static_cast<int32_t>(last_valid_rec.sequence_id - start_rec.sequence_id) >= 0) {
            found_valid = true;
            best_sector = last_valid;
            best_seq = last_valid_rec.sequence_id;
            lowest_sector = w25q::LOG_START_SECTOR;
            lowest_seq = start_rec.sequence_id;
            highest_epoch = last_valid_rec.boot_count;
            return true;
        }
    }

    // Default to full scan for wrapped or complex discontinuous configurations
    return run_full_scan();
}

bool FlashLogger::write_boot_event_record() noexcept {
    data::FlashLogRecord boot_rec{};
    boot_rec.sequence_id = next_sequence_id_++;
    if (next_sequence_id_ == w25q::SEQUENCE_ID_ERASED) {
        next_sequence_id_ = 0U;
    }
    boot_rec.timestamp_ms = 0U;
    boot_rec.pack_current_ma = 0;
    boot_rec.pack_voltage_mv = 0U;
    boot_rec.active_faults = 0U;
    boot_rec.cell_voltages_mv = {0U, 0U, 0U, 0U};
    boot_rec.min_cell_voltage_mv = 0U;
    boot_rec.max_cell_voltage_mv = 0U;
    boot_rec.temperatures_deci_c = {0, 0, 0, 0, 0};
    boot_rec.soc_permille = 0U;
    boot_rec.soh_permille = 0U;
    boot_rec.swelling_force_raw = {0U, 0U, 0U, 0U};
    boot_rec.magic_header = w25q::FLASH_LOG_MAGIC_HEADER;
    boot_rec.record_version = w25q::FLASH_LOG_SCHEMA_VERSION;
    boot_rec.bms_state = static_cast<uint8_t>(types::BmsState::INIT);
    boot_rec.boot_count = raw_data_.get_current_boot_count();
    boot_rec.dropped_normal_count = 0U;
    boot_rec.dropped_fault_count = 0U;

    const auto* byte_ptr = reinterpret_cast<const uint8_t*>(&boot_rec);
    boot_rec.crc16 = calculate_crc16(byte_ptr, offsetof(data::FlashLogRecord, crc16));

    page_buffer_[buffered_records_++] = boot_rec;
    return program_buffered_page(false);
}

bool FlashLogger::init() noexcept {
    raw_data_.set_initialized(false);
    raw_data_.set_healthy(true);
    buffered_records_ = 0;
    next_sector_erased_ = false;
    head_sector_clean_ = false;
    is_frozen_ = false;
    frozen_start_sector_ = 0;
    frozen_end_sector_ = 0;
    consecutive_good_verifies_ = 0;
    pending_dropped_normal_ = 0;
    pending_dropped_fault_ = 0;
    previous_bms_state_ = static_cast<uint8_t>(types::BmsState::STANDBY);
    normal_queue_.clear();
    fault_queue_.clear();

    if (flash_.init() != SpiFlashStatus::OK) {
        raw_data_.set_healthy(false);
        return false;
    }

    uint32_t jedec_id = 0;
    if (flash_.read_jedec_id(&jedec_id) != SpiFlashStatus::OK) {
        raw_data_.set_healthy(false);
        return false;
    }
    const uint8_t mfg = static_cast<uint8_t>((jedec_id >> 16U) & 0xFFU);
    const uint8_t mem_type = static_cast<uint8_t>((jedec_id >> 8U) & 0xFFU);
    const uint8_t cap = static_cast<uint8_t>(jedec_id & 0xFFU);

    // Accept JV (0xEF4018) and JM (0xEF7018) variants, reject invalid manufacturer/capacity
    if (mfg != 0xEFU || cap != 0x18U || (mem_type != 0x40U && mem_type != 0x70U)) {
        raw_data_.set_healthy(false);
        return false;
    }
    raw_data_.set_jedec_id(jedec_id);

    // Check for persistent evidence freeze marker in pre-erased sector 1
    alignas(4) data::FreezeMarker freeze_marker{};
    if (flash_.read(w25q::FREEZE_MARKER_ADDRESS, reinterpret_cast<uint8_t*>(&freeze_marker),
                    sizeof(freeze_marker)) == SpiFlashStatus::OK) {
        if (freeze_marker.magic_header == w25q::FLASH_FREEZE_MAGIC_HEADER) {
            is_frozen_ = true;
            frozen_start_sector_ = freeze_marker.frozen_start_sector;
            frozen_end_sector_ = freeze_marker.frozen_end_sector;
            raw_data_.set_frozen(true);
        }
    }

    // Step 1: Scan sector heads (page 0)
    bool found_any_valid_sector = false;
    uint32_t best_sector = w25q::LOG_START_SECTOR;
    uint32_t best_sector_seq = 0;
    uint32_t lowest_sector = w25q::LOG_START_SECTOR;
    uint32_t lowest_sector_seq = 0;
    uint16_t highest_epoch = 0;

    scan_sector_heads(best_sector, best_sector_seq, lowest_sector, lowest_sector_seq, highest_epoch,
                      found_any_valid_sector);

    oldest_sector_ = found_any_valid_sector ? lowest_sector : w25q::LOG_START_SECTOR;

    // Step 2: Linear scan within the active sector in 256-byte page chunks
    head_sector_ = best_sector;
    head_slot_ = 0;
    next_sequence_id_ = 1;
    uint32_t last_non_empty_slot = 0xFFFFFFFFU;
    bool found_valid_in_active = false;

    alignas(4) uint8_t page_buf[w25q::PAGE_SIZE_BYTES];

    for (size_t p = 0; p < (w25q::SECTOR_SIZE_BYTES / w25q::PAGE_SIZE_BYTES); ++p) {
        const uint32_t page_addr =
            (head_sector_ * w25q::SECTOR_SIZE_BYTES) + (p * w25q::PAGE_SIZE_BYTES);
        if (flash_.read(page_addr, page_buf, sizeof(page_buf)) != SpiFlashStatus::OK) {
            raw_data_.set_healthy(false);
            return false;
        }

        for (size_t r = 0; r < w25q::RECORDS_PER_PAGE; ++r) {
            const size_t slot_idx = (p * w25q::RECORDS_PER_PAGE) + r;
            const uint8_t* slot_bytes = &page_buf[r * w25q::RECORD_SIZE_BYTES];

            if (!is_slot_empty(slot_bytes)) {
                last_non_empty_slot = static_cast<uint32_t>(slot_idx);

                data::FlashLogRecord slot_rec{};
                std::memcpy(&slot_rec, slot_bytes, sizeof(slot_rec));

                if (is_slot_valid(slot_rec)) {
                    found_valid_in_active = true;
                    if (static_cast<int32_t>(slot_rec.sequence_id - next_sequence_id_) >= 0) {
                        next_sequence_id_ = slot_rec.sequence_id + 1U;
                        highest_epoch = slot_rec.boot_count;
                        if (next_sequence_id_ == w25q::SEQUENCE_ID_ERASED) {
                            next_sequence_id_ = 0U;
                        }
                    }
                } else {
                    raw_data_.increment_torn_records();
                }
            }
        }
    }

    // Step 3: Write head is strictly one past the last non-empty slot
    if (last_non_empty_slot == 0xFFFFFFFFU) {
        head_slot_ = 0;
        head_sector_clean_ = true;
    } else if (last_non_empty_slot + 1U < w25q::RECORDS_PER_SECTOR) {
        head_slot_ = last_non_empty_slot + 1U;
        head_sector_clean_ = true;
    } else {
        // Active sector is completely full, advance to next sector
        head_sector_ = head_sector_ + 1U;
        if (head_sector_ >= w25q::TOTAL_SECTOR_COUNT) {
            head_sector_ = w25q::LOG_START_SECTOR;
        }
        head_slot_ = 0;
        head_sector_clean_ = false;  // Must verify clean before write
    }

    // Set boot epoch counter from the highest-sequence record
    const uint16_t new_boot_count = (found_any_valid_sector || found_valid_in_active)
                                        ? static_cast<uint16_t>(highest_epoch + 1U)
                                        : 1U;
    raw_data_.set_current_boot_count((new_boot_count == 0U) ? 1U : new_boot_count);

    // Step 4: Level-based pre-erase check at boot
    evaluate_pre_erase_level();

    raw_data_.set_active_write_address((head_sector_ * w25q::SECTOR_SIZE_BYTES) +
                                       (head_slot_ * w25q::RECORD_SIZE_BYTES));
    raw_data_.set_initialized(true);

    // Step 5: Write boot-event record so incremented epoch reaches flash in brownout loops
    write_boot_event_record();

    return true;
}

void FlashLogger::evaluate_pre_erase_level() noexcept {
    if (head_slot_ >= 48U && !next_sector_erased_) {
        uint32_t next_s = head_sector_ + 1U;
        if (next_s >= w25q::TOTAL_SECTOR_COUNT) {
            next_s = w25q::LOG_START_SECTOR;
        }

        bool erased = false;
        if (reclaim_sector(next_s, erased)) {
            next_sector_erased_ = true;
        }
    }
}

bool FlashLogger::write(const LogPayload& payload) noexcept {
    if (!raw_data_.is_initialized()) {
        raw_data_.increment_dropped_normal();
        return false;
    }

    if (is_frozen_ && head_sector_ >= frozen_start_sector_ && head_sector_ <= frozen_end_sector_) {
        return false;
    }

    const bool is_fault =
        (payload.active_faults != 0U) || (payload.bms_state != previous_bms_state_);
    previous_bms_state_ = payload.bms_state;

    if (is_fault) {
        // Rate-limit identical repeated faults to prevent flood
        if (payload.active_faults != 0U && payload.active_faults == last_fault_mask_ &&
            (payload.timestamp_ms - last_fault_timestamp_ms_ < FAULT_RATE_LIMIT_MS)) {
            if (!normal_queue_.push(payload)) {
                raw_data_.increment_dropped_fault();
                if (pending_dropped_fault_ < 255U)
                    ++pending_dropped_fault_;
                return false;
            }
            return true;
        }

        last_fault_mask_ = payload.active_faults;
        last_fault_timestamp_ms_ = payload.timestamp_ms;

        if (!fault_queue_.push(payload)) {
            if (!normal_queue_.push(payload)) {
                raw_data_.increment_dropped_fault();
                if (pending_dropped_fault_ < 255U)
                    ++pending_dropped_fault_;
                return false;
            }
        }
    } else {
        if (!normal_queue_.push(payload)) {
            raw_data_.increment_dropped_normal();
            if (pending_dropped_normal_ < 255U)
                ++pending_dropped_normal_;
            return false;
        }
    }
    return true;
}

bool FlashLogger::step() noexcept {
    if (!raw_data_.is_initialized())
        return false;

    LogPayload payload{};
    bool from_fault_queue = false;

    if (fault_queue_.pop(payload)) {
        from_fault_queue = true;
    } else if (normal_queue_.pop(payload)) {
        from_fault_queue = false;
    } else {
        return false;  // Idle
    }

    // Assemble serialized record
    data::FlashLogRecord record{};
    record.sequence_id = next_sequence_id_++;
    if (next_sequence_id_ == w25q::SEQUENCE_ID_ERASED) {
        next_sequence_id_ = 0U;
    }

    record.timestamp_ms = payload.timestamp_ms;
    record.pack_current_ma = payload.pack_current_ma;
    record.pack_voltage_mv = payload.pack_voltage_mv;
    record.active_faults = payload.active_faults;
    record.cell_voltages_mv = payload.cell_voltages_mv;
    record.min_cell_voltage_mv = payload.min_cell_voltage_mv;
    record.max_cell_voltage_mv = payload.max_cell_voltage_mv;
    record.temperatures_deci_c = payload.temperatures_deci_c;
    record.soc_permille = payload.soc_permille;
    record.soh_permille = payload.soh_permille;
    record.swelling_force_raw = payload.swelling_force_raw;
    record.magic_header = w25q::FLASH_LOG_MAGIC_HEADER;
    record.record_version = w25q::FLASH_LOG_SCHEMA_VERSION;
    record.bms_state = payload.bms_state;
    record.boot_count = raw_data_.get_current_boot_count();

    // Stamp in-record saturating drop counters
    record.dropped_normal_count = pending_dropped_normal_;
    record.dropped_fault_count = pending_dropped_fault_;
    pending_dropped_normal_ = 0U;
    pending_dropped_fault_ = 0U;

    const auto* byte_ptr = reinterpret_cast<const uint8_t*>(&record);
    record.crc16 = calculate_crc16(byte_ptr, offsetof(data::FlashLogRecord, crc16));

    if (buffered_records_ >= w25q::RECORDS_PER_PAGE) {
        buffered_records_ = 0;  // Overflow guard
    }

    page_buffer_[buffered_records_++] = record;

    // Trigger program when room in current page is filled or fault record arrived
    const uint32_t room_in_page = w25q::RECORDS_PER_PAGE - (head_slot_ % w25q::RECORDS_PER_PAGE);
    if (buffered_records_ >= room_in_page || from_fault_queue) {
        program_buffered_page(from_fault_queue);
    }

    evaluate_pre_erase_level();
    return true;
}

bool FlashLogger::program_buffered_page(bool is_fault) noexcept {
    if (buffered_records_ == 0U)
        return true;

    if (is_frozen_ && head_sector_ >= frozen_start_sector_ && head_sector_ <= frozen_end_sector_) {
        buffered_records_ = 0;
        return false;
    }

    // Ensure sector is clean before first write to slot 0
    if (head_slot_ == 0U && !head_sector_clean_) {
        bool was_erased = false;
        if (!reclaim_sector(head_sector_, was_erased)) {
            raw_data_.set_healthy(false);
            buffered_records_ = 0;
            return false;
        }
        head_sector_clean_ = true;
    }

    // Erase Suspend / Resume handling for fault records
    bool was_erase_suspended = false;
    if (is_fault) {
        uint8_t st = 0;
        flash_.read_status(&st);
        if ((st & w25q::STATUS_WIP) != 0U || flash_.is_erase_suspended()) {
            flash_.suspend_erase();
            uint32_t poll_count = 0;
            while (poll_count++ < 1000U) {
                flash_.read_status(&st);
                if ((st & w25q::STATUS_WIP) == 0U) {
                    break;
                }
            }
            was_erase_suspended = true;
        }
    }

    const uint32_t write_addr =
        (head_sector_ * w25q::SECTOR_SIZE_BYTES) + (head_slot_ * w25q::RECORD_SIZE_BYTES);
    const size_t bytes_to_write = buffered_records_ * w25q::RECORD_SIZE_BYTES;
    const size_t records_attempted = buffered_records_;

    SpiFlashStatus p_status = flash_.program_page(
        write_addr, reinterpret_cast<const uint8_t*>(page_buffer_.data()), bytes_to_write);

    if (p_status != SpiFlashStatus::OK) {
        raw_data_.set_healthy(false);
        raw_data_.increment_verify_errors();
        consecutive_good_verifies_ = 0;
        buffered_records_ = 0;
        advance_head(static_cast<uint32_t>(records_attempted), false);
        if (was_erase_suspended) {
            flash_.resume_erase();
        }
        return false;
    }

    // Read-back verification
    alignas(4) uint8_t verify_buf[w25q::PAGE_SIZE_BYTES];
    bool verify_passed = true;
    if (flash_.read(write_addr, verify_buf, bytes_to_write) != SpiFlashStatus::OK ||
        std::memcmp(verify_buf, page_buffer_.data(), bytes_to_write) != 0) {
        verify_passed = false;
        raw_data_.set_healthy(false);
        raw_data_.increment_verify_errors();
        consecutive_good_verifies_ = 0;
    }

    if (verify_passed) {
        consecutive_good_verifies_++;
        if (consecutive_good_verifies_ >= REQUIRED_GOOD_VERIFIES_FOR_RECOVERY) {
            raw_data_.set_healthy(true);
        }
        buffered_records_ = 0;
        advance_head(static_cast<uint32_t>(records_attempted), true);
    } else {
        // Readback mismatch: mark slot bad and re-program if fault record
        advance_head(static_cast<uint32_t>(records_attempted), false);
        if (is_fault) {
            // Re-evaluate sector boundaries and frozen state for the retry
            if (is_frozen_ && head_sector_ >= frozen_start_sector_ &&
                head_sector_ <= frozen_end_sector_) {
                buffered_records_ = 0;
                if (was_erase_suspended)
                    flash_.resume_erase();
                return false;
            }
            if (head_slot_ == 0U && !head_sector_clean_) {
                bool was_erased = false;
                if (!reclaim_sector(head_sector_, was_erased)) {
                    buffered_records_ = 0;
                    if (was_erase_suspended)
                        flash_.resume_erase();
                    return false;
                }
                head_sector_clean_ = true;
            }
            const uint32_t retry_addr =
                (head_sector_ * w25q::SECTOR_SIZE_BYTES) + (head_slot_ * w25q::RECORD_SIZE_BYTES);
            if (flash_.program_page(retry_addr,
                                    reinterpret_cast<const uint8_t*>(page_buffer_.data()),
                                    bytes_to_write) == SpiFlashStatus::OK) {
                if (flash_.read(retry_addr, verify_buf, bytes_to_write) == SpiFlashStatus::OK &&
                    std::memcmp(verify_buf, page_buffer_.data(), bytes_to_write) == 0) {
                    consecutive_good_verifies_++;
                    if (consecutive_good_verifies_ >= REQUIRED_GOOD_VERIFIES_FOR_RECOVERY) {
                        raw_data_.set_healthy(true);
                    }
                    advance_head(static_cast<uint32_t>(records_attempted), true);
                } else {
                    advance_head(static_cast<uint32_t>(records_attempted), false);
                }
            } else {
                advance_head(static_cast<uint32_t>(records_attempted), false);
            }
        }
        buffered_records_ = 0;
    }

    if (was_erase_suspended) {
        flash_.resume_erase();
    }
    return verify_passed;
}

void FlashLogger::flush() noexcept {
    while (step()) {
        // Drain all pending records
    }
    if (buffered_records_ > 0U) {
        program_buffered_page(false);
    }
}

bool FlashLogger::read_record(uint32_t logical_index, data::FlashLogRecord& out_record) noexcept {
    if (!raw_data_.is_initialized())
        return false;
    const uint32_t record_count = get_computed_record_count();
    if (logical_index >= record_count)
        return false;

    // Calculate physical sector and slot
    uint32_t target_sector = oldest_sector_ + (logical_index / w25q::RECORDS_PER_SECTOR);
    if (target_sector >= w25q::TOTAL_SECTOR_COUNT) {
        target_sector = w25q::LOG_START_SECTOR + (target_sector - w25q::TOTAL_SECTOR_COUNT);
    }
    const uint32_t target_slot = logical_index % w25q::RECORDS_PER_SECTOR;
    const uint32_t addr =
        (target_sector * w25q::SECTOR_SIZE_BYTES) + (target_slot * w25q::RECORD_SIZE_BYTES);

    if (flash_.read(addr, reinterpret_cast<uint8_t*>(&out_record), sizeof(out_record)) !=
        SpiFlashStatus::OK) {
        return false;
    }

    return is_slot_valid(out_record);
}

bool FlashLogger::read_latest(data::FlashLogRecord& out_record) noexcept {
    const uint32_t count = get_computed_record_count();
    if (!raw_data_.is_initialized() || count == 0U)
        return false;

    // Search backward from write head up to min(64, count) slots
    const size_t max_lookback = std::min(w25q::RECORDS_PER_SECTOR, static_cast<size_t>(count));
    uint32_t curr_sector = head_sector_;
    int32_t curr_slot = static_cast<int32_t>(head_slot_) - 1;

    for (size_t i = 0; i < max_lookback; ++i) {
        if (curr_slot < 0) {
            curr_sector = (curr_sector > w25q::LOG_START_SECTOR) ? (curr_sector - 1U)
                                                                 : (w25q::TOTAL_SECTOR_COUNT - 1U);
            curr_slot = static_cast<int32_t>(w25q::RECORDS_PER_SECTOR - 1U);
        }

        const uint32_t addr = (curr_sector * w25q::SECTOR_SIZE_BYTES) +
                              (static_cast<uint32_t>(curr_slot) * w25q::RECORD_SIZE_BYTES);

        if (flash_.read(addr, reinterpret_cast<uint8_t*>(&out_record), sizeof(out_record)) ==
            SpiFlashStatus::OK) {
            if (is_slot_valid(out_record)) {
                return true;
            }
        }
        --curr_slot;
    }
    return false;
}

bool FlashLogger::erase_all_logs() noexcept {
    for (uint32_t s = w25q::LOG_START_SECTOR; s < w25q::TOTAL_SECTOR_COUNT; ++s) {
        if (is_frozen_ && s >= frozen_start_sector_ && s <= frozen_end_sector_) {
            continue;
        }
        if (flash_.erase_sector(s * w25q::SECTOR_SIZE_BYTES) != SpiFlashStatus::OK) {
            raw_data_.set_healthy(false);
            return false;
        }
    }
    head_sector_ = w25q::LOG_START_SECTOR;
    head_slot_ = 0;
    oldest_sector_ = w25q::LOG_START_SECTOR;
    next_sequence_id_ = 1;
    buffered_records_ = 0;
    next_sector_erased_ = false;
    head_sector_clean_ = true;
    raw_data_.set_total_records_logged(0);
    raw_data_.set_active_write_address(head_sector_ * w25q::SECTOR_SIZE_BYTES);
    return true;
}

}  // namespace bms::firmware::drivers
