/**
 * @file world_requests.cpp
 * @brief Deferment system for processing safe structural modifications and interactions in the game
 * world.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#include "simple_platformer/world/world_requests.hpp"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <stdexcept>
#include <vector>

#include "simple_platformer/actor/actor_id.hpp"
#include "simple_platformer/combat/combat.hpp"
#include "simple_platformer/inventory/item_use.hpp"
#include "simple_platformer/world/world.hpp"

namespace simple_platformer
{
    // Queues a request to deal damage to a target actor
    void WorldRequests::damage(ActorId target, int amount)
    {
        // Enforce valid actor references and non-negative damage values
        if (!isValid(target) || amount <= 0)
        {
            throw std::invalid_argument("Damage requests require a target and positive amount");
        }
        damageRequests.push_back({target, amount});
    }

    // Queues a request to safely destroy or remove an actor from the game world
    void WorldRequests::remove(ActorId target)
    {
        if (!isValid(target))
        {
            throw std::invalid_argument("Removal requests require a target");
        }
        removalRequests.push_back(target);
    }

    // Queues a request to spawn a new single projectile
    void WorldRequests::spawnProjectile(Projectile projectile)
    {
        projectileSpawns.push_back(projectile);
    }

    // Queues a removal request for a projectile based on its container index
    void WorldRequests::removeProjectile(std::size_t index)
    {
        projectileRemovals.push_back(index);
    }

    // Queues a request to spawn a cluster or burst of projectiles
    void WorldRequests::spawnProjectileBurst(ProjectileBurst burst)
    {
        projectileBurstSpawns.push_back(burst);
    }

    // Queues a removal request for a projectile burst container entry by index
    void WorldRequests::removeProjectileBurst(std::size_t index)
    {
        projectileBurstRemovals.push_back(index);
    }

    // Checks if there are any deferred world structural or interaction changes pending
    bool WorldRequests::empty() const
    {
        return damageRequests.empty() && removalRequests.empty() && projectileSpawns.empty() &&
               projectileRemovals.empty() && projectileBurstSpawns.empty() &&
               projectileBurstRemovals.empty() && pickupCollections.empty() && itemUses.empty();
    }

    // Provides read access to the batch of actors scheduled for imminent removal
    const std::vector<ActorId>& WorldRequests::actorsToRemove() const
    {
        return removalRequests;
    }

    // Queues a request to collect an item pickup by index
    void WorldRequests::collectPickup(std::size_t index)
    {
        pickupCollections.push_back(index);
    }

    // Queues an action where a specific actor uses an item from a designated slot
    void WorldRequests::useItem(ActorId actor, std::size_t slot)
    {
        if (!isValid(actor))
        {
            throw std::invalid_argument("Item use requires an actor");
        }
        itemUses.push_back({actor, slot});
    }

    namespace
    {
        // Helper function to safely execute vector index-based cleanups.
        // Removing by index shifts everything after it down, so the indexes go highest first
        // and each one is removed only once however many times it was requested.
        void removeEachHighestFirst(
            std::vector<std::size_t>& indexes,
            const std::function<void(std::size_t)>& remove)
        {
            // Sort ascending to group duplicate indices together
            std::sort(indexes.begin(), indexes.end());
            // Discard any duplicated entries to prevent out-of-bounds errors on multi-triggers
            indexes.erase(std::unique(indexes.begin(), indexes.end()), indexes.end());
            // Process reverse order (highest index first) so index shifts do not ruin preceding
            // offsets
            for (auto index = indexes.rbegin(); index != indexes.rend(); ++index)
            {
                remove(*index);
            }
        }
    }

    // Dispatches all queued mutations to the game world instance safely and flushes the queue
    void applyWorldRequests(World& world, WorldRequests& requests)
    {
        // 1. Process item activations first
        for (const auto& use : requests.itemUses)
        {
            useItem(world, use.actor, use.slot);
        }

        // 2. Clear out collected environment pickups safely
        removeEachHighestFirst(
            requests.pickupCollections,
            [&world](std::size_t index) { world.collectPickup(index); });

        // 3. Process safe removal operations for tracked actors
        for (const ActorId id : requests.removalRequests)
        {
            world.removeActor(id);
        }

        // 4. Update the active projectile vectors safely
        removeEachHighestFirst(
            requests.projectileRemovals,
            [&world](std::size_t index) { world.removeProjectile(index); });
        for (const Projectile& projectile : requests.projectileSpawns)
        {
            world.addProjectile(projectile);
        }

        // 5. Update the burst particle systems vectors safely
        removeEachHighestFirst(
            requests.projectileBurstRemovals,
            [&world](std::size_t index) { world.removeProjectileBurst(index); });
        for (const ProjectileBurst& burst : requests.projectileBurstSpawns)
        {
            world.addProjectileBurst(burst);
        }

        // 6. Reset all request queues to receive the next frame's deferred actions
        requests.itemUses.clear();
        requests.pickupCollections.clear();
        requests.removalRequests.clear();
        requests.projectileSpawns.clear();
        requests.projectileRemovals.clear();
        requests.projectileBurstSpawns.clear();
        requests.projectileBurstRemovals.clear();
    }
}
