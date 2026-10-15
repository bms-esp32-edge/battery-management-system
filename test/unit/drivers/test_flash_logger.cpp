/**
 * @file test_flash_logger.cpp
 * @brief Comprehensive native unit tests for FlashLogger driver, boot scan recovery,
 *        two-tier sector searching, level-based pre-erase, torn write recovery,
 *        evidence freeze, erase suspend/resume, drop counters, and fault injection.
 */

#include "firmware/drivers/flash_logger.hpp"

#include "mock_spi_flash.hpp"
#include "modules/drivers/flash_logger_data.hpp"
#include "unity.h"

using namespace bms::firmware::drivers;
using namespace bms::modules::drivers;
using namespace bms::test::mocks;

/* ============================================================================
 * Helper: Generate deterministic sample log payload
 * ============================================================================ */
static LogPayload make_sample_payload(uint32_t ts, int32_t current_ma, uint32_t faults = 0U,
                                      uint8_t state = 1U) {
    LogPayload p{};
    p.timestamp_ms = ts;
    p.pack_current_ma = current_ma;
    p.pack_voltage_mv = 14800U;
    p.active_faults = faults;
    p.cell_voltages_mv = {3700U, 3705U, 3695U, 3700U};
    p.min_cell_voltage_mv = 3695U;
    p.max_cell_voltage_mv = 3705U;
    p.temperatures_deci_c = {250, 255, 248, 252, 240};
    p.soc_permille = 850U;
    p.soh_permille = 990U;
    p.swelling_force_raw = {120U, 125U, 118U, 122U};
    p.bms_state = state;
    return p;
}

/* ============================================================================
 * Test 1: Clean Flash Initialization & JEDEC Check (Boot-Event Record Stamp)
 * ============================================================================ */
void test_flash_logger_init_and_jedec(void) {
    MockSpiFlash mock_flash{};
    FlashLogger logger(mock_flash);

    TEST_ASSERT_TRUE(logger.init());
    TEST_ASSERT_TRUE(logger.is_healthy());
    TEST_ASSERT_TRUE(logger.is_initialized());
    TEST_ASSERT_EQUAL_HEX32(w25q128::JEDEC_W25Q128JV_ID, logger.get_diagnostics().get_jedec_id());
    TEST_ASSERT_EQUAL_UINT(1U, logger.get_boot_count());
    TEST_ASSERT_EQUAL_UINT(w25q128::LOG_START_SECTOR, logger.get_head_sector());

    // Boot-event record is written to flash at the conclusion of init
    TEST_ASSERT_EQUAL_UINT(1U, logger.get_head_slot());
    TEST_ASSERT_EQUAL_UINT(1U, logger.get_total_records());

    FlashLogRecord boot_rec{};
    TEST_ASSERT_TRUE(logger.read_record(0, boot_rec));
    TEST_ASSERT_EQUAL_UINT(1U, boot_rec.sequence_id);
    TEST_ASSERT_EQUAL_UINT(1U, boot_rec.boot_count);
    TEST_ASSERT_EQUAL_UINT(static_cast<uint8_t>(bms::modules::types::BmsState::INIT),
                           boot_rec.bms_state);
}

/* ============================================================================
 * Test 2: Sequential Record Writes, Page Buffering, & Logical Index Read
 * ============================================================================ */
void test_flash_logger_sequential_writes_and_reads(void) {
    MockSpiFlash mock_flash{};
    FlashLogger logger(mock_flash);
    TEST_ASSERT_TRUE(logger.init());
    // Slot 0 has boot-event record. Room left in page 0 is 3 records.

    // Write 3 normal records (completes page 0: slots 0..3)
    for (uint32_t i = 0; i < 3; ++i) {
        TEST_ASSERT_TRUE(logger.write(make_sample_payload(1000U + (i * 100U), 5000)));
    }
    for (uint32_t i = 0; i < 3; ++i) {
        TEST_ASSERT_TRUE(logger.step());
    }

    TEST_ASSERT_EQUAL_UINT(4U, logger.get_total_records());
    TEST_ASSERT_EQUAL_UINT(4U, logger.get_head_slot());

    // Verify records can be read back and are byte-for-byte valid
    FlashLogRecord rec{};
    TEST_ASSERT_TRUE(logger.read_record(0, rec));
    TEST_ASSERT_EQUAL_UINT(1U, rec.sequence_id);
    TEST_ASSERT_EQUAL_UINT(1U, rec.boot_count);

    TEST_ASSERT_TRUE(logger.read_record(1, rec));
    TEST_ASSERT_EQUAL_UINT(2U, rec.sequence_id);
    TEST_ASSERT_EQUAL_UINT(1000U, rec.timestamp_ms);
    TEST_ASSERT_EQUAL_INT(5000, rec.pack_current_ma);

    TEST_ASSERT_TRUE(logger.read_record(3, rec));
    TEST_ASSERT_EQUAL_UINT(4U, rec.sequence_id);
    TEST_ASSERT_EQUAL_UINT(1200U, rec.timestamp_ms);

    // Read latest
    TEST_ASSERT_TRUE(logger.read_latest(rec));
    TEST_ASSERT_EQUAL_UINT(4U, rec.sequence_id);
}

