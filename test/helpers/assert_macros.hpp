#pragma once

#ifdef UNIT_TESTING
#include <unity.h>
#endif

// Custom BMS Test Assertions
#define TEST_ASSERT_VOLTAGE_IN_RANGE(v, min_v, max_v) \
    TEST_ASSERT_TRUE_MESSAGE((v) >= (min_v) && (v) <= (max_v), "Voltage out of safe range")

#define TEST_ASSERT_TEMP_SAFE(temp_c, max_temp_c) \
    TEST_ASSERT_TRUE_MESSAGE((temp_c) <= (max_temp_c), "Temperature exceeded safety limit")
