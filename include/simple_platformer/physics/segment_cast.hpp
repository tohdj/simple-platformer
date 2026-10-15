/**
 * @file segment_cast.hpp
 * @brief Linear ray/box sweeping routines and intersection tests against environment tiles and
 * physics boundaries.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#pragma once

#include <optional>

#include <glm/vec2.hpp>

#include "simple_platformer/math/aabb.hpp"
#include "simple_platformer/math/coordinates.hpp"

namespace simple_platformer
{
    class TileMap;

    // Evaluates line segment intersection against an axis-aligned bounding box.
    // Returns a normalized fraction between 0.0 (start) and 1.0 (end) representing the impact
    // point, or std::nullopt.
    std::optional<float> segmentCast(const Aabb& box, glm::vec2 start, glm::vec2 end);

    // Expands a target box by half the dimensions of a moving box on all sides (Minkowski Sum).
    // Casting a zero-width line from the moving box's center against this perimeter finds the exact
    // collision point.
    Aabb expandedForMovingBox(const Aabb& target, glm::vec2 movingSize);

    // Payload tracking information about a successful tile segment interception.
    struct TileSegmentHit
    {
        float segmentTime =
            0.0F;  // Normalized interpolation index along the ray segment (0.0 to 1.0).
        Cell cell; // The grid coordinate coordinates of the intercepted tile.
    };

    // Computes the earliest intersection point against movement-obstructing tiles along a path
    // vector. Sweeps a moving bounding box from a starting center towards a destination. Map
    // boundaries are treated as solid limits.
    std::optional<TileSegmentHit> segmentCastMovementBlockingTiles(
        const TileMap& map,
        glm::vec2 start,
        glm::vec2 end,
        glm::vec2 movingSize = {0.0F, 0.0F});

    // Computes the earliest intersection point against sight-obstructing tiles along a ray line.
    // Ignores consecutive sight-blocking cells surrounding the starting position so entities can
    // look out from inside cover.
    std::optional<float> segmentCastSightBlockingTiles(
        const TileMap& map,
        glm::vec2 start,
        glm::vec2 end);
}
