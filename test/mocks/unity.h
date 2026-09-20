#pragma once

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

// Forward declare Unity lifecycle hooks
void setUp(void);
void tearDown(void);

#ifndef UNITY_BEGIN
#define UNITY_BEGIN()                                     \
    do {                                                  \
        printf("--- Unity Test Execution Started ---\n"); \
    } while (0)
#endif

#ifndef UNITY_END
#define UNITY_END() (0)
#endif

#ifndef RUN_TEST
#define RUN_TEST(func)                           \
    do {                                         \
        printf("[RUNNING TEST] %s ... ", #func); \
        setUp();                                 \
        func();                                  \
        tearDown();                              \
        printf("PASSED\n");                      \
    } while (0)
#endif

#ifndef TEST_ASSERT_TRUE
#define TEST_ASSERT_TRUE(condition)                                                               \
    do {                                                                                          \
        if (!(condition)) {                                                                       \
            fprintf(stderr, "\nAssertion Failed: (%s) is false at %s:%d\n", #condition, __FILE__, \
                    __LINE__);                                                                    \
            exit(1);                                                                              \
        }                                                                                         \
    } while (0)
#endif

#ifndef TEST_ASSERT_TRUE_MESSAGE
#define TEST_ASSERT_TRUE_MESSAGE(condition, message)                                           \
    do {                                                                                       \
        if (!(condition)) {                                                                    \
            fprintf(stderr, "\nAssertion Failed: %s at %s:%d\n", message, __FILE__, __LINE__); \
            exit(1);                                                                           \
        }                                                                                      \
    } while (0)
#endif

#ifndef TEST_ASSERT_FALSE
#define TEST_ASSERT_FALSE(condition) TEST_ASSERT_TRUE(!(condition))
#endif

#ifndef TEST_ASSERT_EQUAL_FLOAT
#define TEST_ASSERT_EQUAL_FLOAT(expected, actual)                                          \
    do {                                                                                   \
        float diff = std::fabs(static_cast<float>(expected) - static_cast<float>(actual)); \
        if (diff > 0.0001f) {                                                              \
            fprintf(stderr, "\nAssertion Failed: Expected %f, got %f at %s:%d\n",          \
                    static_cast<double>(expected), static_cast<double>(actual), __FILE__,  \
                    __LINE__);                                                             \
            exit(1);                                                                       \
        }                                                                                  \
    } while (0)
#endif

#ifndef TEST_ASSERT_EQUAL_UINT8
#define TEST_ASSERT_EQUAL_UINT8(expected, actual)                                           \
    do {                                                                                    \
        if (static_cast<uint8_t>(expected) != static_cast<uint8_t>(actual)) {               \
            fprintf(stderr, "\nAssertion Failed: Expected %u, got %u at %s:%d\n",           \
                    static_cast<unsigned int>(expected), static_cast<unsigned int>(actual), \
                    __FILE__, __LINE__);                                                    \
            exit(1);                                                                        \
        }                                                                                   \
    } while (0)
#endif

#ifndef TEST_ASSERT_EQUAL_INT
#define TEST_ASSERT_EQUAL_INT(expected, actual)                                                \
    do {                                                                                       \
        if (static_cast<int>(expected) != static_cast<int>(actual)) {                          \
            fprintf(stderr, "\nAssertion Failed: Expected %d, got %d at %s:%d\n",              \
                    static_cast<int>(expected), static_cast<int>(actual), __FILE__, __LINE__); \
            exit(1);                                                                           \
        }                                                                                      \
    } while (0)
#endif
