/**
 * @file body.hpp
 * @brief Rigid body components, gravity simulation, and axis-aligned collision sweeps against map
 * geometry.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#pragma once

#include <glm/vec2.hpp>

#include "simple_platformer/math/aabb.hpp"

namespace simple_platformer
{
    class TileMap;

    // A foundational physical object representation tracking spatial boundaries and translation
    // velocities.
    struct Body
    {
        // Collision rectangle measured in world pixels. Its structural size is completely
        // independent of visual sprites.
        Aabb bounds;

        // Instantaneous movement vector expressed in world pixels per second.
        glm::vec2 velocity = {0.0F, 0.0F};
    };

    // Global baseline downward gravitational acceleration scale applied to unanchored bodies.
    constexpr float DefaultGravity = 800.0F;

    // Global standard downward terminal velocity cap restricting free-falling objects.
    constexpr float DefaultMaximumFallSpeed = 600.0F;

    // Integrates gravitational forces onto a body's vertical velocity component over a time step.
    void applyGravity(Body& body, float gravity, float maximumFallSpeed, float deltaTime);

    // Identifies the specific directional planes where a body is making physical contact.
    struct CollisionContacts
    {
        bool left = false;  // True if the left boundary overlaps a solid block or map perimeter.
        bool right = false; // True if the right boundary overlaps a solid block or map perimeter.
        bool ground =
            false; // True if the bottom baseline is supported by a stable horizontal surface.
        bool ceiling =
            false; // True if the top boundary intercepts a blocking overhead ceiling block.
    };

    // Translates a body through space, resolving axis-aligned collisions using AABB sweeping steps.
    // Integrates positions step-by-step: checking and resolving horizontal X movement rules before
    // applying vertical Y movement slices. Obstructing tiles halt travel along the intercepted axis
    // and instantly nullify its respective velocity component.
    CollisionContacts moveBody(const TileMap& map, Body& body, float deltaTime);

    // Probes for geometric surface contacts surrounding a stationary box without mutating its
    // position. Incorporates a miniature floating-point epsilon tolerance margin to account for
    // rounding artifacts.
    CollisionContacts touchingSurfaces(const TileMap& map, const Aabb& bounds);

    // Inspects immediate contact perimeters exclusively for tiles flagged with active climbing
    // properties.
    CollisionContacts touchingClimbableSurfaces(const TileMap& map, const Aabb& bounds);
}
