#include <cstdint>

#include "firmware/config/pins.hpp"
#include "firmware/config/system_config.hpp"
#include "firmware/config/thresholds.hpp"

#ifdef BMS_HARDWARE_TARGET
#include <esp_log.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "BMS_MAIN";

extern "C" void app_main() {
    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "   ESP32-S3 Edge Smart Battery Management System ");
    ESP_LOGI(TAG, "   ESP-IDF Framework Initialized                 ");
    ESP_LOGI(TAG, "=================================================");

    ESP_LOGI(TAG, "[INFO] Pack Configured for %d Cells",
             static_cast<int>(bms::config::TOTAL_CELL_COUNT));
    ESP_LOGI(TAG, "[INFO] OVP Trip Threshold: %.2f V", bms::thresholds::CELL_OVERVOLTAGE_TRIP_V);
    ESP_LOGI(TAG, "[INFO] UVP Trip Threshold: %.2f V", bms::thresholds::CELL_UNDERVOLTAGE_TRIP_V);

    while (true) {
        ESP_LOGI(TAG, "[BMS Heartbeat] System Operational.");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

#else
#include <iostream>

int main() {
    std::cout << "=================================================" << std::endl;
    std::cout << "   ESP32-S3 Smart BMS Native Simulation Mode     " << std::endl;
    std::cout << "   Host Environment: Linux / Windows Native      " << std::endl;
    std::cout << "=================================================" << std::endl;
    std::cout << "[INFO] Pack Configured for " << static_cast(bms::config::TOTAL_CELL_COUNT)
              << " Cells (4S Configuration)." << std::endl;
    std::cout << "[INFO] OVP Trip Threshold: " << bms::thresholds::CELL_OVERVOLTAGE_TRIP_V << " V"
              << std::endl;
    std::cout << "[INFO] UVP Trip Threshold: " << bms::thresholds::CELL_UNDERVOLTAGE_TRIP_V << " V"
              << std::endl;
    std::cout << "[INFO] Thermal Runaway dT/dt Limit: "
              << bms::thresholds::THERMAL_RUNAWAY_RISE_RATE_C_PER_S << " C/s" << std::endl;
    std::cout << "[INFO] Hardware Interfaces: I2C (SDA=" << static_cast(bms::pins::I2C_SDA_PIN)
              << ", SCL=" << static_cast(bms::pins::I2C_SCL_PIN) << ")" << std::endl;

    return 0;
}
#endif