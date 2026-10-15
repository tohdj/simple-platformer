/**
 * @file inventory.cpp
 * @brief Dynamic inventory container class managing item slots, capacity limits, and transactional
 * modifications.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#include "simple_platformer/inventory/inventory.hpp"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <optional>
#include <stdexcept>
#include <vector>

#include "simple_platformer/inventory/item.hpp"

namespace simple_platformer
{
    // Constructor: Allocates a fixed vector tracking the internal stack array storage slots
    Inventory::Inventory(std::size_t slotCount)
        : slotStorage(slotCount)
    {
    }

    // Read-only reference accessor exposing the flat vector layer of slot allocations
    const std::vector<std::optional<ItemStack>>& Inventory::slots() const
    {
        return slotStorage;
    }

    // Accumulates total quantity metrics across all vector slots containing the targeted item key
    int Inventory::count(ItemId item) const
    {
        int total = 0;
        for (const auto& slot : slotStorage)
        {
            if (slot.has_value() && slot->item == item)
            {
                total += slot->quantity;
            }
        }
        return total;
    }

    // Inserts items into the container layout by first filling existing matching item stacks before
    // populating empty slots
    AddItemResult Inventory::add(const ItemDefinition& definition, int quantity)
    {
        validateItemDefinition(definition);

        // Overflow Guard Pass: Prevent input variables from creating numeric integer wrap-arounds
        if (quantity <= 0 || quantity > std::numeric_limits<int>::max() - count(definition.id))
        {
            throw std::invalid_argument("Item quantity must be positive and fit in an int");
        }

        int remaining = quantity;

        // Phase 1: Scan slots to top up un-filled matching item stacks up to their maximum capacity
        // limit
        for (auto& slot : slotStorage)
        {
            if (slot.has_value() && slot->item == definition.id)
            {
                const int added =
                    std::min(remaining, std::max(0, definition.maximumStack - slot->quantity));
                slot->quantity += added;
                remaining -= added;
            }
        }

        // Phase 2: If items remain, distribute them into completely unallocated/empty slots
        for (auto& slot : slotStorage)
        {
            if (remaining > 0 && !slot.has_value())
            {
                const int added = std::min(remaining, definition.maximumStack);
                slot = ItemStack{definition.id, added};
                remaining -= added;
            }
        }
        return {quantity - remaining, remaining};
    }

    // Scans storage linearly to deduct a specific volume of items across matching stacks
    bool Inventory::remove(ItemId item, int quantity)
    {
        if (item <= 0 || quantity <= 0)
        {
            throw std::invalid_argument("Removal requires an item and positive quantity");
        }
        if (count(item) < quantity)
        {
            return false; // Total inventory stores fall short of the deduction requirement
        }

        for (auto& slot : slotStorage)
        {
            if (slot.has_value() && slot->item == item)
            {
                const int removed = std::min(quantity, slot->quantity);
                slot->quantity -= removed;
                quantity -= removed;

                // Completely erase the optional entry allocation if its item count drops to zero
                if (slot->quantity == 0)
                {
                    slot.reset();
                }
            }
        }
        return true;
    }

    // Targets a single slot node location explicitly via array index parameters to deduct a
    // specific item volume
    bool Inventory::removeFromSlot(std::size_t slot, int quantity)
    {
        if (quantity <= 0)
        {
            throw std::invalid_argument("Removal quantity must be positive");
        }
        if (slot >= slotStorage.size())
        {
            return false; // Provided index falls outside internal array boundary configurations
        }

        auto& selected = slotStorage[slot];
        if (!selected.has_value() || selected->quantity < quantity)
        {
            return false; // Target slot is either vacant or lacks enough items to complete
                          // deduction
        }

        selected->quantity -= quantity;
        if (selected->quantity == 0)
        {
            selected.reset(); // Safely clear the data wrapper block upon zeroing out
        }
        return true;
    }
}
