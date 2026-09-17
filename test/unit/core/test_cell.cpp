#include "unity.h"

#include "firmware/config/pins.hpp"
#include "firmware/config/system_config.hpp"
#include "firmware/config/thresholds.hpp"

void test_cell_voltage_threshold_bounds(void) {
    TEST_ASSERT_TRUE_MESSAGE(bms::thresholds::CELL_OVERVOLTAGE_TRIP_V >
                                 bms::thresholds::CELL_OVERVOLTAGE_WARN_V,
                             "OVP Trip must be greater than OVP Warn");

    TEST_ASSERT_TRUE_MESSAGE(bms::thresholds::CELL_UNDERVOLTAGE_WARN_V >
                                 bms::thresholds::CELL_UNDERVOLTAGE_TRIP_V,
                             "UVP Warn must be greater than UVP Trip");

    TEST_ASSERT_EQUAL_FLOAT(4.25f, bms::thresholds::CELL_OVERVOLTAGE_TRIP_V);
    TEST_ASSERT_EQUAL_FLOAT(2.80f, bms::thresholds::CELL_UNDERVOLTAGE_TRIP_V);
}

void test_pack_configuration(void) {
    TEST_ASSERT_EQUAL_UINT8(4, bms::config::TOTAL_CELL_COUNT);
    TEST_ASSERT_EQUAL_FLOAT(2.5f, bms::config::CELL_NOMINAL_CAPACITY_AH);
}

void test_safety_critical_limits(void) {
    TEST_ASSERT_EQUAL_FLOAT(2.0f, bms::thresholds::THERMAL_RUNAWAY_RISE_RATE_C_PER_S);
    TEST_ASSERT_EQUAL_FLOAT(50.0f, bms::thresholds::PACK_SHORT_CIRCUIT_CRITICAL_A);
    TEST_ASSERT_EQUAL_FLOAT(55.0f, bms::thresholds::CELL_OVERTEMP_CRITICAL_C);
}

void setUp(void) {}
void tearDown(void) {}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_cell_voltage_threshold_bounds);
    RUN_TEST(test_pack_configuration);
    RUN_TEST(test_safety_critical_limits);
    return UNITY_END();
}
