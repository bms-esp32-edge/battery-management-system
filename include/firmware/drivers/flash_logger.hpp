#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "firmware/drivers/ispi_flash.hpp"
#include "modules/drivers/flash_logger_data.hpp"
#include "modules/types/system_enums.hpp"

namespace bms::firmware::drivers {

namespace data = bms::modules::drivers;
namespace types = bms::modules::types;

/**
 * @brief Raw sensor payload submitted by producer tasks.
 * Sequence ID, boot epoch, and CRC are computed and stamped by FlashLogger.
 */
struct LogPayload {
    uint32_t timestamp_ms{0U};
    int32_t pack_current_ma{0};
    uint32_t pack_voltage_mv{0U};
    uint32_t active_faults{0U};
    std::array<uint16_t, 4> cell_voltages_mv{0U, 0U, 0U, 0U};
    uint16_t min_cell_voltage_mv{0U};
    uint16_t max_cell_voltage_mv{0U};
    std::array<int16_t, 5> temperatures_deci_c{0, 0, 0, 0, 0};
    uint16_t soc_permille{0U};
    uint16_t soh_permille{0U};
    std::array<uint16_t, 4> swelling_force_raw{0U, 0U, 0U, 0U};
    uint8_t bms_state{static_cast<uint8_t>(types::BmsState::INIT)};
};

/**
 * @brief High-reliability circular ring-buffer flash logger engine.
 *
 * Implements deterministic sector management, two-tier boot recovery,
 * level-based background pre-erasure, and priority fault flushing.
 */
class FlashLogger {
public:
    static constexpr size_t NORMAL_QUEUE_CAPACITY = 28U;
    static constexpr size_t FAULT_QUEUE_CAPACITY = 4U;

    explicit FlashLogger(ISpiFlash& flash_driver) noexcept;
    ~FlashLogger() noexcept = default;

    // Non-copyable, non-movable
    FlashLogger(const FlashLogger&) = delete;
    FlashLogger& operator=(const FlashLogger&) = delete;

    /**
     * @brief Initializes driver, verifies JEDEC ID, scans sector heads and recovers write head.
     */
    bool init() noexcept;

    /**
     * @brief Non-blocking write to queues.
     * Evaluates fault trigger from payload.active_faults != 0 or state change.
     */
    bool write(const LogPayload& payload) noexcept;

    /**
     * @brief Single-step execution for the background logger task or unit tests.
     * Drains one record, manages page buffering, and triggers level-based pre-erase.
     * @return true if an action was processed, false if queues are idle.
     */
    bool step() noexcept;

    /**
     * @brief Drains all queued records and forces unbuffered write of any pending page buffer.
     */
    void flush() noexcept;

    /**
     * @brief Reads a record by logical slot index (0 = oldest valid slot).
     * @return true if record exists, is within bounds, and is strictly VALID (passes CRC).
     */
    bool read_record(uint32_t logical_index, data::FlashLogRecord& out_record) noexcept;

    /**
     * @brief Reads the newest valid record committed at or before the write head.
     */
    bool read_latest(data::FlashLogRecord& out_record) noexcept;

    /**
     * @brief Persistently freezes sector range in NOR flash without erase.
     * Write head will refuse to enter frozen sector range.
     */
    bool freeze_evidence(uint16_t start_sec, uint16_t end_sec, uint32_t critical_seq,
                         uint32_t timestamp_ms) noexcept;

    /**
     * @brief Service command to clear the evidence freeze marker by sector erase.
     */
    bool clear_freeze() noexcept;

    /**
     * @brief Erases all telemetry log sectors (Sectors 4 to 4095).
     */
    bool erase_all_logs() noexcept;

    // State & Diagnostic Getters
    [[nodiscard]] bool is_healthy() const noexcept { return raw_data_.is_healthy(); }
    [[nodiscard]] bool is_initialized() const noexcept { return raw_data_.is_initialized(); }
    [[nodiscard]] bool is_frozen() const noexcept { return is_frozen_; }
    [[nodiscard]] uint16_t get_frozen_start_sector() const noexcept { return frozen_start_sector_; }
    [[nodiscard]] uint16_t get_frozen_end_sector() const noexcept { return frozen_end_sector_; }
    [[nodiscard]] uint32_t get_total_records() const noexcept {
        return get_computed_record_count();
    }
    [[nodiscard]] uint16_t get_boot_count() const noexcept {
        return raw_data_.get_current_boot_count();
    }
    [[nodiscard]] const data::FlashLoggerRawData& get_diagnostics() const noexcept {
        return raw_data_;
    }

