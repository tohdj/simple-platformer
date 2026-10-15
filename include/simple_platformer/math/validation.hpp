/**
 * @file validation.hpp
 * @brief Validate the parameters provided
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#pragma once

#include <glm/vec2.hpp>

namespace simple_platformer
{
    // Checks if both components of a 2D vector are finite numbers
    bool isFinite(glm::vec2 value);

    // Checks if a scalar value is finite and strictly greater than zero
    bool isFinitePositive(float value);

    // Checks if both components of a 2D vector are finite and strictly greater than zero (e.g. for
    // object sizes)
    bool isFinitePositive(glm::vec2 value);

    // Checks if a scalar value is finite and greater than or equal to zero
    bool isFiniteNonNegative(float value);

    // Checks if both components of a 2D vector are finite and greater than or equal to zero (e.g.
    // for texture bounds)
    bool isFiniteNonNegative(glm::vec2 value);

    // Assures both vector components are finite numbers, throwing an invalid_argument exception on
    // failure
    void requireFinite(glm::vec2 value, const char* what);

    // Assures a time duration value is a finite and non-negative number, allowing zero
    void requireSeconds(float seconds, const char* what);

    // Assures a time step value is a finite and strictly positive number, forbidding zero
    void requirePositiveSeconds(float seconds, const char* what);
}
