#include "unity.h"

void setUp(void);
void tearDown(void);

#include "unit/core/test_cell.cpp"
#include "unit/core/test_types.cpp"

void setUp(void) {}
void tearDown(void) {}

int main(void) {
    UNITY_BEGIN();

    // Execute unit test suites
    run_types_tests();
    run_cell_tests();

    return UNITY_END();
}
