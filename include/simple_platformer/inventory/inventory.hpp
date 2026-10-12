/**
 * @file inventory.hpp
 * @brief Dynamic inventory container class managing item slots, capacity limits, and transactional
 * modifications.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#pragma once

#include <cstddef>
#include <optional>
#include <vector>

#include "simple_platformer/inventory/item.hpp"

namespace simple_platformer
{
    /**
     * @brief The data payload returned by item insertion attempts.
     */
    struct AddItemResult
    {
        int added =
            0; /**< Total number of item units successfully placed into the inventory storage. */
        int remaining = 0; /**< Number of item units that could not fit because storage capacity was
                              completely exhausted. */
    };

    /**
     * @brief Encapsulated data structure managing a fixed collection of item slots.
     *
     * Owns its underlying slots to guarantee callers cannot bypass item stack maximums,
     * quantity bounds, or type rules. Empty slots retain their indices to ensure stable slot
     * references.
     */
    class Inventory
    {
    public:
        /**
         * @brief Constructs an inventory container with a specified number of item slots.
         * @param slotCount The fixed size of the slot container layout. Defaults to 8 slots.
         */
        explicit Inventory(std::size_t slotCount = 8);

        /**
         * @brief Read-only accessor exposing the underlying collection of inventory slots.
         * @return An immutable reference to the vector container containing optional item stacks.
         */
        const std::vector<std::optional<ItemStack>>& slots() const;

        /**
         * @brief Calculates the total cumulative count of a specific item across all slots.
         * @param item The database identifier of the target item type to search for.
         * @return Total quantity count found across all matching stacks.
         */
        int count(ItemId item) const;

        /**
         * @brief Attempts to insert items into the inventory system.
         *
         * The insertion algorithm executes the following sequence:
         * 1. Iterates through existing matching item stacks to fill them up to their definition
         * limit.
         * 2. If item units remain, allocates them into any available empty slots.
         * 3. Calculates and returns the final insertion yield configuration.
         *
         * @param definition The configuration properties defining the item being added.
         * @param quantity Total number of units requested to be placed in storage.
         * @return An AddItemResult indicating how many units were accommodated and how many were
         * rejected.
         */
        AddItemResult add(const ItemDefinition& definition, int quantity);

        /**
         * @brief Attempts to remove a specific aggregate quantity of items across the inventory.
         *
         * Operates under an "all-or-nothing" rule: if the total available inventory count is less
         * than the requested quantity, the function fails instantly and makes zero mutations.
         * Emptying a slot clears the slot payload but leaves the index position stable without
         * shifting elements.
         *
         * @param item The database identifier of the target item type to deduct.
         * @param quantity The number of total items to remove from active storage.
         * @return true if the full quantity was successfully located and removed, false otherwise.
         */
        bool remove(ItemId item, int quantity);

        /**
         * @brief Deducts an item quantity explicitly from a targeted inventory index.
         *
         * Similar to the general remove function, if the individual slot holds fewer units than
         * requested, no changes occur.
         *
         * @param slot The zero-based array index position of the item slot to deduct from.
         * @param quantity The number of items to subtract from this specific stack slot.
         * @return true if the item slot contained sufficient quantity and was successfully
         * deducted, false otherwise.
         */
        bool removeFromSlot(std::size_t slot, int quantity);

    private:
        std::vector<std::optional<ItemStack>>
            slotStorage; /**< The contiguous container backing block holding item tracking slots. A
                            `std::nullopt` entry represents an empty, available slot. */
    };
}
