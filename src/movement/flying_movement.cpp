/**
 * @file flying_movement.cpp
 * @brief Kinematic movement parameters, configuration validation, and integration routines for
 * airborne physics.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#include "simple_platformer/movement/flying_movement.hpp"

#include <stdexcept>

#include <glm/geometric.hpp>
#include <glm/vec2.hpp>

#include "simple_platformer/input/input_state.hpp"
#include "simple_platformer/math/validation.hpp"
#include "simple_platformer/movement/platformer_movement.hpp"
#include "simple_platformer/physics/body.hpp"
#include "simple_platformer/world/tile_map.hpp"

namespace simple_platformer
{
    // Translates input intent into flying velocity, updates position, and resolves tile map
    // collisions
    CollisionContacts updateFlyingMovement(
        const TileMap& map,
        Body& body,
        const FlyingMovement& movement,
        const InputIntentions& intentions,
        float deltaTime)
    {
        // Enforce simulation time step validity and assert parameters are within finite numerical
        // limits
        requireSeconds(deltaTime, "Flying movement time step");
        if (!isFiniteNonNegative(movement.speed) || !isFinite(intentions.direction))
        {
            throw std::invalid_argument(
                "Flying movement requires a finite, non-negative speed and finite intentions");
        }

        // Clone the requested direction vector to prepare for vector magnitude validation
        glm::vec2 direction = intentions.direction;
        const float length = glm::length(direction);

        // Normalize the vector if it exceeds unit length to prevent accelerated diagonal movement
        if (length > 1.0F)
        {
            direction /= length;
        }

        // Map the normalized input direction vector straight into flying velocity speeds
        body.velocity = direction * movement.speed;

        // Dispatches linear sweep integrations against the tile layout grid and returns resulting
        // wall contact flags
        return moveBody(map, body, deltaTime);
    }
}
