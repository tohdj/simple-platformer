/**
 * @file pickup.cpp
 * @brief Logic, structure validation, and physics updates for collectible item pickups in the
 * world.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#include "simple_platformer/world/pickup.hpp"

#include <cstddef>
#include <stdexcept>
#include <vector>

#include "simple_platformer/actor/actor.hpp"
#include "simple_platformer/inventory/inventory.hpp"
#include "simple_platformer/inventory/item.hpp"
#include "simple_platformer/math/aabb.hpp"
#include "simple_platformer/math/validation.hpp"
#include "simple_platformer/physics/body.hpp"
#include "simple_platformer/world/world.hpp"
#include "simple_platformer/world/world_requests.hpp"

namespace simple_platformer
{
    // Validates internal pickup nodes to guarantee finite kinematic dimensions and meaningful
    // counts
    void validatePickup(const Pickup& pickup)
    {
        if (!simple_platformer::isFinite(pickup.body.bounds.topLeft) ||
            !simple_platformer::isFinite(pickup.body.bounds.size) ||
            !simple_platformer::isFinite(pickup.body.velocity) ||
            pickup.body.bounds.size.x <= 0.0F || pickup.body.bounds.size.y <= 0.0F)
        {
            throw std::invalid_argument("Pickups require a finite body with positive-sized bounds");
        }
        if (pickup.stack.quantity <= 0)
        {
            throw std::invalid_argument("Pickups require a positive quantity");
        }
    }

    // Injects a validated pickup instance into the active world layer if the item ID is registered
    void World::addPickup(Pickup pickup)
    {
        validatePickup(pickup);
        itemDefinition(pickup.stack.item); // Assert that the item definition exists in the database
        pickupStorage.push_back(pickup);
    }

    // Accessors for immutable and mutable collections of floating environment items
    const std::vector<Pickup>& World::pickups() const
    {
        return pickupStorage;
    }

    std::vector<Pickup>& World::pickups()
    {
        return pickupStorage;
    }

    // Transfers item counts from an environmental node into the player character inventory
    // container
    void World::collectPickup(std::size_t index)
    {
        Actor* player = findActor(playerId());
        // Verify target entity bounds validity, alive states, and valid inventory component
        // linkages
        if (index >= pickupStorage.size() || player == nullptr ||
            player->life != LifeState::Alive || !player->inventory.has_value())
        {
            return;
        }

        Pickup& pickup = pickupStorage[index];
        // Attempt to insert the item stack contents into the inventory slots layout
        const AddItemResult result =
            player->inventory->add(itemDefinition(pickup.stack.item), pickup.stack.quantity);

        // Update the pickup quantity with whatever could not fit into the player's packs
        pickup.stack.quantity = result.remaining;

        // Completely destroy the entity item node if the entire stack was transferred successfully
        if (result.remaining == 0)
        {
            pickupStorage.erase(pickupStorage.begin() + static_cast<std::ptrdiff_t>(index));
        }
    }

    // Simulates kinematic environmental physics updates (gravity and tile maps collision checks)
    void updatePickupMovement(const TileMap& map, World& world, float deltaTime)
    {
        requireSeconds(deltaTime, "Pickup movement time step");
        for (Pickup& pickup : world.pickups())
        {
            // Apply standard gravitational acceleration forces down onto the rigid body properties
            applyGravity(pickup.body, DefaultGravity, DefaultMaximumFallSpeed, deltaTime);
            // Move item tracking structures across the layout, parsing walls and floors collisions
            moveBody(map, pickup.body, deltaTime);
        }
    }

    // Checks spatial boundaries intersections and schedules collection tasks for overlapping items
    void updatePickups(const World& world, WorldRequests& requests)
    {
        const Actor* player = world.findActor(world.playerId());
        // Skip spatial search computations if the hero entity cannot accept items
        if (player == nullptr || player->life != LifeState::Alive || !player->inventory.has_value())
        {
            return;
        }

        // Search through the environment vectors array for bounding boxes intersection overlap
        // anomalies
        for (std::size_t index = 0; index < world.pickups().size(); ++index)
        {
            if (overlaps(player->body.bounds, world.pickups()[index].body.bounds))
            {
                // Queue a deferred capture update to prevent standard database vector parsing
                // corruptions
                requests.collectPickup(index);
            }
        }
    }
}
