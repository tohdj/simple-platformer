/**
 * @file projectile_system.cpp
 * @brief Simulates velocities, tracks lifecycles, and resolves collisions for world projectiles and
 * bursts.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#include "simple_platformer/combat/projectile_system.hpp"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <vector>

#include <glm/vec2.hpp>

#include "simple_platformer/actor/actor.hpp"
#include "simple_platformer/actor/actor_id.hpp"
#include "simple_platformer/combat/combat.hpp"
#include "simple_platformer/math/aabb.hpp"
#include "simple_platformer/math/coordinates.hpp"
#include "simple_platformer/math/validation.hpp"
#include "simple_platformer/physics/segment_cast.hpp"
#include "simple_platformer/world/tile_map.hpp"
#include "simple_platformer/world/world.hpp"
#include "simple_platformer/world/world_requests.hpp"

namespace simple_platformer
{
    namespace
    {
        // Tracks data regarding what the projectile struck along its path segment
        struct ProjectileHit
        {
            float segmentTime = 1.0F; // Normalized time [0.0, 1.0] along the movement vector
            std::optional<ActorId> actor;
            std::optional<Cell> tile;

            // Evaluates true if either an actor target or static environment tile was struck
            bool occurred() const
            {
                return actor.has_value() || tile.has_value();
            }
        };

        // Factory function configuring a structural burst visual effect configuration
        ProjectileBurst makeBurst(
            const Projectile& projectile,
            glm::vec2 center,
            ProjectileBurstCause cause)
        {
            ProjectileBurst burst;
            burst.cause = cause;
            burst.center = center;
            burst.direction = projectile.velocity;

            // Fallback direction vector if the projectile was completely static on impact
            if (burst.direction.x == 0.0F && burst.direction.y == 0.0F)
            {
                burst.direction = {1.0F, 0.0F};
            }
            burst.sprite = projectile.sprite;
            return burst;
        }

        // Performs swept collision casts to find the nearest structural impact along the frame path
        ProjectileHit findEarliestHit(
            const TileMap& map,
            const World& world,
            const Projectile& projectile,
            glm::vec2 start,
            glm::vec2 end)
        {
            ProjectileHit earliest;

            // Step 1: Perform ray/box sweep against static solid environment layers
            const std::optional<TileSegmentHit> tileHit =
                segmentCastMovementBlockingTiles(map, start, end, projectile.bounds.size);
            if (tileHit.has_value())
            {
                earliest.segmentTime = tileHit->segmentTime;
                earliest.tile = tileHit->cell;
            }

            // Step 2: Sweep against active characters to register hit tracking markers
            for (const Actor& actor : world.actors())
            {
                // Skip dead units, non-combatants, teammates, or the projectile's original shooter
                if (actor.life != LifeState::Alive || !actor.health.has_value() ||
                    !areOpponents(projectile.team, actor.team) || projectile.owner == actor.id)
                {
                    continue;
                }

                // Expand actor AABB by projectile dimensions to evaluate sweeping line intersection
                const std::optional<float> actorHit = segmentCast(
                    expandedForMovingBox(actor.body.bounds, projectile.bounds.size), start, end);

                // Prioritize this hit if it occurs earlier than the current recorded closest
                // contact point
                if (actorHit.has_value() &&
                    (*actorHit < earliest.segmentTime ||
                     (!earliest.occurred() && *actorHit == earliest.segmentTime)))
                {
                    earliest.segmentTime = *actorHit;
                    earliest.actor = actor.id;
                    earliest.tile
                        .reset(); // Actor intersection overrides the environmental collision index
                }
            }
            return earliest;
        }

        // Queues request payloads to spawn visual impact particles and destroy the projectile
        // entity
        void endProjectile(
            WorldRequests& requests,
            const Projectile& projectile,
            std::size_t index,
            glm::vec2 center,
            ProjectileBurstCause cause)
        {
            requests.spawnProjectileBurst(makeBurst(projectile, center, cause));
            requests.removeProjectile(index);
        }
    }

    // Simulates movement, checks environmental collisions, sweeps target actors, and deducts
    // projectile lifespans
    void updateProjectiles(TileMap& map, World& world, WorldRequests& requests, float deltaTime)
    {
        // Validate that the incoming time slice is a valid physical duration
        requireSeconds(deltaTime, "Projectiles time step");

        // Collect tiles broken this frame to defer destruction until the end of the update loop.
        // This ensures later projectiles don't bypass walls broken by earlier projectiles in the
        // same tick.
        std::vector<Cell> brokenTiles;

        // Loop through the continuous linear buffer of active world projectiles
        for (std::size_t index = 0; index < world.projectiles().size(); ++index)
        {
            Projectile& projectile = world.projectiles()[index];

            // Calculate the ray origin point from the center of the bounding box
            const glm::vec2 start = centerOf(projectile.bounds);

            // Do not simulate travel past the remaining lifespan of the projectile
            const float travelTime = std::min(deltaTime, projectile.lifetimeRemaining);

            // Extrapolate the intended end position vector based on linear velocity integration
            const glm::vec2 end = start + projectile.velocity * travelTime;

            // Sweep spatial paths to find the closest actor or environmental intersection point
            const ProjectileHit hit = findEarliestHit(map, world, projectile, start, end);

            // Interpolate the exact point of impact using the normalized intersection time segment
            const glm::vec2 finalCenter = start + (end - start) * hit.segmentTime;

            // Reposition the top-left structural origin of the projectile boundary box at the new
            // position
            projectile.bounds.topLeft = finalCenter - projectile.bounds.size * 0.5F;

            // Route execution based on what item was hit
            if (hit.actor.has_value())
            {
                // Queue up a damage application command to be dispatched against the target actor
                // ID
                requests.damage(*hit.actor, projectile.damage);
            }
            if (hit.tile.has_value() && projectile.breaksTiles)
            {
                // Defer environmental cell damage to maintain processing consistency for other
                // projectiles
                brokenTiles.push_back(*hit.tile);
            }

            // Process entity termination configurations
            if (hit.occurred())
            {
                // Terminate immediately upon impacting a solid collider surface or character
                endProjectile(
                    requests, projectile, index, finalCenter, ProjectileBurstCause::Impact);
            }
            else
            {
                // Update the decay timer if the projectile flew through open space cleanly
                projectile.lifetimeRemaining -= deltaTime;
                if (projectile.lifetimeRemaining <= 0.0F)
                {
                    // Clean up entity because the ammunition spent its full fuel duration range
                    endProjectile(
                        requests,
                        projectile,
                        index,
                        finalCenter,
                        ProjectileBurstCause::LifetimeExpired);
                }
            }
        }

        // Apply deferred tile breaks after all projectile paths have been calculated
        for (const Cell cell : brokenTiles)
        {
            map.breakTile(cell);
        }
    }

    // Updates playback states and handles cleanup timers for transient projectile impact particles
    void updateProjectileBursts(World& world, WorldRequests& requests, float deltaTime)
    {
        // Validate that the system delta value is valid before running particle steps
        requireSeconds(deltaTime, "Projectile bursts time step");

        // Iterate through all active visual effects and clear expired nodes
        for (std::size_t index = 0; index < world.projectileBursts().size(); ++index)
        {
            ProjectileBurst& burst = world.projectileBursts()[index];

            // Apply temporal structural frame step decay logic to the tracking particle life
            burst.lifetimeRemaining -= deltaTime;
            if (burst.lifetimeRemaining <= 0.0F)
            {
                // Send visual effect erasure command up to the central event dispatch queue
                requests.removeProjectileBurst(index);
            }
        }
    }
}
