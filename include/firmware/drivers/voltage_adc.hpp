// ============================================================================
// ARCHITECTURAL NOTE:
// This header declares the Voltage ADC driver interface for the firmware layer.
// It interacts with the hardware/ESP-IDF ADC APIs and populates the pure
// data container defined in modules/drivers/voltage_adc_data.hpp.
// ============================================================================

#pragma once

#include <cstdint>

#include "esp_adc/adc_oneshot.h"
#include "modules/drivers/voltage_adc_data.hpp"

namespace bms::firmware::drivers {

class VoltageAdcDriver {
public:
    VoltageAdcDriver() noexcept = default;
    ~VoltageAdcDriver() noexcept;

    // Sole owner of a hardware handle: no copy, no move.
    // Delete copy constructors to prevent multiple hardware handle instances
    VoltageAdcDriver(const VoltageAdcDriver&) = delete;
    VoltageAdcDriver& operator=(const VoltageAdcDriver&) = delete;

    // Move constructors
    VoltageAdcDriver(VoltageAdcDriver&&) noexcept = delete;
    VoltageAdcDriver& operator=(VoltageAdcDriver&&) noexcept = delete;

    /**
     * @brief Initializes the ADC peripheral (ESP-IDF specific configuration).
     * @return true if initialization is successful, false otherwise.
     */
    // Initializes ADC1 and configures the given channel.
    [[nodiscard]] bool init(adc_channel_t channel = ADC_CHANNEL_0) noexcept;

    /**
     * @brief Reads the current voltage sample from the hardware and populates the data container.
     * @param data Reference to the VoltageAdcData pure data container to be filled.
     * @return true if read was successful, false if hardware failure occurred.
     */
    /// Reads one (averaged) sample. On failure, marks the container invalid.
    [[nodiscard]] bool read_voltage(bms::modules::drivers::VoltageAdcRawData& data) noexcept;

private:
    // Samples averaged per reading (reduces random noise by about sqrt(N)).
    // Provisional value: chosen for the 100 ms acquisition period, not yet measured.
    // TODO: measure read time (must stay well under 100 ms) and noise for
    //       N = 1, 8, 16, then keep the smallest N that meets the accuracy target.
    // Note: averaging does not correct ADC offset or nonlinearity.
    static constexpr int kSamples = 8;
    bool is_initialized_{false};
    adc_channel_t channel_{ADC_CHANNEL_0};
    adc_oneshot_unit_handle_t adc_handle_{nullptr};

    // Future expansion: ESP-IDF specific handles like adc_oneshot_unit_handle_t can be added here
};

}  // namespace bms::firmware::drivers