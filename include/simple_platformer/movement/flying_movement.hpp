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

    // Component defining speed limits for non-gravitational, multi-directional flying entities
    struct FlyingMovement
    {
        float speed = 60.0F; // Maximum movement speed measured in world pixels per second
    };

    // Translates input intent into flying velocity, updates position, and resolves tile map
    // collisions
    CollisionContacts updateFlyingMovement(
        const TileMap& map,
        Body& body,
        const FlyingMovement& movement,
        const InputIntentions& intentions,
        float deltaTime);
}
