/**
 * @file        actor_system.cpp
 * @brief       Declares environmental update functions responsible for moving actors and resolving
 * geometric tile collisions.
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright   Copyright (c) 2026 Simple Platformer. All rights reserved.
 */

#include "simple_platformer/actor/actor_system.hpp"

#include <stdexcept>

#include "simple_platformer/actor/actor.hpp"
#include "simple_platformer/input/input_state.hpp"
#include "simple_platformer/math/aabb.hpp"
#include "simple_platformer/movement/flying_movement.hpp"
#include "simple_platformer/movement/platformer_movement.hpp"
#include "simple_platformer/movement/surface_climb.hpp"
#include "simple_platformer/world/tile_map.hpp"
#include "simple_platformer/world/world.hpp"

namespace simple_platformer
{
    // Evaluates and updates physical position properties across all actors by routing their input
    // intentions to relevant locomotion systems
    void updateActorMovement(const TileMap& map, World& world, float deltaTime)
    {
        for (Actor& actor : world.actors())
        {
            // Reset input values completely if the actor is dead to prevent post-mortem movement
            // drift
            const InputIntentions intentions =
                actor.life == LifeState::Alive ? actor.intentions : InputIntentions{};

            // 1. Evaluate walking, running, and jumping states for traditional platformer actors
            if (actor.platformerMovement.has_value())
            {
                PlatformerMovement& movement = *actor.platformerMovement;
                const bool wasGrounded = movement.grounded;

                if (actor.surfaceClimb.has_value())
                {
                    // Update climbing locomotion code if the actor is currently clinging to a wall
                    // or ceiling surface
                    updateSurfaceClimbMovement(
                        map, actor.body, movement, *actor.surfaceClimb, intentions, deltaTime);
                }
                else
                {
                    // Apply standard horizontal running and vertical jumping forces relative to
                    // static geometry bounds
                    updatePlatformerMovement(map, actor.body, movement, intentions, deltaTime);
                }

                // Sound Event Trigger: emit a mechanical landing noise if a living character
                // transitions from mid-air to solid ground
                if (!wasGrounded && movement.grounded && actor.life == LifeState::Alive)
                {
                    world.emitNoise({actor.id, feetOf(actor.body.bounds), NoiseKind::Landing});
                }
            }
            // 2. Evaluate flying or floating actors that ignore default platform gravity parameters
            else if (actor.flyingMovement.has_value())
            {
                updateFlyingMovement(map, actor.body, *actor.flyingMovement, intentions, deltaTime);
            }
            else
            {
                throw std::logic_error("An actor has no movement component");
            }

            // 3. Update the actor's sprite flip orientation context flag based on its updated
            // directional intentions mapping
            actor.facing = facingFor(intentions, actor.facing);
        }
    }
}
