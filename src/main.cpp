#include "firmware/config/pins.hpp"
#include "firmware/config/system_config.hpp"
#include "firmware/config/thresholds.hpp"

#ifdef BMS_HARDWARE_TARGET
#include <Arduino.h>

void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 2000) {
        // Wait for serial monitor connection
    }

    Serial.println("=================================================");
    Serial.println("   ESP32-S3 Edge Smart Battery Management System ");
    Serial.println("   Hardware Target Initialized                   ");
    Serial.println("=================================================");
}

void loop() {
    Serial.printf("[BMS Heartbeat] System Operational. Total Cells: %d\n",
                  bms::config::TOTAL_CELL_COUNT);
    delay(1000);
}

#else
#include <iostream>

int main() {
    std::cout << "=================================================" << std::endl;
    std::cout << "   ESP32-S3 Smart BMS Native Simulation Mode     " << std::endl;
    std::cout << "   Host Environment: Linux / Windows Native      " << std::endl;
    std::cout << "=================================================" << std::endl;
    std::cout << "[INFO] Pack Configured for " << static_cast<int>(bms::config::TOTAL_CELL_COUNT)
              << " Cells (4S Configuration)." << std::endl;
    std::cout << "[INFO] OVP Trip Threshold: " << bms::thresholds::CELL_OVERVOLTAGE_TRIP_V << " V"
              << std::endl;
    std::cout << "[INFO] UVP Trip Threshold: " << bms::thresholds::CELL_UNDERVOLTAGE_TRIP_V << " V"
              << std::endl;
    std::cout << "[INFO] Thermal Runaway dT/dt Limit: "
              << bms::thresholds::THERMAL_RUNAWAY_RISE_RATE_C_PER_S << " C/s" << std::endl;

    return 0;
}

#endif
