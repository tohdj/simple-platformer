/**
 * @file use_item.hpp
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

    /**
     * @brief Attempts to activate or consume an item from an actor's inventory slot.
     *
     * This function checks if the target actor exists, verifies the inventory slot index bounds,
     * and executes the corresponding item logic (e.g., applying status effects, firing a weapon,
     * or consuming a consumable).
     *
     * @param world The active game world context containing the actor and items data.
     * @param actor The unique identifier of the entity attempting to use the item.
     * @param slot The zero-based array index of the inventory container slot to trigger.
     * @return true if the item was successfully used or consumed; false if the slot was empty,
     *         the action was on cooldown, or the actor was invalid.
     */
    bool useItem(World& world, ActorId actor, std::size_t slot);
}
