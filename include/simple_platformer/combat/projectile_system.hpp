/**
 * @file projectile_system.hpp
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

    // Simulates movement, checks environmental collisions, sweeps target actors, and deducts
    // projectile lifespans
    void updateProjectiles(TileMap& map, World& world, WorldRequests& requests, float deltaTime);

    // Updates playback states and handles cleanup timers for transient projectile impact particles
    void updateProjectileBursts(World& world, WorldRequests& requests, float deltaTime);
}
