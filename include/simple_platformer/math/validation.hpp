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
    /** @brief Checks if both components of a 2D vector are finite numbers. */
    bool isFinite(glm::vec2 value);

    /** @brief Checks if a scalar value is finite and strictly greater than zero. */
    bool isFinitePositive(float value);

    /**
     * @brief Checks if both components of a 2D vector are finite and strictly greater than zero.
     * @note Commonly used to validate dimensions and structural sizes.
     */
    bool isFinitePositive(glm::vec2 value);

    /** @brief Checks if a scalar value is finite and greater than or equal to zero. */
    bool isFiniteNonNegative(float value);

    /**
     * @brief Checks if both components of a 2D vector are finite and greater than or equal to zero.
     * @note Commonly used to validate texture atlas positions or coordinates.
     */
    bool isFiniteNonNegative(glm::vec2 value);

    /**
     * @brief Validates that both vector components are finite, throwing an exception if they are
     * not.
     *
     * @param value The 2D vector to validate.
     * @param what A description prefix used to construct the exception message (e.g., "Input
     * intentions").
     * @throws std::invalid_argument or similar standard exception if validation fails.
     */
    void requireFinite(glm::vec2 value, const char* what);

    /**
     * @brief Validates that a duration is finite and non-negative, allowing zero.
     *
     * @param seconds The time duration to validate.
     * @param what A description prefix used to construct the exception message (e.g., "Frame
     * time").
     * @throws std::invalid_argument or similar standard exception if validation fails.
     */
    void requireSeconds(float seconds, const char* what);

    /**
     * @brief Validates that a duration is finite and strictly positive, forbidding zero.
     *
     * @param seconds The time step duration to validate.
     * @param what A description prefix used to construct the exception message (e.g., "Attacks time
     * step").
     * @throws std::invalid_argument or similar standard exception if validation fails.
     */
    void requirePositiveSeconds(float seconds, const char* what);
}