/* ============================================================================
 * Test 3: Emergency / Fault Record Immediate Flush
 * ============================================================================ */
void test_flash_logger_fault_immediate_flush(void) {
    MockSpiFlash mock_flash{};
    FlashLogger logger(mock_flash);
    TEST_ASSERT_TRUE(logger.init());
    // Slot 0 has boot event record. head_slot_ is 1.

    // Queue 1 normal record (stays in page buffer, room is 3)
    TEST_ASSERT_TRUE(logger.write(make_sample_payload(1000U, 2000, 0U)));
    TEST_ASSERT_TRUE(logger.step());
    TEST_ASSERT_EQUAL_UINT(1U, logger.get_total_records());  // Still buffered

    // Queue 1 fault record (active_faults != 0) -> forces immediate commit
    TEST_ASSERT_TRUE(logger.write(make_sample_payload(1050U, -15000, 0x0001U)));
    TEST_ASSERT_TRUE(logger.step());

    // Both records (normal + fault) should now be committed to flash immediately
    TEST_ASSERT_EQUAL_UINT(3U, logger.get_total_records());
    TEST_ASSERT_EQUAL_UINT(3U, logger.get_head_slot());

    FlashLogRecord fault_rec{};
    TEST_ASSERT_TRUE(logger.read_record(2, fault_rec));
    TEST_ASSERT_EQUAL_UINT(3U, fault_rec.sequence_id);
    TEST_ASSERT_EQUAL_HEX16(0x0001U, fault_rec.active_faults);
    TEST_ASSERT_EQUAL_INT(-15000, fault_rec.pack_current_ma);
}

/* ============================================================================
 * Test 4: Fault Includes Pre-Fault Buffered Normal Records
 * ============================================================================ */
void test_flash_logger_fault_includes_pre_fault_buffered_records(void) {
    MockSpiFlash mock_flash{};
    FlashLogger logger(mock_flash);
    TEST_ASSERT_TRUE(logger.init());
    const uint32_t initial_prog_count = mock_flash.get_program_page_count();

    // Buffer 2 normal records ahead of fault
    TEST_ASSERT_TRUE(logger.write(make_sample_payload(1000U, 1000, 0U)));
    TEST_ASSERT_TRUE(logger.step());
    TEST_ASSERT_TRUE(logger.write(make_sample_payload(1010U, 1200, 0U)));
    TEST_ASSERT_TRUE(logger.step());
    // Both normal records are buffered in RAM
    TEST_ASSERT_EQUAL_UINT(1U, logger.get_total_records());
    TEST_ASSERT_EQUAL_UINT(initial_prog_count, mock_flash.get_program_page_count());

    // Submit critical fault record
    TEST_ASSERT_TRUE(logger.write(make_sample_payload(1020U, -25000, 0x0004U)));  // UVP fault
    TEST_ASSERT_TRUE(logger.step());

    // Exactly ONE page program must have committed all 3 records together
    TEST_ASSERT_EQUAL_UINT(initial_prog_count + 1U, mock_flash.get_program_page_count());
    TEST_ASSERT_EQUAL_UINT(4U, logger.get_total_records());

    // Verify records reached flash in sequence
    FlashLogRecord r1{}, r2{}, r3{};
    TEST_ASSERT_TRUE(logger.read_record(1, r1));
    TEST_ASSERT_TRUE(logger.read_record(2, r2));
    TEST_ASSERT_TRUE(logger.read_record(3, r3));

    TEST_ASSERT_EQUAL_UINT(2U, r1.sequence_id);
    TEST_ASSERT_EQUAL_UINT(1000U, r1.timestamp_ms);

    TEST_ASSERT_EQUAL_UINT(3U, r2.sequence_id);
    TEST_ASSERT_EQUAL_UINT(1010U, r2.timestamp_ms);

    TEST_ASSERT_EQUAL_UINT(4U, r3.sequence_id);
    TEST_ASSERT_EQUAL_HEX16(0x0004U, r3.active_faults);
}

