/**
 * @file sprite.hpp
 * @brief Composes asset texture references, anchors, and bounding boxes for drawing routines.
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
    /**
     * @brief Defines a sub-rectangle region within a texture sheet.
     */
    struct SpriteRegion
    {
        glm::vec2 position = {0.0F, 0.0F}; /**< Source top-left coordinate measured in raw texture
                                              atlas pixels. The rendering pipeline dynamically maps
                                              this to normalized UV space coordinates. */
        glm::vec2 size = {
            0.0F,
            0.0F}; /**< Dimensions of the sub-rectangle measured in raw texture atlas pixels. */
    };

    /**
     * @brief Determines how a visual sprite maps its structural offset to a physical body boundary.
     */
    enum class SpriteAnchor
    {
        BodyFeet,  /**< Aligns the absolute bottom-center edge of the sprite asset directly with the
                      body's baseline feet coordinates. */
        BodyCenter /**< Aligns the absolute geometric midpoint center of the sprite asset directly
                      with the body's bounding box center. */
    };

    /**
     * @brief Composes an asset texture reference with placement anchors for drawing routines.
     *
     * @note The sprite region renders using 1:1 pixel scaling: one texture pixel matches one world
     * coordinate pixel. Modifying sprite assets or dimensions does not shift or scale the
     * underlying physical body collision matrix size.
     */
    struct Sprite
    {
        int textureId =
            0; /**< Resource index pointing to the cached GPU texture asset array wrapper. */
        SpriteRegion region; /**< Section layout area defining what pixels from the atlas sheet are
                                displayed. */
        SpriteAnchor anchor =
            SpriteAnchor::BodyFeet; /**< Positioning anchor logic determining how the image spans
                                       across the entity's physics box boundaries. */
    };

    /**
     * @brief Computes the absolute visual display bounding box boundaries for a sprite.
     *
     * Maps the sprite's dimensions in world pixels onto a physical entity's spatial axis-aligned
     * bounding box (AABB) using its configured anchor offset alignment style.
     *
     * @param bodyBounds The physical axis-aligned bounding box coordinates of the entity.
     * @param sprite The target sprite definition context carrying the texture sizing and anchoring
     * layout data.
     * @return Aabb The generated display box containing the visual boundaries of the asset overlay.
     */
    Aabb spriteBounds(const Aabb& bodyBounds, const Sprite& sprite);
}
