/**
 * @file world_requests.hpp
 * @brief Deferment system for processing safe structural modifications and interactions in the game
 * world.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#pragma once

#include <cstddef>
#include <vector>

#include "simple_platformer/actor/actor_id.hpp"
#include "simple_platformer/combat/combat.hpp"

namespace simple_platformer
{
    class World;

    /**
     * @brief Defers structural changes and interactions in the game world.
     *
     * Accumulates requests during the frame update cycle to prevent pointer
     * invalidation or race conditions, safely applying them at specific sync points.
     */
    class WorldRequests
    {
    public:
        /** @brief Queue a damage request for a target actor. */
        void damage(ActorId target, int amount);

        /** @brief Queue the permanent removal of an actor from the world. */
        void remove(ActorId target);

        /** @brief Queue the instantiation of a new projectile. */
        void spawnProjectile(Projectile projectile);

        /** @brief Queue the removal of a projectile by its internal index. */
        void removeProjectile(std::size_t index);

        /** @brief Queue the instantiation of a projectile particle burst. */
        void spawnProjectileBurst(ProjectileBurst burst);

        /** @brief Queue the removal of a projectile burst by its internal index. */
        void removeProjectileBurst(std::size_t index);

        /** @brief Queue a pickup collection event by its internal index. */
        void collectPickup(std::size_t index);

        /** @brief Queue an inventory item usage action for an actor. */
        void useItem(ActorId actor, std::size_t slot);

        /** @brief Checks if there are any pending requests in the queues. */
        bool empty() const;

        /**
         * @brief Returns actors queued for removal.
         * @note External cleanup systems inspect this before the actors are destroyed.
         */
        const std::vector<ActorId>& actorsToRemove() const;

    private:
        struct DamageRequest
        {
            ActorId target;
            int amount = 0;
        };

        struct UseItemRequest
        {
            ActorId actor;
            std::size_t slot = 0;
        };

        friend void updateActorLifecycle(World&, WorldRequests&, float);
        friend void applyWorldRequests(World&, WorldRequests&);

        std::vector<DamageRequest> damageRequests;
        std::vector<ActorId> removalRequests;
        std::vector<Projectile> projectileSpawns;
        std::vector<std::size_t> projectileRemovals;
        std::vector<ProjectileBurst> projectileBurstSpawns;
        std::vector<std::size_t> projectileBurstRemovals;
        std::vector<std::size_t> pickupCollections;
        std::vector<UseItemRequest> itemUses;
    };

    /**
     * @brief Immediately processes structural updates, item usage, and item collections.
     *
     * This function does not advance game time. Queued damage requests are ignored here
     * and are instead consumed by `updateActorLifecycle`.
     *
     * @note Ideal for handling immediate UI interactions while the game simulation is paused.
     */
    void applyWorldRequests(World& world, WorldRequests& requests);
}
