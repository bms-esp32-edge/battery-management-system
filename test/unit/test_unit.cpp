#include "unity.h"

extern void run_types_tests(void);
extern void run_cell_tests(void);

void setUp(void) {}
void tearDown(void) {}

int main(void) {
    UNITY_BEGIN();

    // Execute unit test suites
    run_types_tests();
    run_cell_tests();

    return UNITY_END();
}
