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
    // A unique identifier for actors within the game world.
    // Wraps a 32-bit unsigned integer to provide type safety, preventing accidental mixing with
    // other numeric variables.
    struct ActorId
    {
        std::uint32_t value = 0; // The underlying numeric value of the ID. Defaults to 0 (invalid).
    };

    // Compares two ActorIds for exact value equality.
    bool operator==(ActorId left, ActorId right);

    // Compares two ActorIds for value inequality.
    bool operator!=(ActorId left, ActorId right);

    // Checks if an ActorId represents a valid actor handle (enforces the non-zero convention).
    bool isValid(ActorId id);
}
