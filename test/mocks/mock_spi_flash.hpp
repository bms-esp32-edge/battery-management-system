#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <random>
#include <vector>

#include "firmware/drivers/ispi_flash.hpp"
#include "modules/drivers/flash_logger_data.hpp"

namespace bms::test::mocks {

namespace w25q = bms::modules::drivers::w25q128;
using bms::firmware::drivers::ISpiFlash;
using bms::firmware::drivers::SpiFlashStatus;

class MockSpiFlash : public ISpiFlash {
public:
    MockSpiFlash() {
        memory_.resize(w25q::TOTAL_CAPACITY_BYTES, 0xFFU);
        erase_counts_.resize(w25q::TOTAL_SECTOR_COUNT, 0U);
    }

    ~MockSpiFlash() override = default;

    SpiFlashStatus init() noexcept override {
        is_initialized_ = true;
        return SpiFlashStatus::OK;
    }

    SpiFlashStatus read(uint32_t address, uint8_t* buffer, size_t length) noexcept override {
        if (!is_initialized_ || buffer == nullptr ||
            address + length > w25q::TOTAL_CAPACITY_BYTES) {
            return SpiFlashStatus::INVALID_PARAM;
        }

        std::memcpy(buffer, &memory_[address], length);
        if (inject_corrupt_readback_ && length > 0) {
            buffer[0] ^= 0xFFU;  // Invert first byte to simulate readback corruption
            inject_corrupt_readback_ = false;
        }
        return SpiFlashStatus::OK;
    }

    SpiFlashStatus program_page(uint32_t address, const uint8_t* data,
                                size_t length) noexcept override {
        if (!is_initialized_ || data == nullptr || length == 0 ||
            length > w25q::PAGE_SIZE_BYTES) {
            return SpiFlashStatus::INVALID_PARAM;
        }
        if (inject_program_failure_) {
            inject_program_failure_ = false;
            return SpiFlashStatus::BUS_ERROR;
        }
        // Reject writes spanning across page boundary
        if ((address & 0xFFU) + length > w25q::PAGE_SIZE_BYTES) {
            return SpiFlashStatus::INVALID_PARAM;
        }

        size_t bytes_to_program = length;
        if (inject_cutoff_bytes_ != 0xFFFFFFFFU) {
            bytes_to_program = std::min(length, static_cast<size_t>(inject_cutoff_bytes_));
            inject_cutoff_bytes_ = 0xFFFFFFFFU;  // One-shot
        }

        if (inject_random_bits_seed_ != 0U) {
            std::mt19937 rng(inject_random_bits_seed_);
            std::uniform_int_distribution<uint32_t> dist(0, 255);
            for (size_t i = 0; i < bytes_to_program; ++i) {
                const uint8_t rand_mask = static_cast<uint8_t>(dist(rng));
                const uint8_t simulated_new = data[i] | rand_mask;
                memory_[address + i] &= simulated_new;  // NOR flash bit-clearing rule
            }
            inject_random_bits_seed_ = 0U;  // One-shot
        } else {
            // Standard NOR flash bitwise AND programming
            for (size_t i = 0; i < bytes_to_program; ++i) {
                memory_[address + i] &= data[i];
            }
        }

        program_page_count_++;
        return SpiFlashStatus::OK;
    }

    SpiFlashStatus erase_sector(uint32_t sector_address) noexcept override {
        if (!is_initialized_ || sector_address % w25q::SECTOR_SIZE_BYTES != 0 ||
            sector_address >= w25q::TOTAL_CAPACITY_BYTES) {
            return SpiFlashStatus::INVALID_PARAM;
        }

        const uint32_t sector_idx = sector_address / w25q::SECTOR_SIZE_BYTES;
        erase_counts_[sector_idx]++;

        if (inject_torn_erase_seed_ != 0U) {
            // Torn erase: partially erase bytes randomly
            std::mt19937 rng(inject_torn_erase_seed_);
            std::uniform_int_distribution<uint32_t> dist(0, 255);
            for (size_t i = 0; i < w25q::SECTOR_SIZE_BYTES; ++i) {
                memory_[sector_address + i] = static_cast<uint8_t>(dist(rng));
            }
            inject_torn_erase_seed_ = 0U;
        } else {
            std::fill_n(&memory_[sector_address], w25q::SECTOR_SIZE_BYTES, 0xFFU);
        }

        return SpiFlashStatus::OK;
    }

