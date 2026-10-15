/**
 * @file item.hpp
 * @brief Blueprint definitions, usage effect categories, and inventory allocation stacks for items.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#pragma once

#include <string>

#include "simple_platformer/render/sprite.hpp"

namespace simple_platformer
{
    // Unique identifier token representing a specific type of item in the master database.
    using ItemId = int;

    // Runtime behavioral hooks triggered when an item is consumed or activated.
    enum class ItemEffect
    {
        None, // No immediate script evaluation (e.g., pure crafting materials or quest tokens).
        Heal  // Restores an actor's current health pool points.
    };

    // The core immutable data structure defining an item's properties in the game database.
    struct ItemDefinition
    {
        ItemId id = 0;    // Unique database key identifying this specific item archetype.
        std::string name; // User-facing display name of the item.
        Sprite
            icon; // Visual sprite element used to draw the item inside inventory menus or UI slots.
        int maximumStack = 1; // Maximum capacity limit for how many instances of this item can
                              // occupy a single inventory slot.
        ItemEffect effect =
            ItemEffect::None; // The behavior type applied to the actor when this item is used.
        int effectAmount = 0; // The numeric modifier scale associated with the applied effect
                              // (e.g., amount of HP restored).
    };

    // A dynamic inventory container allocation representing an entry of items.
    struct ItemStack
    {
        ItemId item =
            0; // The database identifier of the item type stored in this allocation block.
        int quantity = 1; // The current count of items combined inside this specific slot.
    };

    // Runs debug asset assertions to ensure item properties fall within valid boundaries.
    // Verifies properties such as ensuring maximumStack is greater than 0, strings are not empty,
    // and numbers match logic rules.
    void validateItemDefinition(const ItemDefinition& definition);
}
