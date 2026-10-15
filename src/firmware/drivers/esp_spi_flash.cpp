#ifdef BMS_HARDWARE_TARGET
#include <atomic>
#include <cstring>
#include <driver/gpio.h>
#include <driver/spi_master.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "firmware/config/pins.hpp"

#include "firmware/drivers/ispi_flash.hpp"

#include "modules/drivers/flash_logger_data.hpp"

namespace bms::firmware::drivers {

namespace w25q = bms::modules::drivers::w25q128;
static const char* TAG = "ESP_SPI_FLASH";

class EspSpiFlash : public ISpiFlash {
public:
    EspSpiFlash() noexcept = default;
    ~EspSpiFlash() override {
        if (spi_dev_handle_ != nullptr) {
            spi_bus_remove_device(spi_dev_handle_);
        }
    }

    SpiFlashStatus init() noexcept override {
        spi_bus_config_t buscfg{};
        buscfg.mosi_io_num = bms::pins::SPI_MOSI_PIN;
        buscfg.miso_io_num = bms::pins::SPI_MISO_PIN;
        buscfg.sclk_io_num = bms::pins::SPI_SCK_PIN;
        buscfg.quadwp_io_num = -1;
        buscfg.quadhd_io_num = -1;
        buscfg.max_transfer_sz = 4160;

        esp_err_t ret = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
        if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
            ESP_LOGE(TAG, "Failed to initialize SPI bus: %d", ret);
            return SpiFlashStatus::BUS_ERROR;
        }

        spi_device_interface_config_t devcfg{};
        devcfg.clock_speed_hz = 40 * 1000 * 1000;  // 40 MHz
        devcfg.mode = 0;                           // Mode 0 (CPOL=0, CPHA=0)
        devcfg.spics_io_num = bms::pins::FLASH_CS_PIN;
        devcfg.queue_size = 8;

        ret = spi_bus_add_device(SPI2_HOST, &devcfg, &spi_dev_handle_);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to add SPI flash device: %d", ret);
            return SpiFlashStatus::BUS_ERROR;
        }

