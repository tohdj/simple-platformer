/**
 * @file projectile_simulation.hpp
 * @brief Simulates velocities, tracks lifecycles, and resolves collisions for world projectiles and
 * bursts.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#pragma once

namespace simple_platformer
{
    class TileMap;
    class World;
    class WorldRequests;

    /**
     * @brief Simulates, moves, and checks collisions for all active projectiles in the world.
     *
     * This function executes the following workflow during the frame step:
     * 1. Updates positions by integrating projectile velocities across the elapsed time frame.
     * 2. Evaluates AABB overlaps against solid static environmental elements in the tile map.
     * 3. Performs spatial opponent intersection sweeps against valid target actors.
     * 4. Deducts projectile lifetime values and queues structural impact damage or burst effects
     * via requests.
     *
     * @param map The static environment geometry grid used for spatial collision detection.
     * @param world The game world containing the collections of active projectiles and actors.
     * @param requests The request queue used to safely submit actor damage events and projectile
     * removals.
     * @param deltaTime The elapsed time since the last frame in seconds.
     */
    void updateProjectiles(TileMap& map, World& world, WorldRequests& requests, float deltaTime);

    /**
     * @brief Updates the playback tracking states for transient projectile impact particles.
     *
     * Manages the lifecycle of explosion animations or hit effect fragments spawned when a
     * projectile contacts an object. Decrements performance life timers and cleans up expired burst
     * items.
     *
     * @param world The game world containing the collection of active projectile burst instances.
     * @param requests The request queue used to dispatch burst deletion commands upon sequence
     * expiration.
     * @param deltaTime The elapsed time since the last frame in seconds.
     */
    void updateProjectileBursts(World& world, WorldRequests& requests, float deltaTime);
}
