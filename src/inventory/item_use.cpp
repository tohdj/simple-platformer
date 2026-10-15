/**
 * @file item_use.cpp
 * @brief Logic and invocation handlers for triggering item usage from an actor's inventory slot.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#include "simple_platformer/inventory/item_use.hpp"

#include <algorithm>
#include <cstddef>

#include "simple_platformer/actor/actor.hpp"
#include "simple_platformer/actor/actor_id.hpp"
#include "simple_platformer/inventory/inventory.hpp"
#include "simple_platformer/inventory/item.hpp"
#include "simple_platformer/world/world.hpp"

namespace simple_platformer
{
    // Attempts to activate or consume an item from a specific inventory slot grid index
    bool useItem(World& world, ActorId id, std::size_t slot)
    {
        Actor* actor = world.findActor(id);

        // Safety Guard Pass: Verify target presence, active lifecycle state, inventory components,
        // and index array bounds
        if (actor == nullptr || actor->life != LifeState::Alive || !actor->inventory.has_value() ||
            slot >= actor->inventory->slots().size())
        {
            return false;
        }

        const auto& selected = actor->inventory->slots()[slot];
        if (!selected.has_value())
        {
            return false; // Target slot allocation is completely empty
        }

        // Isolate the immutable blueprint properties matching the targeted item from the database
        // catalog
        const ItemDefinition& definition = world.itemDefinition(selected->item);
        switch (definition.effect)
        {
        case ItemEffect::None:
            return false; // Material or token items carry no functional script activation hooks

        case ItemEffect::Heal:
            // Skip execution if health metrics are unassigned or the current pool is completely
            // saturated
            if (!actor->health.has_value() || actor->health->current >= actor->health->maximum)
            {
                return false;
            }

            // Add point modifications cleanly, checking boundaries to prevent exceeding the health
            // maximum ceiling
            actor->health->current +=
                std::min(definition.effectAmount, actor->health->maximum - actor->health->current);

            // Deduct a single count of the item from the container slot inventory record upon
            // success
            actor->inventory->removeFromSlot(slot, 1);
            return true;
        }
        return false;
    }
}