    // Geometry Helpers
    [[nodiscard]] uint32_t get_head_sector() const noexcept { return head_sector_; }
    [[nodiscard]] uint32_t get_head_slot() const noexcept { return head_slot_; }
    [[nodiscard]] uint32_t get_oldest_sector() const noexcept { return oldest_sector_; }
    [[nodiscard]] uint32_t get_computed_record_count() const noexcept;

private:
    // Internal Queues for deterministic host testing and FreeRTOS task decoupling
    template <typename T, size_t N>
    class FixedRingQueue {
    public:
        constexpr FixedRingQueue() noexcept = default;
        bool push(const T& item) noexcept {
            if (count_ >= N) return false;
            buffer_[tail_] = item;
            tail_ = (tail_ + 1U) % N;
            ++count_;
            return true;
        }
        bool pop(T& out_item) noexcept {
            if (count_ == 0U) return false;
            out_item = buffer_[head_];
            head_ = (head_ + 1U) % N;
            --count_;
            return true;
        }
        [[nodiscard]] bool is_empty() const noexcept { return count_ == 0U; }
        [[nodiscard]] bool is_full() const noexcept { return count_ >= N; }
        [[nodiscard]] size_t size() const noexcept { return count_; }
        void clear() noexcept {
            head_ = 0;
            tail_ = 0;
            count_ = 0;
        }

    private:
        std::array<T, N> buffer_{};
        size_t head_{0U};
        size_t tail_{0U};
        size_t count_{0U};
    };

    ISpiFlash& flash_;
    data::FlashLoggerRawData raw_data_{};

    FixedRingQueue<LogPayload, NORMAL_QUEUE_CAPACITY> normal_queue_{};
    FixedRingQueue<LogPayload, FAULT_QUEUE_CAPACITY> fault_queue_{};

    // Page Buffering
    std::array<data::FlashLogRecord, data::w25q128::RECORDS_PER_PAGE> page_buffer_{};
    size_t buffered_records_{0U};

    // Circular Ring Buffer Topology
    uint32_t oldest_sector_{data::w25q128::LOG_START_SECTOR};
    uint32_t head_sector_{data::w25q128::LOG_START_SECTOR};
    uint32_t head_slot_{0U};  // 0 to 63
    uint32_t next_sequence_id_{1U};
    uint8_t previous_bms_state_{static_cast<uint8_t>(types::BmsState::INIT)};
    bool next_sector_erased_{false};
    bool head_sector_clean_{false};

    // Persistent Freeze State
    bool is_frozen_{false};
    uint16_t frozen_start_sector_{0U};
    uint16_t frozen_end_sector_{0U};

    // Fault Rate Limiting
    uint32_t last_fault_mask_{0U};
    uint32_t last_fault_timestamp_ms_{0U};
    static constexpr uint32_t FAULT_RATE_LIMIT_MS = 100U;

    // Drop tracking counters for next committed record
    uint8_t pending_dropped_normal_{0U};
    uint8_t pending_dropped_fault_{0U};

    // Verification and Health recovery
    uint32_t consecutive_good_verifies_{0U};
    static constexpr uint32_t REQUIRED_GOOD_VERIFIES_FOR_RECOVERY = 3U;
    uint32_t normal_records_since_last_spot_verify_{0U};

    // Internal Helper Methods
    void advance_head(uint32_t n_slots, bool slots_are_good) noexcept;
    bool is_sector_clean(uint32_t sector_index) noexcept;
    bool reclaim_sector(uint32_t sector_index, bool& was_erased) noexcept;
    void evaluate_pre_erase_level() noexcept;
    bool program_buffered_page(bool is_fault = false) noexcept;
    bool write_boot_event_record() noexcept;
    bool scan_sector_heads(uint32_t& best_sector, uint32_t& best_seq, uint32_t& lowest_sector,
                           uint32_t& lowest_seq, uint16_t& highest_epoch,
                           bool& found_valid) noexcept;
    static uint16_t calculate_crc16(const uint8_t* data, size_t length) noexcept;
    static bool is_slot_empty(const uint8_t* slot_bytes) noexcept;
    static bool is_slot_valid(const data::FlashLogRecord& record) noexcept;
};

}  // namespace bms::firmware::drivers
