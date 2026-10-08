#include "unity.h"

void setUp(void);
void tearDown(void);

#include "../src/firmware/drivers/flash_logger.cpp"
#include "unit/core/test_cell.cpp"
#include "unit/core/test_types.cpp"
#include "unit/drivers/test_flash_logger.cpp"

void setUp(void) {}
void tearDown(void) {}

int main(void) {
    UNITY_BEGIN();

    // Execute unit test suites
    run_types_tests();
    run_cell_tests();
    run_flash_logger_tests();

    return UNITY_END();
}
