/**
 * @file test_helpers.hpp
 * @brief Common helper utilities and synthetic data generators for unit/integration tests.
 */

#pragma once

#include <cmath>
#include <cstdint>

namespace bms {
namespace test {

/**
 * @brief Approximates floating-point equality within standard tolerance.
 */
inline bool is_approx_equal(float a, float b, float epsilon = 0.001f) {
    return std::fabs(a - b) <= epsilon;
}

/**
 * @brief Helper to generate predictable monotonic timestamp increments.
 */
inline uint32_t step_timestamp_ms(uint32_t current_ms, uint32_t step_ms = 100U) {
    return current_ms + step_ms;
}

}  // namespace test
}  // namespace bms
