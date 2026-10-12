/**
 * @file flying_movement.hpp
 * @brief Kinematic movement parameters, configuration validation, and integration routines for
 * airborne physics.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#pragma once

#include "simple_platformer/physics/body.hpp"

namespace simple_platformer
{
    class TileMap;
    struct Body;
    struct InputIntentions;
    enum class Facing;

    /**
     * @brief Component defining movement parameters for flying entities.
     *
     * Configures non-gravitational, multi-directional locomotion behaviors
     * (e.g., used by floating enemies, bats, or spectres).
     */
    struct FlyingMovement
    {
        float speed = 60.0F; /**< Maximum movement speed measured in world pixels per second. */
    };

    /**
     * @brief Processes flying physics calculations and resolves environmental collisions.
     *
     * This function executes the following steps during the update frame:
     * 1. Translates the target intent direction vectors into flying velocities.
     * 2. Integrates position changes onto the body based on delta time.
     * 3. Performs AABB intersection sweeps against the tile map layout geometry.
     * 4. Corrects overlapping penetrations and reports hit boundary surfaces.
     *
     * @param map The static environment geometry grid used for collision checks.
     * @param body The physics body structure containing location, velocity, and bounds.
     * @param movement The structural speed limits and dampening configurations.
     * @param intentions High-level input direction values requested this update tick.
     * @param deltaTime The elapsed time since the last frame in seconds.
     * @return A CollisionContacts bitmask containing flags for all surfaces contacted (e.g., left
     * wall, ceiling).
     */
    CollisionContacts updateFlyingMovement(
        const TileMap& map,
        Body& body,
        const FlyingMovement& movement,
        const InputIntentions& intentions,
        float deltaTime);
}