    SpiFlashStatus read_status(uint8_t* status_out) noexcept override {
        if (!is_initialized_ || status_out == nullptr) return SpiFlashStatus::INVALID_PARAM;
        *status_out = status_reg_;
        return SpiFlashStatus::OK;
    }

    SpiFlashStatus read_jedec_id(uint32_t* id_out) noexcept override {
        if (!is_initialized_ || id_out == nullptr) return SpiFlashStatus::INVALID_PARAM;
        *id_out = jedec_id_;
        return SpiFlashStatus::OK;
    }

    SpiFlashStatus suspend_erase() noexcept override {
        if (!is_initialized_) return SpiFlashStatus::INVALID_PARAM;
        if (erase_in_progress_) {
            erase_suspended_ = true;
            erase_in_progress_ = false;
            status_reg_ |= w25q::STATUS_SUS;
            status_reg_ &= ~w25q::STATUS_WIP;
            return SpiFlashStatus::OK;
        }
        return SpiFlashStatus::OK;
    }

    SpiFlashStatus resume_erase() noexcept override {
        if (!is_initialized_) return SpiFlashStatus::INVALID_PARAM;
        if (erase_suspended_) {
            erase_suspended_ = false;
            status_reg_ &= ~w25q::STATUS_SUS;
            // Complete simulated erase of the suspended sector
            if (suspended_sector_address_ < w25q::TOTAL_CAPACITY_BYTES) {
                std::fill_n(&memory_[suspended_sector_address_], w25q::SECTOR_SIZE_BYTES, 0xFFU);
            }
            return SpiFlashStatus::OK;
        }
        return SpiFlashStatus::OK;
    }

    [[nodiscard]] bool is_erase_suspended() const noexcept override {
        return erase_suspended_;
    }

    // Fault Injection and Test Introspection APIs
    void inject_power_loss_after_bytes(uint32_t bytes) noexcept {
        inject_cutoff_bytes_ = bytes;
    }

    void inject_random_bits_on_next_write(uint32_t seed) noexcept {
        inject_random_bits_seed_ = seed;
    }

    void inject_program_failure_on_next_call(bool fail) noexcept {
        inject_program_failure_ = fail;
    }

    void inject_corrupt_next_readback(bool corrupt) noexcept {
        inject_corrupt_readback_ = corrupt;
    }

    void inject_torn_erase_on_next_call(uint32_t seed) noexcept {
        inject_torn_erase_seed_ = seed;
    }

    void inject_erase_in_progress(uint32_t sector_addr) noexcept {
        erase_in_progress_ = true;
        erase_suspended_ = false;
        suspended_sector_address_ = sector_addr;
        status_reg_ |= w25q::STATUS_WIP;
    }

    void set_jedec_id(uint32_t id) noexcept { jedec_id_ = id; }
    void set_status_reg(uint8_t status) noexcept { status_reg_ = status; }

    [[nodiscard]] uint32_t get_sector_erase_count(uint32_t sector_idx) const noexcept {
        return (sector_idx < erase_counts_.size()) ? erase_counts_[sector_idx] : 0U;
    }

    [[nodiscard]] uint32_t get_program_page_count() const noexcept {
        return program_page_count_;
    }

    [[nodiscard]] const uint8_t* raw_data() const noexcept { return memory_.data(); }

private:
    std::vector<uint8_t> memory_{};
    std::vector<uint32_t> erase_counts_{};
    uint32_t jedec_id_{w25q::JEDEC_W25Q128JV_ID};
    uint8_t status_reg_{0U};
    bool is_initialized_{false};
    bool erase_in_progress_{false};
    bool erase_suspended_{false};
    uint32_t suspended_sector_address_{0U};
    uint32_t program_page_count_{0U};

    uint32_t inject_cutoff_bytes_{0xFFFFFFFFU};
    uint32_t inject_random_bits_seed_{0U};
    uint32_t inject_torn_erase_seed_{0U};
    bool inject_program_failure_{false};
    bool inject_corrupt_readback_{false};
};

}  // namespace bms::test::mocks
