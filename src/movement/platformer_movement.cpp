/**
 * @file platformer_movement.cpp
 * @brief Kinematic movement parameters, configuration validation, and integration routines for
 * platformer physics.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#include "simple_platformer/movement/platformer_movement.hpp"

#include <algorithm>
#include <stdexcept>

#include <glm/vec2.hpp>

#include "simple_platformer/input/input_state.hpp"
#include "simple_platformer/math/aabb.hpp"
#include "simple_platformer/math/coordinates.hpp"
#include "simple_platformer/math/validation.hpp"
#include "simple_platformer/physics/body.hpp"
#include "simple_platformer/world/tile_map.hpp"

namespace simple_platformer
{
    namespace
    {
        // Linearly steps a floating-point value towards a target point up to a maximum adjustment
        // delta cap
        float moveTowards(float current, float target, float maximumChange)
        {
            if (current < target)
            {
                return std::min(current + maximumChange, target);
            }

            return std::max(current - maximumChange, target);
        }

        // Asserts data validity across movement configuration parameters and incoming control
        // vector arrays before calculations run
        void validate(
            const PlatformerMovementConfig& config,
            const InputIntentions& intentions,
            float deltaTime)
        {
            requireSeconds(deltaTime, "Platformer movement time step");
            validatePlatformerMovementConfig(config);

            requireFinite(intentions.direction, "Input intentions");
        }

        // Ticks down frame timing trackers for jump input buffering and coyote landing windows
        void updateTimers(
            PlatformerMovement& movement,
            const InputIntentions& intentions,
            float deltaTime)
        {
            if (intentions.jumpPressed)
            {
                movement.jumpBufferRemaining =
                    movement.config
                        .jumpBufferDuration; // Refresh input buffer tracking window on tap edge
            }
            else
            {
                movement.jumpBufferRemaining =
                    std::max(0.0F, movement.jumpBufferRemaining - deltaTime);
            }

            if (movement.grounded)
            {
                movement.coyoteRemaining =
                    movement.config
                        .coyoteDuration; // Refresh grace window continuously while grounded
            }
            else
            {
                movement.coyoteRemaining = std::max(0.0F, movement.coyoteRemaining - deltaTime);
            }
        }

        // Computes horizontal velocity changes using distinct acceleration and braking scales for
        // air or land tracking
        void updateHorizontalVelocity(
            Body& body,
            const PlatformerMovement& movement,
            const InputIntentions& intentions,
            float deltaTime)
        {
            const float direction = std::clamp(intentions.direction.x, -1.0F, 1.0F);

            // Allow airborne momentum to coast naturally by dropping updates if horizontal stick
            // inputs are blank
            if (direction == 0.0F && !movement.grounded)
            {
                return;
            }

            const float target = direction * movement.config.maximumSpeed;
            const float acceleration = direction == 0.0F   ? movement.config.groundDeceleration
                                       : movement.grounded ? movement.config.groundAcceleration
                                                           : movement.config.airAcceleration;

            body.velocity.x = moveTowards(body.velocity.x, target, acceleration * deltaTime);
        }

        // Evaluates buffered jump commands and coyote grace windows to launch an actor upward into
        // an airborne state
        void startBufferedJump(Body& body, PlatformerMovement& movement, bool jumpPressed)
        {
            const bool wantsToJump = jumpPressed || movement.jumpBufferRemaining > 0.0F;
            const bool canJump = movement.grounded || movement.coyoteRemaining > 0.0F;
            if (!wantsToJump || !canJump)
            {
                return;
            }

            // Apply direct vertical thrust impulse vector, then flush timing buffers instantly to
            // prevent double-jumps
            body.velocity.y = -movement.config.jumpSpeed;
            movement.grounded = false;
            movement.coyoteRemaining = 0.0F;
            movement.jumpBufferRemaining = 0.0F;
        }

        // Probe every floor tile the leading edge would cross this step, so even a
        // fast walk cannot skip a narrow gap. This is an opt-in walking constraint.
        bool wouldWalkOffLedge(const TileMap& map, const Body& body, float deltaTime)
        {
            if (body.velocity.x == 0.0F || deltaTime == 0.0F)
            {
                return false;
            }
            const int side = body.velocity.x < 0.0F ? -1 : 1;
            const float front = body.bounds.topLeft.x + (side > 0 ? body.bounds.size.x : 0.0F);
            const float probeX = front + static_cast<float>(side);
            const float belowFeet = feetOf(body.bounds).y + 1.0F;

            // Sweep forward position parameters down into cell metrics to inspect for sudden drops
            // or pits
            const Cell first = cellAt(map.tileSize(), {probeX, belowFeet});
            const int last =
                cellAt(map.tileSize(), {probeX + body.velocity.x * deltaTime, belowFeet}).x;

            for (int x = first.x; side > 0 ? x <= last : x >= last; x += side)
            {
                if (!map.blocksMovement({x, first.y}))
                {
                    return true; // Encountered a pass-through/air cell: a ledge drop is detected
                                 // ahead
                }
            }
            return false;
        }

        // Halts horizontal velocity immediately if a ledge drop is detected and cliff-avoidance
        // properties are active
        bool stopAtLedge(
            const TileMap& map,
            Body& body,
            const PlatformerMovement& movement,
            const InputIntentions& intentions,
            float deltaTime)
        {
            if (!intentions.avoidLedges || !movement.grounded ||
                !wouldWalkOffLedge(map, body, deltaTime))
            {
                return false;
            }

            body.velocity.x = 0.0F; // Brake locomotion instantly at the precipice edge line
            return true;
        }

        // Letting go of jump while rising pulls the actor down harder, for a short hop.
        void applyJumpAwareGravity(
            Body& body,
            const PlatformerMovement& movement,
            const InputIntentions& intentions,
            float deltaTime)
        {
            // Detect if player released the jump button early during an upward vertical trajectory
            // step
            const bool cuttingJump = body.velocity.y < 0.0F && !intentions.jumpHeld;

            // Scale gravity calculation forces upward to pull the character down early for short
            // hops
            const float gravity =
                cuttingJump ? movement.config.jumpReleaseGravity : movement.config.gravity;

            applyGravity(body, gravity, movement.config.maximumFallSpeed, deltaTime);
        }
    }

    // Compares two movement profiles field-by-field to check for absolute value equality across
    // physics scalars
    bool operator==(const PlatformerMovementConfig& left, const PlatformerMovementConfig& right)
    {
        return left.maximumSpeed == right.maximumSpeed &&
               left.groundAcceleration == right.groundAcceleration &&
               left.airAcceleration == right.airAcceleration &&
               left.groundDeceleration == right.groundDeceleration &&
               left.jumpSpeed == right.jumpSpeed && left.gravity == right.gravity &&
               left.jumpReleaseGravity == right.jumpReleaseGravity &&
               left.maximumFallSpeed == right.maximumFallSpeed &&
               left.coyoteDuration == right.coyoteDuration &&
               left.jumpBufferDuration == right.jumpBufferDuration;
    }

    // Evaluates value inequality between two configurations by negating the equality check result
    bool operator!=(const PlatformerMovementConfig& left, const PlatformerMovementConfig& right)
    {
        return !(left == right);
    }

    // Asserves that all structural kinematics variables are finite, real, and non-negative values
    void validatePlatformerMovementConfig(const PlatformerMovementConfig& config)
    {
        const bool invalidConfig = !isFiniteNonNegative(config.maximumSpeed) ||
                                   !isFiniteNonNegative(config.groundAcceleration) ||
                                   !isFiniteNonNegative(config.airAcceleration) ||
                                   !isFiniteNonNegative(config.groundDeceleration) ||
                                   !isFiniteNonNegative(config.jumpSpeed) ||
                                   !isFiniteNonNegative(config.gravity) ||
                                   !isFiniteNonNegative(config.jumpReleaseGravity) ||
                                   !isFiniteNonNegative(config.maximumFallSpeed) ||
                                   !isFiniteNonNegative(config.coyoteDuration) ||
                                   !isFiniteNonNegative(config.jumpBufferDuration);
        if (invalidConfig)
        {
            throw std::invalid_argument("Platformer movement configuration cannot be negative");
        }
    }

    // Determines character facing vectors, letting projectile weapon aim direction override
    // locomotion steering
    Facing facingFor(const InputIntentions& intentions, Facing current)
    {
        const float decisive =
            intentions.aimDirection.x != 0.0F ? intentions.aimDirection.x : intentions.direction.x;

        if (decisive < 0.0F)
        {
            return Facing::Left;
        }
        return decisive > 0.0F
                   ? Facing::Right
                   : current; // Retain current orientation state if inputs are completely neutral
    }

    // Main execution update loop integrating horizontal velocities, handling jumps, scaling
    // gravity, and resolving collision masks
    CollisionContacts updatePlatformerMovement(
        const TileMap& map,
        Body& body,
        PlatformerMovement& movement,
        const InputIntentions& intentions,
        float deltaTime)
    {
        // 1. Core verification pass and countdown timer tick downs
        validate(movement.config, intentions, deltaTime);
        updateTimers(movement, intentions, deltaTime);

        // 2. Compute horizontal force updates and evaluate jump trigger buffer matrices
        updateHorizontalVelocity(body, movement, intentions, deltaTime);
        startBufferedJump(body, movement, intentions.jumpPressed);

        // 3. Process jump-cut checks and apply adaptive down-force calculations
        applyJumpAwareGravity(body, movement, intentions, deltaTime);

        // 4. Evaluate edge precipice constraints before committing translation coordinate updates
        const bool stoppedAtLedge = stopAtLedge(map, body, movement, intentions, deltaTime);

        // 5. Run the axis-splitting AABB sweeper function to shift positions and capture surface
        // contact markers
        const CollisionContacts contacts = moveBody(map, body, deltaTime);

        // 6. Bind resulting structural environment locks into state memory storage blocks
        movement.blocked = stoppedAtLedge || contacts.left || contacts.right;
        movement.grounded = contacts.ground;

        return contacts;
    }

}