        return SpiFlashStatus::OK;
    }

    SpiFlashStatus read(uint32_t address, uint8_t* buffer, size_t length) noexcept override {
        if (buffer == nullptr || address + length > w25q::TOTAL_CAPACITY_BYTES) {
            return SpiFlashStatus::INVALID_PARAM;
        }
        if (spi_dev_handle_ == nullptr)
            return SpiFlashStatus::BUS_ERROR;

        // Chunk transfers into <= 2048 bytes for DMA safety
        size_t bytes_left = length;
        uint32_t curr_addr = address;
        uint8_t* curr_dest = buffer;

        while (bytes_left > 0) {
            const size_t chunk = (bytes_left > 2048U) ? 2048U : bytes_left;

            spi_transaction_ext_t tx{};
            tx.base.flags = SPI_TRANS_VARIABLE_ADDR | SPI_TRANS_VARIABLE_CMD;
            tx.base.cmd = w25q::CMD_READ_DATA;
            tx.base.addr = curr_addr;
            tx.command_bits = 8;
            tx.address_bits = 24;
            tx.base.length = chunk * 8U;
            tx.base.rxlength = chunk * 8U;
            tx.base.rx_buffer = curr_dest;

            esp_err_t err = spi_device_transmit(spi_dev_handle_, &tx.base);
            if (err != ESP_OK)
                return SpiFlashStatus::BUS_ERROR;

            bytes_left -= chunk;
            curr_addr += static_cast<uint32_t>(chunk);
            curr_dest += chunk;
        }

        return SpiFlashStatus::OK;
    }

    SpiFlashStatus program_page(uint32_t address, const uint8_t* data,
                                size_t length) noexcept override {
        if (data == nullptr || length == 0 || length > w25q::PAGE_SIZE_BYTES) {
            return SpiFlashStatus::INVALID_PARAM;
        }
        if ((address & 0xFFU) + length > w25q::PAGE_SIZE_BYTES) {
            return SpiFlashStatus::INVALID_PARAM;  // Rejects crossing page boundary
        }

        SpiFlashStatus st = write_enable();
        if (st != SpiFlashStatus::OK)
            return st;

        spi_transaction_ext_t tx{};
        tx.base.flags = SPI_TRANS_VARIABLE_ADDR | SPI_TRANS_VARIABLE_CMD;
        tx.base.cmd = w25q::CMD_PAGE_PROGRAM;
        tx.base.addr = address;
        tx.command_bits = 8;
        tx.address_bits = 24;
        tx.base.length = length * 8U;
        tx.base.tx_buffer = data;

        esp_err_t err = spi_device_transmit(spi_dev_handle_, &tx.base);
        if (err != ESP_OK)
            return SpiFlashStatus::BUS_ERROR;

        return wait_busy();
    }

    SpiFlashStatus erase_sector(uint32_t sector_address) noexcept override {
        if (sector_address % w25q::SECTOR_SIZE_BYTES != 0 ||
            sector_address >= w25q::TOTAL_CAPACITY_BYTES) {
            return SpiFlashStatus::INVALID_PARAM;
        }

        SpiFlashStatus st = write_enable();
        if (st != SpiFlashStatus::OK)
            return st;

        spi_transaction_ext_t tx{};
        tx.base.flags = SPI_TRANS_VARIABLE_ADDR | SPI_TRANS_VARIABLE_CMD;
        tx.base.cmd = w25q::CMD_SECTOR_ERASE_4K;
        tx.base.addr = sector_address;
        tx.command_bits = 8;
        tx.address_bits = 24;
        tx.base.length = 0;

        esp_err_t err = spi_device_transmit(spi_dev_handle_, &tx.base);
        if (err != ESP_OK)
            return SpiFlashStatus::BUS_ERROR;
        erase_in_progress_ = true;
        SpiFlashStatus st_wait = wait_erase_complete();
        erase_in_progress_ = false;
        return st_wait;
    }

    SpiFlashStatus read_status(uint8_t* status_out) noexcept override {
        if (status_out == nullptr || spi_dev_handle_ == nullptr) {
            return SpiFlashStatus::INVALID_PARAM;
        }

        spi_transaction_ext_t tx{};
        tx.base.flags = SPI_TRANS_VARIABLE_CMD | SPI_TRANS_USE_RXDATA;
        tx.base.cmd = w25q::CMD_READ_STATUS_1;
        tx.command_bits = 8;
        tx.base.length = 8U;
        tx.base.rxlength = 8U;

        esp_err_t err = spi_device_transmit(spi_dev_handle_, &tx.base);
        if (err != ESP_OK)
            return SpiFlashStatus::BUS_ERROR;

        *status_out = tx.base.rx_data[0];
        return SpiFlashStatus::OK;
    }

    SpiFlashStatus read_jedec_id(uint32_t* id_out) noexcept override {
        if (id_out == nullptr || spi_dev_handle_ == nullptr) {
            return SpiFlashStatus::INVALID_PARAM;
        }

        spi_transaction_ext_t tx{};
        tx.base.flags = SPI_TRANS_VARIABLE_CMD | SPI_TRANS_USE_RXDATA;
        tx.base.cmd = w25q::CMD_READ_JEDEC_ID;
        tx.command_bits = 8;
        tx.base.length = 24U;
        tx.base.rxlength = 24U;

        esp_err_t err = spi_device_transmit(spi_dev_handle_, &tx.base);
        if (err != ESP_OK)
            return SpiFlashStatus::BUS_ERROR;

        const uint32_t id = (static_cast<uint32_t>(tx.base.rx_data[0]) << 16U) |
                            (static_cast<uint32_t>(tx.base.rx_data[1]) << 8U) |
                            static_cast<uint32_t>(tx.base.rx_data[2]);
        *id_out = id;

        // Verify Winbond manufacturer (0xEF), 128Mbit capacity (0x18), and memory type (0x40 or
        // 0x70)
        const uint8_t mfg = static_cast<uint8_t>(tx.base.rx_data[0]);
        const uint8_t mem_type = static_cast<uint8_t>(tx.base.rx_data[1]);
        const uint8_t cap = static_cast<uint8_t>(tx.base.rx_data[2]);

        if (mfg != 0xEFU || cap != 0x18U || (mem_type != 0x40U && mem_type != 0x70U)) {
            return SpiFlashStatus::BUS_ERROR;
        }
        return SpiFlashStatus::OK;
    }

    SpiFlashStatus suspend_erase() noexcept override {
        if (spi_dev_handle_ == nullptr)
            return SpiFlashStatus::BUS_ERROR;

        if (!erase_in_progress_) {
            // Can only suspend an active erase. If WIP is 1 due to a page program,
            // we must not suspend it. Wait for the program to finish instead.
            return wait_busy(5U);
        }

        spi_transaction_ext_t tx{};
        tx.base.flags = SPI_TRANS_VARIABLE_CMD;
        tx.command_bits = 8;
        tx.base.cmd = w25q::CMD_ERASE_SUSPEND;
        esp_err_t err = spi_device_transmit(spi_dev_handle_, &tx.base);
        if (err != ESP_OK)
            return SpiFlashStatus::BUS_ERROR;

        erase_suspended_ = true;
        erase_in_progress_ = false;
        // W25Q128JV requires up to 20 microseconds (tSUS) to suspend
        return wait_busy(50U);
    }

    SpiFlashStatus resume_erase() noexcept override {
        if (spi_dev_handle_ == nullptr)
            return SpiFlashStatus::BUS_ERROR;

        if (!erase_suspended_)
            return SpiFlashStatus::OK;  // Nothing to resume

        spi_transaction_ext_t tx{};
        tx.base.flags = SPI_TRANS_VARIABLE_CMD;
        tx.command_bits = 8;
        tx.base.cmd = w25q::CMD_ERASE_RESUME;
        esp_err_t err = spi_device_transmit(spi_dev_handle_, &tx.base);
        if (err != ESP_OK)
            return SpiFlashStatus::BUS_ERROR;

        erase_suspended_ = false;
        erase_in_progress_ = true;
        return SpiFlashStatus::OK;
    }

    [[nodiscard]] bool is_erase_suspended() const noexcept override { return erase_suspended_; }

