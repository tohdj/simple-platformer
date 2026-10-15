/**
 * @file        actor_id.cpp
 * @brief       Declares the ActorId safety wrapper and its associated validation and comparison
 * operators.
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright   Copyright (c) 2026 Simple Platformer. All rights reserved.
 */

#include "simple_platformer/actor/actor_id.hpp"

namespace simple_platformer
{
    // Evaluates whether two actor tracking handles share the exact same underlying registration
    // value
    bool operator==(ActorId left, ActorId right)
    {
        return left.value == right.value;
    }

    // Evaluates inequality between two actor handles by negating the equality operator result
    bool operator!=(ActorId left, ActorId right)
    {
        return !(left == right);
    }

    // Checks the validity of an actor ID handle, enforcing the engine convention that 0 acts as a
    // null/invalid record
    bool isValid(ActorId id)
    {
        return id.value != 0;
    }
}
