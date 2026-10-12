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

    /**
     * @brief Horizontal orientation tracking indicating the direction an actor is looking or
     * moving.
     */
    enum class Facing
    {
        Left,
        Right
    };

    /**
     * @brief Physics parameters defining locomotion profiles for platformer-style movement.
     */
    struct PlatformerMovementConfig
    {
        float maximumSpeed =
            100.0F; /**< Maximum horizontal velocity limit in world pixels per second. */
        float groundAcceleration = 800.0F; /**< Rate of speed gain applied horizontally while
                                              standing on solid floor surfaces. */
        float airAcceleration =
            400.0F; /**< Rate of speed gain applied horizontally while airborne. */
        float groundDeceleration = 1000.0F; /**< Braking force scale applied horizontally when no
                                               horizontal input is active on the ground. */
        float jumpSpeed = 240.0F; /**< Initial upward vertical velocity applied instantly at the
                                     moment of jumping. */
        float gravity = DefaultGravity; /**< Downward acceleration scale applied under standard
                                           falling or rising circumstances. */
        float jumpReleaseGravity =
            2.0F * DefaultGravity; /**< Increased gravity scale applied if the player releases the
                                      jump button early (allows variable jump control). */
        float maximumFallSpeed =
            DefaultMaximumFallSpeed; /**< Maximum downward terminal velocity cap to prevent
                                        excessive falling speed. */
        float coyoteDuration = 0.1F; /**< Grace window duration in seconds allowing jumps shortly
                                        after running off an edge. */
        float jumpBufferDuration = 0.1F; /**< Time window duration in seconds to cache a jump
                                            command pressed slightly before touching down. */
    };

    /**
     * @brief Asserts and verifies that movement parameters fall within mathematically logical
     * limits.
     * @param config The immutable configuration properties block to inspect.
     * @throws std::invalid_argument or similar exception if validation parameters fall below zero.
     */
    void validatePlatformerMovementConfig(const PlatformerMovementConfig& config);

    /** @brief Evaluates two movement configurations for logical equality. */
    bool operator==(const PlatformerMovementConfig& left, const PlatformerMovementConfig& right);

    /** @brief Evaluates two movement configurations for logical inequality. */
    bool operator!=(const PlatformerMovementConfig& left, const PlatformerMovementConfig& right);

    /**
     * @brief Evaluates input parameters to decide which direction an entity should orient.
     *
     * Applies the following hierarchical determination rules:
     * 1. If an active aiming intent direction exists, aim orientation overrides all.
     * 2. If no aim is input, falls back to matching the horizontal directional movement intent.
     * 3. If zero input intention is active, maintains the actor's current orientation state.
     *
     * @param intentions The input actions and directions captured for this simulation frame.
     * @param current The existing orientation state before evaluating this tick.
     * @return The updated Facing enumeration value.
     */
    Facing facingFor(const InputIntentions& intentions, Facing current);

    /**
     * @brief Component storing runtime simulation flags and memory variables for platformer motion.
     */
    struct PlatformerMovement
    {
        PlatformerMovementConfig
            config; /**< The physics coefficients profile guiding this actor's motion rule logic. */
        bool grounded = false; /**< True if the actor's physics bounding box was firmly supported by
                                  a floor surface last update tick. */
        float coyoteRemaining = 0.0F; /**< Tracking clock timer in seconds monitoring the remaining
                                         coyote grace window. */
        float jumpBufferRemaining = 0.0F; /**< Tracking clock timer in seconds measuring the
                                             expiration threshold of buffered jump clicks. */
        bool blocked = false; /**< Flag indicating horizontal movement was actively arrested by a
                                 solid wall obstacle or ledge guard block. */
    };

    /**
     * @brief Simulates running, jumping, falling, and physical collision resolution against static
     * blocks.
     *
     * This core calculation routine handles running friction acceleration, processes gravity
     * transitions, evaluates buffered jump checks, applies AABB sweeping tests, and updates
     * grounding indicators.
     *
     * @param map The static environment geometry grid layout context used to compute solid blocks.
     * @param body The physics body wrapper component tracking global locations and velocity
     * vectors.
     * @param movement The runtime platformer configuration and memory state registers to update.
     * @param intentions Intended horizontal steering triggers and jump inputs captured this update
     * tick.
     * @param deltaTime The discrete frame simulation duration step length in seconds.
     * @return A CollisionContacts bitmask describing all physical surfaces contacted during
     * resolution steps.
     */
    CollisionContacts updatePlatformerMovement(
        const TileMap& map,
        Body& body,
        PlatformerMovement& movement,
        const InputIntentions& intentions,
        float deltaTime);
}
