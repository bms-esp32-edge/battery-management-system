#pragma once

#include <cstddef>
#include <cstdint>

namespace bms::firmware::drivers {

/**
 * @brief Status return codes for low-level SPI NOR Flash operations.
 */
enum class SpiFlashStatus : uint8_t {
    OK = 0,
    BUSY,
    TIMEOUT,
    WRITE_PROTECT,
    BUS_ERROR,
    INVALID_PARAM
};

/**
 * @brief Transport interface for Winbond W25Q-series SPI NOR Flash memory.
 */
class ISpiFlash {
public:
    virtual ~ISpiFlash() = default;

    virtual SpiFlashStatus init() noexcept = 0;
    virtual SpiFlashStatus read(uint32_t address, uint8_t* buffer, size_t length) noexcept = 0;
    virtual SpiFlashStatus program_page(uint32_t address, const uint8_t* data,
                                        size_t length) noexcept = 0;
    virtual SpiFlashStatus erase_sector(uint32_t sector_address) noexcept = 0;
    virtual SpiFlashStatus suspend_erase() noexcept = 0;
    virtual SpiFlashStatus resume_erase() noexcept = 0;
    virtual SpiFlashStatus read_status(uint8_t* status_out) noexcept = 0;
    virtual SpiFlashStatus read_jedec_id(uint32_t* id_out) noexcept = 0;
    [[nodiscard]] virtual bool is_erase_suspended() const noexcept = 0;
};

}  // namespace bms::firmware::drivers
