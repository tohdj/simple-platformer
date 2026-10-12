/**
 * @file surface_climb.hpp
 * @brief Physics state machine and navigation logic for climbing vertical and horizontal surfaces.
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
    struct Aabb;
    struct Body;
    struct InputIntentions;
    struct PlatformerMovement;

    /**
     * @brief Identifies the type of surface an entity is currently attached to or scaling.
     */
    enum class ClimbSurface
    {
        None,      /**< Not currently clinging to any climbable surface. */
        LeftWall,  /**< Attached to a wall on the entity's left side. */
        RightWall, /**< Attached to a wall on the entity's right side. */
        Ceiling    /**< Attached to a ceiling directly overhead. */
    };

    /**
     * @brief Defines the orientation of a climber's head while navigating a vertical wall.
     */
    enum class WallHeading
    {
        Up,  /**< Pointing or moving upward. */
        Down /**< Pointing or moving downward. */
    };

    /**
     * @brief Tuning parameters that define global or archetype-specific climbing movement.
     */
    struct SurfaceClimbConfig
    {
        float speed = 60.0F; /**< Climbing speed expressed in world units (pixels) per second. */
    };

    /**
     * @brief Runtime state tracking an actor's optional capability to scale surfaces.
     *
     * The `surface` variable handles active physical contact, while the player's
     * input intentions (`climbGrip`) decide whether to grab, stay attached, or drop off.
     */
    struct SurfaceClimb
    {
        SurfaceClimbConfig config;                 /**< Configured movement limits and speeds. */
        ClimbSurface surface = ClimbSurface::None; /**< Current surface the body is attached to. */
        WallHeading wallHeading =
            WallHeading::Up; /**< Active heading maintained while stationary on a wall to prevent
                                flickering orientation. */
    };

    /**
     * @brief Validates config fields to ensure values are safe and positive.
     * @throws std::invalid_argument If configurations like negative speed are detected.
     */
    void validateSurfaceClimbConfig(const SurfaceClimbConfig& config);

    /**
     * @brief Resolves the target heading direction based on current user inputs and state.
     *
     * Matches the direction of input intentions when moving. If inputs are idle,
     * it falls back to the current heading. Off-wall states default to Up until the next contact.
     *
     * @param surface The active climbing surface context.
     * @param intentions Current user input tracking direction and grip states.
     * @param current The existing heading state before evaluation.
     */
    WallHeading wallHeadingFor(
        ClimbSurface surface,
        const InputIntentions& intentions,
        WallHeading current);

    /**
     * @brief Checks if an actor's axis-aligned bounding box (AABB) overlaps a climbable tile asset.
     * @note The floor is handled separately by ground movement states and is not a valid climb
     * surface.
     *
     * @param map The active level structure layout.
     * @param bounds The physical outer dimensions of the entity.
     * @param surface The specific boundary side to evaluate (Left, Right, or Ceiling).
     */
    bool touchesClimbable(const TileMap& map, const Aabb& bounds, ClimbSurface surface);

    /**
     * @brief Advances the climbing physics state machine by evaluating input, movement modifiers,
     * and delta time.
     *
     * Handles friction, attachment updates, velocity modifications, and resolves spatial
     * collisions against the tile layout map.
     *
     * @param map Active level layout metadata.
     * @param body Raw physics representation containing positioning and velocities.
     * @param movement Shared platformer kinematics fields (e.g., gravity scales, flags).
     * @param climb Active runtime climbing persistence structure.
     * @param intentions Inputs driving actor movement.
     * @param deltaTime Elapsed frame time segment.
     * @return CollisionContacts Struct recording all physical points intercepted during processing.
     */
    CollisionContacts updateSurfaceClimbMovement(
        const TileMap& map,
        Body& body,
        PlatformerMovement& movement,
        SurfaceClimb& climb,
        const InputIntentions& intentions,
        float deltaTime);
}
