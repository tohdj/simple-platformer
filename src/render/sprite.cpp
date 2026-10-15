/**
 * @file sprite.cpp
 * @brief Composes asset texture references, anchors, and bounding boxes for drawing routines.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */
#include "simple_platformer/render/sprite.hpp"

#include <stdexcept>

#include "simple_platformer/math/aabb.hpp"

namespace simple_platformer
{
    // Computes the absolute visual rendering boundaries for a sprite relative to its associated
    // physical rigid body
    Aabb spriteBounds(const Aabb& bodyBounds, const Sprite& sprite)
    {
        switch (sprite.anchor)
        {
        case SpriteAnchor::BodyFeet: {
            // Align the bottom-center of the visual sprite texture perfectly with the baseline feet
            // coordinates of the physics box
            const glm::vec2 bodyFeet = feetOf(bodyBounds);
            const glm::vec2 size = sprite.region.size;
            return {{bodyFeet.x - size.x * 0.5F, bodyFeet.y - size.y}, size};
        }
        case SpriteAnchor::BodyCenter:
            // Centrally align the geometric midpoint axes of both the visual sprite and physical
            // rigid body boundaries
            return boxCenteredOn(centerOf(bodyBounds), sprite.region.size);
        }

        throw std::invalid_argument("Sprite anchor is invalid");
    }
}