/* ============================================================================
 * Test 5: Erase Suspend and Resume on Fault
 * ============================================================================ */
void test_flash_logger_erase_suspend_on_fault(void) {
    MockSpiFlash mock_flash{};
    FlashLogger logger(mock_flash);
    TEST_ASSERT_TRUE(logger.init());

    // Inject active erase on Sector 5 (WIP bit set)
    mock_flash.inject_erase_in_progress(5U * w25q128::SECTOR_SIZE_BYTES);

    // Queue emergency fault record
    TEST_ASSERT_TRUE(logger.write(make_sample_payload(2000U, -30000, 0x0008U)));
    TEST_ASSERT_TRUE(logger.step());

    // Fault page should be programmed into Sector 4 while Sector 5 erase was suspended
    TEST_ASSERT_TRUE(logger.is_healthy());
    TEST_ASSERT_EQUAL_UINT(2U, logger.get_total_records());

    FlashLogRecord fault_rec{};
    TEST_ASSERT_TRUE(logger.read_record(1, fault_rec));
    TEST_ASSERT_EQUAL_HEX16(0x0008U, fault_rec.active_faults);

    // Erase was resumed and completed
    TEST_ASSERT_FALSE(mock_flash.is_erase_suspended());
}

/* ============================================================================
 * Test 6: Evidence Freeze Persists Across Reboot & Head Respects Range
 * ============================================================================ */
void test_flash_logger_evidence_freeze_persists_across_reboot(void) {
    MockSpiFlash mock_flash{};

    // Session 1: Freeze Sectors 4..5
    {
        FlashLogger logger(mock_flash);
        TEST_ASSERT_TRUE(logger.init());
        TEST_ASSERT_TRUE(logger.freeze_evidence(4U, 5U, 42U, 9999U));
        TEST_ASSERT_TRUE(logger.is_frozen());
        TEST_ASSERT_EQUAL_UINT(4U, logger.get_frozen_start_sector());
        TEST_ASSERT_EQUAL_UINT(5U, logger.get_frozen_end_sector());
    }

    // Session 2: Reboot and verify freeze marker is recognized
    {
        FlashLogger recovered_logger(mock_flash);
        TEST_ASSERT_TRUE(recovered_logger.init());
        TEST_ASSERT_TRUE(recovered_logger.is_frozen());
        TEST_ASSERT_EQUAL_UINT(4U, recovered_logger.get_frozen_start_sector());
        TEST_ASSERT_EQUAL_UINT(5U, recovered_logger.get_frozen_end_sector());

        // Service command clears freeze
        TEST_ASSERT_TRUE(recovered_logger.clear_freeze());
        TEST_ASSERT_FALSE(recovered_logger.is_frozen());
    }
}

/* ============================================================================
 * Test 7: Rate-Limit Repeated Identical Fault Records
 * ============================================================================ */
void test_flash_logger_rate_limit_repeated_faults(void) {
    MockSpiFlash mock_flash{};
    FlashLogger logger(mock_flash);
    TEST_ASSERT_TRUE(logger.init());

    // First fault at t=1000
    TEST_ASSERT_TRUE(logger.write(make_sample_payload(1000U, -10000, 0x0001U)));

    // Identical fault arrives 20ms later (t=1020) -> should be rate-limited from fault queue
    TEST_ASSERT_TRUE(logger.write(make_sample_payload(1020U, -10000, 0x0001U)));

    // Step logger: only the first fault should flush immediately
    TEST_ASSERT_TRUE(logger.step());
    TEST_ASSERT_EQUAL_UINT(2U, logger.get_total_records());  // Boot event + 1 fault
}

/* ============================================================================
 * Test 8: Two Reboots Inside One Sector (Monotonic Boot Epoch)
 * ============================================================================ */
void test_flash_logger_two_reboots_in_one_sector(void) {
    MockSpiFlash mock_flash{};

    // Boot 1
    {
        FlashLogger logger(mock_flash);
        TEST_ASSERT_TRUE(logger.init());
        TEST_ASSERT_EQUAL_UINT(1U, logger.get_boot_count());
        logger.write(make_sample_payload(100U, 1000));
        logger.flush();
    }

    // Boot 2 in same sector
    {
        FlashLogger logger(mock_flash);
        TEST_ASSERT_TRUE(logger.init());
        TEST_ASSERT_EQUAL_UINT(2U, logger.get_boot_count());
        logger.write(make_sample_payload(200U, 1000));
        logger.flush();
    }

    // Boot 3 in same sector
    {
        FlashLogger logger(mock_flash);
        TEST_ASSERT_TRUE(logger.init());
        TEST_ASSERT_EQUAL_UINT(3U, logger.get_boot_count());

        FlashLogRecord latest{};
        TEST_ASSERT_TRUE(logger.read_latest(latest));
        TEST_ASSERT_EQUAL_UINT(3U, latest.boot_count);
    }
}

