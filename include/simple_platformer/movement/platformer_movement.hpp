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
        /// Maximum horizontal velocity limit in world pixels per second.
        float maximumSpeed = 100.0F;
        /// Rate of speed gain applied horizontally while standing on solid floor surfaces.
        float groundAcceleration = 800.0F;
        /// Rate of speed gain applied horizontally while airborne.
        float airAcceleration = 400.0F;
        /// Braking force scale applied horizontally when no horizontal input is active on the
        /// ground.
        float groundDeceleration = 1000.0F;
        /// Initial upward vertical velocity applied instantly at the moment of jumping.
        float jumpSpeed = 240.0F;
        /// Downward acceleration scale applied under standard falling or rising circumstances.
        float gravity = DefaultGravity;
        /// Increased gravity scale applied if the player releases the jump button early (allows
        /// variable jump control).
        float jumpReleaseGravity = 2.0F * DefaultGravity;
        /// Maximum downward terminal velocity cap to prevent excessive falling speed.
        float maximumFallSpeed = DefaultMaximumFallSpeed;
        /// Grace window duration in seconds allowing jumps shortly after running off an edge.
        float coyoteDuration = 0.1F;
        /// Time window duration in seconds to cache a jump command pressed slightly before touching
        /// down.
        float jumpBufferDuration = 0.1F;
    };

    /**
     * @brief Asserts and verifies that movement parameters fall within mathematically logical
     * limits.
     * @param config The immutable configuration properties block to inspect.
     */
    void validatePlatformerMovementConfig(const PlatformerMovementConfig& config);
    bool operator==(const PlatformerMovementConfig& left, const PlatformerMovementConfig& right);
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
        /// The physics coefficients profile guiding this actor's motion rule logic.
        PlatformerMovementConfig config;
        /// True if the actor's physics bounding box was firmly supported by a floor surface last
        /// update tick.
        bool grounded = false;
        /// Tracking clock timer in seconds monitoring the remaining coyote grace window.
        float coyoteRemaining = 0.0F;
        /// Tracking clock timer in seconds measuring the expiration threshold of buffered jump
        /// clicks.
        float jumpBufferRemaining = 0.0F;
        /// Flag indicating horizontal movement was actively arrested by a solid wall obstacle or
        /// ledge guard block.
        bool blocked = false;
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
