#pragma once

#include <string>

#include "simple_platformer/render/sprite.hpp"

namespace simple_platformer
{
    /// Unique identifier token representing a specific type of item in the master database.
    using ItemId = int;

    /**
     * @brief Runtime behavioral hooks triggered when an item is consumed or activated.
     */
    enum class ItemEffect
    {
        None, /// No immediate script evaluation (e.g., pure crafting materials or quest tokens).
        Heal  /// Restores an actor's current health pool points.
    };

    /**
     * @brief The core immutable data structure defining an item's properties in the game database.
     */
    struct ItemDefinition
    {
        /// Unique database key identifying this specific item archetype.
        ItemId id = 0;
        /// User-facing display name of the item.
        std::string name;
        /// Visual sprite element used to draw the item inside inventory menus or UI slots.
        Sprite icon;
        /// Maximum capacity limit for how many instances of this item can occupy a single inventory
        /// slot.
        int maximumStack = 1;
        /// The behavior type applied to the actor when this item is used.
        ItemEffect effect = ItemEffect::None;
        /// The numeric modifier scale associated with the applied effect (e.g., amount of HP
        /// restored).
        int effectAmount = 0;
    };

    /**
     * @brief A dynamic inventory container allocation representing an entry of items.
     */
    struct ItemStack
    {
        /// The database identifier of the item type stored in this allocation block.
        ItemId item = 0;
        /// The current count of items combined inside this specific slot.
        int quantity = 1;
    };

    /**
     * @brief Runs debug asset assertions to ensure item properties fall within valid boundaries.
     *
     * Verifies properties such as ensuring `maximumStack` is greater than 0, strings are not empty,
     * and that numbers match logic rules.
     *
     * @param definition The immutable data block config to evaluate.
     */
    void validateItemDefinition(const ItemDefinition& definition);
}
