/**
 * @file platformer_movement.hpp
 * @brief Kinematic movement parameters, configuration validation, and integration routines for
 * platformer physics.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#pragma once

#include "simple_platformer/input/input_state.hpp"
#include "simple_platformer/physics/body.hpp"

namespace simple_platformer
{
    class TileMap;

    // Horizontal orientation tracking indicating the direction an actor is looking or moving.
    enum class Facing
    {
        Left,
        Right
    };

    // Physics parameters defining locomotion profiles for platformer-style movement.
    struct PlatformerMovementConfig
    {
        float maximumSpeed =
            100.0F; // Maximum horizontal velocity limit in world pixels per second.
        float groundAcceleration = 800.0F; // Rate of speed gain applied horizontally while standing
                                           // on solid floor surfaces.
        float airAcceleration = 400.0F; // Rate of speed gain applied horizontally while airborne.
        float groundDeceleration = 1000.0F; // Braking force scale applied horizontally when no
                                            // horizontal input is active on the ground.
        float jumpSpeed =
            240.0F; // Initial upward vertical velocity applied instantly at the moment of jumping.
        float gravity = DefaultGravity; // Downward acceleration scale applied under standard
                                        // falling or rising circumstances.
        float jumpReleaseGravity = 2.0F * DefaultGravity; // Increased gravity scale applied if the
                                                          // player releases the jump button early.
        float maximumFallSpeed = DefaultMaximumFallSpeed; // Maximum downward terminal velocity cap
                                                          // to prevent excessive falling speed.
        float coyoteDuration = 0.1F;     // Grace window duration in seconds allowing jumps shortly
                                         // after running off an edge.
        float jumpBufferDuration = 0.1F; // Time window duration in seconds to cache a jump command
                                         // pressed slightly before touching down.
    };

    // Asserts and verifies that movement parameters fall within mathematically logical,
    // non-negative limits.
    void validatePlatformerMovementConfig(const PlatformerMovementConfig& config);

    // Evaluates two movement configurations for field-by-field value equality.
    bool operator==(const PlatformerMovementConfig& left, const PlatformerMovementConfig& right);

    // Evaluates two movement configurations for value inequality.
    bool operator!=(const PlatformerMovementConfig& left, const PlatformerMovementConfig& right);

    // Evaluates input parameters to decide orientation, letting aiming intentions override
    // horizontal movement direction.
    Facing facingFor(const InputIntentions& intentions, Facing current);

    // Component storing runtime simulation flags and memory variables for platformer motion.
    struct PlatformerMovement
    {
        PlatformerMovementConfig
            config; // The physics coefficients profile guiding this actor's motion rule logic.
        bool grounded = false; // True if the actor's physics bounding box was firmly supported by a
                               // floor surface last update tick.
        float coyoteRemaining =
            0.0F; // Tracking clock timer in seconds monitoring the remaining coyote grace window.
        float jumpBufferRemaining = 0.0F; // Tracking clock timer in seconds measuring the
                                          // expiration threshold of buffered jump clicks.
        bool blocked = false; // Flag indicating horizontal movement was actively arrested by a
                              // solid wall obstacle or ledge guard block.
    };

    // Simulates running acceleration, jump inputs processing, jump-hop gravity damping, and
    // physical tile collision sweeps.
    CollisionContacts updatePlatformerMovement(
        const TileMap& map,
        Body& body,
        PlatformerMovement& movement,
        const InputIntentions& intentions,
        float deltaTime);
}
