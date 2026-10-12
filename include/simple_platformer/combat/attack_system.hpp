#pragma once

#include "simple_platformer/combat/combat.hpp"
#include "simple_platformer/math/aabb.hpp"
#include "simple_platformer/movement/platformer_movement.hpp"

namespace simple_platformer
{
    class World;
    class WorldRequests;

    /**
     * @brief Calculates the world-space bounding box for a bite attack.
     *
     * Offsets the attack hit box relative to the actor's current bounds and directional
     * orientation.
     *
     * @param actorBounds The bounding box of the actor initiating the attack.
     * @param bite The attack properties containing range and offset parameters.
     * @param facing The current direction the actor is looking (Left or Right).
     * @return An Aabb structure representing the active damage zone.
     */
    Aabb biteHitbox(const Aabb& actorBounds, const BiteAttack& bite, Facing facing);

    /**
     * @brief Updates all active attacks, checks for hits, and applies damage.
     *
     * This function processes combat logic across the frame by:
     * 1. Advancing cooldown timers for active attacks.
     * 2. Constructing hitboxes for actors currently executing strikes.
     * 3. Performing spatial intersection checks against valid target actors.
     * 4. Queuing damage events or hit responses into the requests system.
     *
     * @param world The game world containing the combatant actors.
     * @param requests The request queue used to submit damage events and world changes safely.
     * @param deltaTime The elapsed time since the last frame in seconds.
     */
    void updateAttacks(World& world, WorldRequests& requests, float deltaTime);
}
