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
    // The data payload returned by item insertion attempts.
    struct AddItemResult
    {
        int added = 0; // Total number of item units successfully placed into the inventory storage.
        int remaining = 0; // Number of item units that could not fit because storage capacity was
                           // completely exhausted.
    };

    // Encapsulated data structure managing a fixed collection of item slots.
    // Owns its underlying slots to guarantee callers cannot bypass item stack maximums, quantity
    // bounds, or type rules. Empty slots retain their indices to ensure stable slot references.
    class Inventory
    {
    public:
        // Constructs an inventory container with a specified number of item slots. Defaults to 8
        // slots.
        explicit Inventory(std::size_t slotCount = 8);

        // Read-only accessor exposing the underlying collection of inventory slots.
        const std::vector<std::optional<ItemStack>>& slots() const;

        // Calculates the total cumulative count of a specific item across all slots.
        int count(ItemId item) const;

        // Attempts to insert items into the inventory system.
        // Fills existing matching item stacks up to their limit before allocating remaining units
        // into available empty slots.
        AddItemResult add(const ItemDefinition& definition, int quantity);

        // Attempts to remove a specific aggregate quantity of items across the inventory.
        // Operates under an "all-or-nothing" rule: if the total count is insufficient, zero
        // mutations occur.
        bool remove(ItemId item, int quantity);

        // Deducts an item quantity explicitly from a targeted inventory index.
        // Fails instantly if the single targeted slot holds fewer units than requested.
        bool removeFromSlot(std::size_t slot, int quantity);

    private:
        // The contiguous container backing block holding item tracking slots. A std::nullopt entry
        // represents an empty, available slot.
        std::vector<std::optional<ItemStack>> slotStorage;
    };
}
