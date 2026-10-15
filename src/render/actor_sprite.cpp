/**
 * @file actor_sprite.cpp
 * @brief Calculations and structures for anchoring, rotating, and mirroring actor sprites against
 * surfaces.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#include "simple_platformer/render/actor_sprite.hpp"

#include <stdexcept>

#include <glm/gtc/constants.hpp>
#include <glm/vec2.hpp>

#include "simple_platformer/actor/actor.hpp"
#include "simple_platformer/math/aabb.hpp"
#include "simple_platformer/movement/platformer_movement.hpp"
#include "simple_platformer/movement/surface_climb.hpp"
#include "simple_platformer/render/sprite.hpp"

namespace simple_platformer
{
    namespace
    {
        // Transforms visual orientations, processing axis dimensional swaps for rotating actors
        ActorSpritePlacement turned(
            glm::vec2 visibleCentre,
            glm::vec2 size,
            float rotationRadians,
            bool flipHorizontal,
            bool quarterTurn)
        {
            // Transpose structural dimensions if the entity undergoes a 90-degree side
            // transformation rotation
            const glm::vec2 visibleSize = quarterTurn ? glm::vec2{size.y, size.x} : size;

            return {
                boxCenteredOn(
                    visibleCentre, size), // Standard canonical unrotated bounding boundaries
                rotationRadians,          // Global canvas texture rotation index angle tracking
                flipHorizontal, // Horizontal mirror flag state for changing face direction
                boxCenteredOn(visibleCentre, visibleSize) // Recalculated final footprint box
                                                          // matching orientation aspect ratios
            };
        }
    }

    // Computes layout placements, translation matrices, and spatial alignment parameters for an
    // actor's drawing mesh
    ActorSpritePlacement placeActorSprite(const Actor& actor)
    {
        if (!actor.sprite.has_value())
        {
            throw std::invalid_argument("Placing an actor's sprite requires a sprite");
        }
        const Sprite& sprite = *actor.sprite;
        const Aabb& body = actor.body.bounds;

        // Extract attachment mechanics data blocks from the structural actor entity
        const ClimbSurface surface =
            actor.surfaceClimb.has_value() ? actor.surfaceClimb->surface : ClimbSurface::None;
        const WallHeading wallHeading =
            actor.surfaceClimb.has_value() ? actor.surfaceClimb->wallHeading : WallHeading::Up;
        const bool facingLeft = actor.facing == Facing::Left;

        // 1. Fallback Pass: Return canonical ground configurations if the actor is disconnected
        // from walls
        if (surface == ClimbSurface::None)
        {
            const Aabb bounds = spriteBounds(body, sprite);
            return {bounds, 0.0F, facingLeft, bounds};
        }

        const bool headingUp = wallHeading == WallHeading::Up;
        const bool fromFeet = sprite.anchor == SpriteAnchor::BodyFeet;
        const glm::vec2 centre = centerOf(body);

        // Half the sprite's height: how far its centre sits from the edge its feet are on.
        const float feetToCentre = sprite.region.size.y * 0.5F;

        // Unmirrored art heads right; a turn carries the head with it. A quarter turn
        // clockwise heads it down, one anticlockwise up, and a half turn left.
        switch (surface)
        {
        case ClimbSurface::Ceiling:
            // 2. Ceiling Placement: Flip upside down (180 degrees / Pi radians) and invert
            // alignment offsets
            return turned(
                fromFeet ? glm::vec2{centre.x, body.topLeft.y + feetToCentre} : centre,
                sprite.region.size,
                glm::pi<float>(),
                !facingLeft,
                false);

        case ClimbSurface::LeftWall:
            // 3. Left Wall Placement: Rotate 90 degrees clockwise (Half Pi) and offset horizontally
            // from the left edge
            return turned(
                fromFeet ? glm::vec2{body.topLeft.x + feetToCentre, centre.y} : centre,
                sprite.region.size,
                glm::half_pi<float>(),
                headingUp,
                true);

        case ClimbSurface::RightWall:
            // 4. Right Wall Placement: Rotate 90 degrees counter-clockwise (-Half Pi) and offset
            // from the right boundary edge
            return turned(
                fromFeet ? glm::vec2{rightOf(body) - feetToCentre, centre.y} : centre,
                sprite.region.size,
                -glm::half_pi<float>(),
                !headingUp,
                true);

        case ClimbSurface::None:
            break;
        }
        throw std::invalid_argument("Climb surface is invalid");
    }

}
