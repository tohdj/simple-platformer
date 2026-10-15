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

    // Identifies the type of surface an entity is currently attached to or scaling
    enum class ClimbSurface
    {
        None,      // Not currently clinging to any climbable surface
        LeftWall,  // Attached to a wall on the entity's left side
        RightWall, // Attached to a wall on the entity's right side
        Ceiling    // Attached to a ceiling directly overhead
    };

    // Defines the orientation of a climber's head while navigating a vertical wall
    enum class WallHeading
    {
        Up,  // Pointing or moving upward
        Down // Pointing or moving downward
    };

    // Tuning parameters that define global or archetype-specific climbing movement
    struct SurfaceClimbConfig
    {
        float speed = 60.0F; // Climbing speed expressed in world units (pixels) per second
    };

    // Runtime state tracking an actor's optional capability to scale surfaces
    // Note: The surface field tracks physical contact, while climbGrip inputs manage attachment
    // choices
    struct SurfaceClimb
    {
        SurfaceClimbConfig config;                 // Configured movement limits and speeds
        ClimbSurface surface = ClimbSurface::None; // Current surface the body is attached to
        // Active heading maintained while stationary on a wall to prevent flickering orientation
        WallHeading wallHeading = WallHeading::Up;
    };

    // Validates config fields to ensure values are safe and positive (throws on invalid parameters)
    void validateSurfaceClimbConfig(const SurfaceClimbConfig& config);

    // Resolves the target heading direction based on current user inputs, fallback heading, and
    // surface state
    WallHeading wallHeadingFor(
        ClimbSurface surface,
        const InputIntentions& intentions,
        WallHeading current);

    // Checks if an actor's bounding box overlaps a climbable tile (excluding floor surfaces)
    bool touchesClimbable(const TileMap& map, const Aabb& bounds, ClimbSurface surface);

    // Advances the climbing physics state machine by evaluating inputs, attachment logic, and
    // velocities
    CollisionContacts updateSurfaceClimbMovement(
        const TileMap& map,
        Body& body,
        PlatformerMovement& movement,
        SurfaceClimb& climb,
        const InputIntentions& intentions,
        float deltaTime);
}
