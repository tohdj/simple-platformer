/**
 * @file item_use.hpp
 * @brief Logic and invocation handlers for triggering item usage from an actor's inventory slot.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#pragma once

#include <cstddef>

#include "simple_platformer/actor/actor_id.hpp"

namespace simple_platformer
{
    class World;

    // Attempts to activate or consume an item from an actor's inventory slot.
    // Checks if the target actor exists, verifies slot index bounds, and executes the corresponding
    // item logic. Returns true if successfully used; false if the slot was empty, action was on
    // cooldown, or actor was invalid.
    bool useItem(World& world, ActorId actor, std::size_t slot);
}
