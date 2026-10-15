/**
 * @file        actor_lifecycle.hpp
 * @brief       Declares functions and constants responsible for handling damage processing, death
 * timers, and lifecycle updates.
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright   Copyright (c) 2026 Simple Platformer. All rights reserved.
 */

#pragma once

namespace simple_platformer
{
    class World;
    class WorldRequests;

    // The duration (in seconds) that an actor remains in a death state before cleanup or respawn
    constexpr float ActorDeathSeconds = 0.4F;

    // Resolves pending damage, updates death timers, handles player respawns, and schedules dead
    // NPC cleanups
    void updateActorLifecycle(World& world, WorldRequests& requests, float deltaTime);
}