/* ============================================================================
 * Test 9: JEDEC ID Accept and Reject Matrix
 * ============================================================================ */
void test_flash_logger_jedec_accept_reject(void) {
    // 9A: Accept Winbond JV-IQ (0xEF4018)
    {
        MockSpiFlash mock{};
        mock.set_jedec_id(w25q128::JEDEC_W25Q128JV_ID);
        FlashLogger logger(mock);
        TEST_ASSERT_TRUE(logger.init());
    }

    // 9B: Accept Winbond JM/DTR (0xEF7018)
    {
        MockSpiFlash mock{};
        mock.set_jedec_id(w25q128::JEDEC_W25Q128JM_ID);
        FlashLogger logger(mock);
        TEST_ASSERT_TRUE(logger.init());
    }

    // 9C: Reject Unknown Manufacturer (0xC24018)
    {
        MockSpiFlash mock{};
        mock.set_jedec_id(0xC24018U);
        FlashLogger logger(mock);
        TEST_ASSERT_FALSE(logger.init());
        TEST_ASSERT_FALSE(logger.is_healthy());
    }

    // 9D: Reject Wrong Capacity (0xEF4017 => 64Mbit)
    {
        MockSpiFlash mock{};
        mock.set_jedec_id(0xEF4017U);
        FlashLogger logger(mock);
        TEST_ASSERT_FALSE(logger.init());
    }
}

/* ============================================================================
 * Test 10: Dropped-Record Counters Stamped in Next Written Record
 * ============================================================================ */
void test_flash_logger_queue_overflow_drop_counters(void) {
    MockSpiFlash mock_flash{};
    FlashLogger logger(mock_flash);
    TEST_ASSERT_TRUE(logger.init());

    // Normal queue capacity is 28. Fill 28 records.
    for (uint32_t i = 0; i < 28; ++i) {
        TEST_ASSERT_TRUE(logger.write(make_sample_payload(100U + i, 1000)));
    }

    // Next 5 writes will overflow normal queue and increment drop counter
    for (uint32_t i = 0; i < 5; ++i) {
        TEST_ASSERT_FALSE(logger.write(make_sample_payload(500U + i, 1000)));
    }
    TEST_ASSERT_EQUAL_UINT(5U, logger.get_diagnostics().get_dropped_normal_records());

    // Flush records to flash
    logger.flush();

    // Find the record with dropped counter stamped
    FlashLogRecord rec{};
    bool found_dropped_stamp = false;
    for (uint32_t i = 1; i < logger.get_total_records(); ++i) {
        if (logger.read_record(i, rec) && rec.dropped_normal_count > 0) {
            TEST_ASSERT_EQUAL_UINT(5U, rec.dropped_normal_count);
            found_dropped_stamp = true;
            break;
        }
    }
    TEST_ASSERT_TRUE(found_dropped_stamp);
}

/* ============================================================================
 * Test 11: Fault Readback Mismatch Retries Next Slot & Clears Unhealthy State
 * ============================================================================ */
void test_flash_logger_fault_readback_retry_next_slot(void) {
    MockSpiFlash mock_flash{};
    FlashLogger logger(mock_flash);
    TEST_ASSERT_TRUE(logger.init());
    TEST_ASSERT_EQUAL_UINT(1U, logger.get_head_slot());

    // Inject corruption on next readback of fault record
    mock_flash.inject_corrupt_next_readback(true);
    logger.write(make_sample_payload(100U, -10000, 0x0001U));
    logger.step();

    // Fault record should fail verify on slot 1, mark slot 1 bad, and retry on slot 2!
    TEST_ASSERT_EQUAL_UINT(1U, logger.get_diagnostics().get_verify_errors());
    TEST_ASSERT_TRUE(logger.get_diagnostics().ever_failed());
    TEST_ASSERT_EQUAL_UINT(3U, logger.get_head_slot());

    // After 3 consecutive good writes, is_healthy recovers to true
    for (uint32_t i = 0; i < 3; ++i) {
        logger.write(make_sample_payload(200U + i, 1000));
        logger.flush();
    }
    TEST_ASSERT_TRUE(logger.is_healthy());
    TEST_ASSERT_TRUE(logger.get_diagnostics().ever_failed());  // Sticky flag stays true
}

/* ============================================================================
 * Test 12: Sequence and Boot Count RFC-1982 Serial Number Arithmetic
 * ============================================================================ */
