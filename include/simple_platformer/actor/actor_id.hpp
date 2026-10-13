/**
 * @file        actor_id.hpp
 * @brief       Declares the ActorId safety wrapper and its associated validation and comparison
 * operators.
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright   Copyright (c) 2026 Simple Platformer. All rights reserved.
 */

#pragma once

#include <cstdint>

namespace simple_platformer
{
    /**
     * @brief A unique identifier for actors within the game world.
     *
     * Wraps a 32-bit unsigned integer to provide type safety, preventing
     * accidental mixing of actor IDs with other numeric identifiers.
     */
    struct ActorId
    {
        /// The underlying numeric value of the ID. Defaults to 0 (invalid).
        std::uint32_t value = 0;
    };

    /**
     * @brief Compares two ActorIds for equality.
     * @param left The first ActorId to compare.
     * @param right The second ActorId to compare.
     * @return true if both IDs have the same value, false otherwise.
     */
    bool operator==(ActorId left, ActorId right);

    /**
     * @brief Compares two ActorIds for inequality.
     * @param left The first ActorId to compare.
     * @param right The second ActorId to compare.
     * @return true if the IDs have different values, false otherwise.
     */
    bool operator!=(ActorId left, ActorId right);

    /**
     * @brief Checks if an ActorId represents a valid actor.
     * @param id The ActorId to evaluate.
     * @return true if the ID is valid (typically non-zero), false otherwise.
     */
    bool isValid(ActorId id);
}