private:
    spi_device_handle_t spi_dev_handle_{nullptr};
    std::atomic<bool> erase_suspended_{false};
    std::atomic<bool> erase_in_progress_{false};

    SpiFlashStatus write_enable() noexcept {
        spi_transaction_ext_t tx{};
        tx.base.flags = SPI_TRANS_VARIABLE_CMD;
        tx.command_bits = 8;
        tx.base.cmd = w25q::CMD_WRITE_ENABLE;
        esp_err_t err = spi_device_transmit(spi_dev_handle_, &tx.base);
        return (err == ESP_OK) ? SpiFlashStatus::OK : SpiFlashStatus::BUS_ERROR;
    }

    SpiFlashStatus wait_busy(uint32_t timeout_ms = 1000U) noexcept {
        uint8_t status = 0;
        uint32_t elapsed = 0;
        while (elapsed < timeout_ms) {
            if (read_status(&status) != SpiFlashStatus::OK) {
                return SpiFlashStatus::BUS_ERROR;
            }
            if ((status & w25q::STATUS_WIP) == 0U) {
                return SpiFlashStatus::OK;
            }
            vTaskDelay(pdMS_TO_TICKS(5));
            elapsed += 5;
        }
        return SpiFlashStatus::TIMEOUT;
    }

    SpiFlashStatus wait_erase_complete() noexcept {
        uint8_t status = 0;
        uint32_t elapsed = 0;
        while (elapsed < 1000U) {
            if (read_status(&status) != SpiFlashStatus::OK) {
                return SpiFlashStatus::BUS_ERROR;
            }
            if ((status & w25q::STATUS_WIP) == 0U) {
                // If the WIP bit is 0, we must verify we are not currently suspended.
                // A suspended erase yields WIP == 0, but is NOT complete.
                if (!erase_suspended_) {
                    return SpiFlashStatus::OK;
                }
            }
            vTaskDelay(pdMS_TO_TICKS(5));
            elapsed += 5;
        }
        return SpiFlashStatus::TIMEOUT;
    }
};

}  // namespace bms::firmware::drivers

#endif