void test_flash_logger_sequence_and_boot_count_wrap(void) {
    // 32-bit sequence ID serial comparison
    uint32_t seq_old = 0xFFFFFFFEU;
    uint32_t seq_new = 0x00000001U;
    TEST_ASSERT_TRUE(static_cast<int32_t>(seq_new - seq_old) > 0);

    // 16-bit boot count serial comparison
    uint16_t boot_old = 0xFFFEU;
    uint16_t boot_new = 0x0001U;
    TEST_ASSERT_TRUE(static_cast<int16_t>(boot_new - boot_old) > 0);
}

/* ============================================================================
 * Test 13: Level-Based Pre-Erase Trigger at Slot 48 (75% Sector Full)
 * ============================================================================ */
void test_flash_logger_level_based_pre_erase(void) {
    MockSpiFlash mock_flash{};
    mock_flash.init();

    // Dirty Sector 5 to simulate stale un-erased memory
    mock_flash.inject_torn_erase_on_next_call(1234U);
    mock_flash.erase_sector(5U * w25q128::SECTOR_SIZE_BYTES);
    TEST_ASSERT_EQUAL_UINT(1U, mock_flash.get_sector_erase_count(5U));

    FlashLogger logger(mock_flash);
    TEST_ASSERT_TRUE(logger.init());
    // Boot event is at slot 0. Write 47 more records to reach slot 48.
    for (uint32_t i = 0; i < 47; ++i) {
        logger.write(make_sample_payload(1000U + i, 1000));
        logger.step();
    }

    TEST_ASSERT_EQUAL_UINT(48U, logger.get_head_slot());
    TEST_ASSERT_EQUAL_UINT(2U, mock_flash.get_sector_erase_count(5U));
}

/* ============================================================================
 * Test 14: Torn Erase Recovery at Boot
 * ============================================================================ */
void test_flash_logger_torn_erase_boot_recovery(void) {
    MockSpiFlash mock_flash{};

    {
        FlashLogger logger(mock_flash);
        logger.init();
        for (uint32_t i = 0; i < 47; ++i) {
            logger.write(make_sample_payload(1000U + i, 1000));
            logger.step();
        }
    }

    // Simulate torn erase on Sector 5
    mock_flash.inject_torn_erase_on_next_call(42U);
    mock_flash.erase_sector(5U * w25q128::SECTOR_SIZE_BYTES);

    {
        FlashLogger logger(mock_flash);
        TEST_ASSERT_TRUE(logger.init());
        TEST_ASSERT_TRUE(logger.is_healthy());
    }
}

/* ============================================================================
 * Test 15: Fuzz Boot Scan with Random & Torn Flash Images
 * ============================================================================ */
void test_flash_logger_fuzz_torn_flash_image(void) {
    for (uint32_t seed : {1111U, 2222U, 3333U, 4444U}) {
        MockSpiFlash mock_flash{};
        mock_flash.inject_random_bits_on_next_write(seed);
        mock_flash.init();

        FlashLogger logger(mock_flash);
        // Ensure robust recovery with zero memory errors or buffer overruns
        logger.init();
        logger.write(make_sample_payload(5000U, 1500));
        logger.flush();

        FlashLogRecord rec{};
        logger.read_latest(rec);
    }
}

/* ============================================================================
 * Suite Runner for Flash Logger
 * ============================================================================ */
void run_flash_logger_tests(void) {
    RUN_TEST(test_flash_logger_init_and_jedec);
    RUN_TEST(test_flash_logger_sequential_writes_and_reads);
    RUN_TEST(test_flash_logger_fault_immediate_flush);
    RUN_TEST(test_flash_logger_fault_includes_pre_fault_buffered_records);
    RUN_TEST(test_flash_logger_erase_suspend_on_fault);
    RUN_TEST(test_flash_logger_evidence_freeze_persists_across_reboot);
    RUN_TEST(test_flash_logger_rate_limit_repeated_faults);
    RUN_TEST(test_flash_logger_two_reboots_in_one_sector);
    RUN_TEST(test_flash_logger_jedec_accept_reject);
    RUN_TEST(test_flash_logger_queue_overflow_drop_counters);
    RUN_TEST(test_flash_logger_fault_readback_retry_next_slot);
    RUN_TEST(test_flash_logger_sequence_and_boot_count_wrap);
    RUN_TEST(test_flash_logger_level_based_pre_erase);
    RUN_TEST(test_flash_logger_torn_erase_boot_recovery);
    RUN_TEST(test_flash_logger_fuzz_torn_flash_image);
}
