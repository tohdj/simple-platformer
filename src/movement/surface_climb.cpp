/**
 * @file surface_climb.cpp
 * @brief Physics state machine and navigation logic for climbing vertical and horizontal surfaces.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#include "simple_platformer/movement/surface_climb.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "simple_platformer/input/input_state.hpp"
#include "simple_platformer/math/validation.hpp"
#include "simple_platformer/movement/platformer_movement.hpp"
#include "simple_platformer/physics/body.hpp"
#include "simple_platformer/world/tile_map.hpp"

namespace simple_platformer
{
    namespace
    {
        // Helper to determine if a contact flag matches the specified climbing surface type
        bool touchesSurface(ClimbSurface surface, const CollisionContacts& contacts)
        {
            switch (surface)
            {
            case ClimbSurface::LeftWall:
                return contacts.left;
            case ClimbSurface::RightWall:
                return contacts.right;
            case ClimbSurface::Ceiling:
                return contacts.ceiling;
            case ClimbSurface::None:
                return false;
            }
            return false;
        }

        // Evaluates input directions and physical collision points to determine the next attachment
        // state
        ClimbSurface requestedSurface(
            ClimbSurface current,
            const CollisionContacts& contacts,
            const InputIntentions& intentions)
        {
            // Evaluate if the actor actively intends to hold onto a surface this frame
            const bool holding =
                intentions.climbGrip == ClimbGrip::Hold ||
                (intentions.climbGrip == ClimbGrip::Keep && current != ClimbSurface::None);
            if (!holding)
            {
                return ClimbSurface::None; // Release grip immediately if holding inputs are absent
            }

            // At a corner, the intended travel axis chooses the next surface. Without
            // a stronger direction, keep the current grip if it still has contact.
            const float horizontal = std::abs(intentions.direction.x);
            const float vertical = std::abs(intentions.direction.y);

            // Prioritise ceiling attachment when horizontal movement commands dominate at a
            // boundary corner
            if (contacts.ceiling && horizontal > vertical)
            {
                return ClimbSurface::Ceiling;
            }

            // Prioritise wall updates when vertical intent is stronger than horizontal inputs
            if (vertical > horizontal)
            {
                if (contacts.left)
                {
                    return ClimbSurface::LeftWall;
                }
                if (contacts.right)
                {
                    return ClimbSurface::RightWall;
                }
            }

            // Maintain the current surface state if the actor is still in physical contact with it
            if (touchesSurface(current, contacts))
            {
                return current;
            }

            // Fall back to a default surface discovery check based on standard attachment priority
            // orders
            if (contacts.left)
            {
                return ClimbSurface::LeftWall;
            }
            if (contacts.right)
            {
                return ClimbSurface::RightWall;
            }
            return contacts.ceiling ? ClimbSurface::Ceiling : ClimbSurface::None;
        }
    }

    // Evaluates if the actor's bounding box is actively touching a climbable tile asset on the
    // specified surface side
    bool touchesClimbable(const TileMap& map, const Aabb& bounds, ClimbSurface surface)
    {
        return touchesSurface(surface, touchingClimbableSurfaces(map, bounds));
    }

    // Assures climbing configuration variables match valid non-negative physical thresholds
    void validateSurfaceClimbConfig(const SurfaceClimbConfig& config)
    {
        if (!isFinitePositive(config.speed))
        {
            throw std::invalid_argument("Surface climb speed must be finite and positive");
        }
    }

    // Dictates vertical layout drawing/heading priorities based on movement velocity and surface
    // attachment
    WallHeading wallHeadingFor(
        ClimbSurface surface,
        const InputIntentions& intentions,
        WallHeading current)
    {
        // Reset heading state to default if the actor isn't currently clinging to a wall
        if (surface != ClimbSurface::LeftWall && surface != ClimbSurface::RightWall)
        {
            return WallHeading::Up;
        }
        // Negative Y inputs point upward in this layout coordinate system
        if (intentions.direction.y < 0.0F)
        {
            return WallHeading::Up;
        }
        // Fall back to the existing heading if velocity inputs are zero to avoid flickering
        return intentions.direction.y > 0.0F ? WallHeading::Down : current;
    }

    // Advances the climbing engine state loop by applying velocity mappings and physics constraints
    CollisionContacts updateSurfaceClimbMovement(
        const TileMap& map,
        Body& body,
        PlatformerMovement& movement,
        SurfaceClimb& climb,
        const InputIntentions& intentions,
        float deltaTime)
    {
        // Enforce physical constraints and data safety assertions
        requireSeconds(deltaTime, "Surface climb time step");
        validateSurfaceClimbConfig(climb.config);
        validatePlatformerMovementConfig(movement.config);
        requireFinite(intentions.direction, "Input intentions");

        // Step 1: Query adjacent grid geometry and resolve the active attachment surface choice
        const ClimbSurface previous = climb.surface;
        climb.surface =
            requestedSurface(previous, touchingClimbableSurfaces(map, body.bounds), intentions);
        climb.wallHeading = wallHeadingFor(climb.surface, intentions, climb.wallHeading);

        // Step 2: Fall back to normal physics simulation loop if the actor is not climbing
        if (climb.surface == ClimbSurface::None)
        {
            // Nullify momentum transiently on the transition frame when slipping off a surface
            if (previous != ClimbSurface::None)
            {
                body.velocity = {0.0F, 0.0F};
            }
            return updatePlatformerMovement(map, body, movement, intentions, deltaTime);
        }

        // Step 3: Zero out base forces and map inputs to the climbing surface axes
        body.velocity = {0.0F, 0.0F}; // Strips standard platformer gravity integrations
        if (climb.surface == ClimbSurface::Ceiling)
        {
            body.velocity.x = std::clamp(intentions.direction.x, -1.0F, 1.0F) * climb.config.speed;
        }
        else
        {
            body.velocity.y = std::clamp(intentions.direction.y, -1.0F, 1.0F) * climb.config.speed;
        }

        // Suppress jump buffering windows while attached to walls or ceilings
        movement.coyoteRemaining = 0.0F;
        movement.jumpBufferRemaining = 0.0F;

        // Step 4: Execute spatial simulation translation steps and update context state registers
        const CollisionContacts contacts = moveBody(map, body, deltaTime);
        const CollisionContacts surfaces = touchingSurfaces(map, body.bounds);
        movement.grounded = surfaces.ground;
        movement.blocked = contacts.left || contacts.right || contacts.ground || contacts.ceiling;

        // Step 5: Force immediate detachment if the movement took the actor off climbable surface
        // ranges
        if (!touchesClimbable(map, body.bounds, climb.surface))
        {
            climb.surface = ClimbSurface::None;
            climb.wallHeading = WallHeading::Up;
            body.velocity = {0.0F, 0.0F};
        }

        return contacts;
    }
}
