/**
 * @file validation.cpp
 * @brief Validate the parameters provided
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#include "simple_platformer/math/validation.hpp"

#include <cmath>
#include <stdexcept>
#include <string>

#include <glm/vec2.hpp>

namespace simple_platformer
{
    // Verifies that both the X and Y coordinates are real numbers (not NaN or infinity)
    bool isFinite(glm::vec2 value)
    {
        return std::isfinite(value.x) && std::isfinite(value.y);
    }

    // Checks that a float value is valid and strictly greater than zero
    bool isFinitePositive(float value)
    {
        return std::isfinite(value) && value > 0.0F;
    }

    // Assures both components of a 2D vector are valid and strictly greater than zero
    bool isFinitePositive(glm::vec2 value)
    {
        return isFinitePositive(value.x) && isFinitePositive(value.y);
    }

    // Checks that a float value is valid and equal to or greater than zero
    bool isFiniteNonNegative(float value)
    {
        return std::isfinite(value) && value >= 0.0F;
    }

    // Assures both components of a 2D vector are valid and equal to or greater than zero
    bool isFiniteNonNegative(glm::vec2 value)
    {
        return isFiniteNonNegative(value.x) && isFiniteNonNegative(value.y);
    }

    // Asserts a 2D vector contains valid numeric data, throwing if NaN or infinity is detected
    void requireFinite(glm::vec2 value, const char* what)
    {
        if (!isFinite(value))
        {
            throw std::invalid_argument(std::string(what) + " must be finite");
        }
    }

    // Validates that an incoming time duration is a valid physical slice, allowing zero
    void requireSeconds(float seconds, const char* what)
    {
        if (!isFiniteNonNegative(seconds))
        {
            throw std::invalid_argument(
                std::string(what) + " must be a finite, non-negative number of seconds");
        }
    }

    // Validates that a physics simulation time step is positive and strictly non-zero
    void requirePositiveSeconds(float seconds, const char* what)
    {
        if (!isFinitePositive(seconds))
        {
            throw std::invalid_argument(std::string(what) + " must be finite and positive");
        }
    }
}
