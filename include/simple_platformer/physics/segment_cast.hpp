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

    /**
     * @brief Evaluates line segment intersection against an axis-aligned bounding box.
     *
     * @param box The target bounding box to cast against.
     * @param start Origin point of the ray trace segment.
     * @param end Termination point of the ray trace segment.
     * @return std::optional<float> A normalized fraction between 0.0 (start) and 1.0 (end)
     * representing the impact point, or std::nullopt if no intersection occurs.
     */
    std::optional<float> segmentCast(const Aabb& box, glm::vec2 start, glm::vec2 end);

    /**
     * @brief Expands a target box by half the dimensions of a moving box on all sides (Minkowski
     * Sum).
     *
     * By performing this expansion on the target, casting a zero-width line from the moving box's
     * center point against this inflated perimeter yields the exact point where the two boxes would
     * first collide.
     *
     * @param target The original stationary boundary box to inflate.
     * @param movingSize Width and height dimensions of the active moving bounding footprint.
     * @return Aabb The expanded bounding box.
     */
    Aabb expandedForMovingBox(const Aabb& target, glm::vec2 movingSize);

    /**
     * @brief Payload tracking information about a successful tile segment interception.
     */
    struct TileSegmentHit
    {
        float segmentTime =
            0.0F;  /**< Normalized interpolation index along the ray segment (0.0 to 1.0). */
        Cell cell; /**< The grid coordinate coordinates of the intercepted tile. */
    };

    /**
     * @brief Computes the earliest intersection point against movement-obstructing tiles along a
     * path vector.
     *
     * Sweeps a moving bounding box from a starting center layout towards a target destination.
     * Map boundaries are inherently treated as solid limits. Passing a zero size reduces the sweep
     * calculation down to a standard line-based raycast.
     *
     * @param map The static environment geometry grid layout context.
     * @param start The initial center position of the moving box.
     * @param end The target destination center position of the moving box.
     * @param movingSize Bounding width and height scales of the traveling physics object.
     * @return std::optional<TileSegmentHit> Hit details if an obstacle is struck, or std::nullopt
     * if the trajectory is completely clear.
     */
    std::optional<TileSegmentHit> segmentCastMovementBlockingTiles(
        const TileMap& map,
        glm::vec2 start,
        glm::vec2 end,
        glm::vec2 movingSize = {0.0F, 0.0F});

    /**
     * @brief Computes the earliest intersection point against sight-obstructing tiles along a ray
     * line.
     *
     * This routine deliberately ignores any consecutive, unbroken sequence of sight-blocking cells
     * enveloping the ray's starting position. This enables entities standing inside cover elements
     * (e.g. foliage) to see outward unobstructed, while distinct cover blocks encountered further
     * along the path still disrupt sight.
     *
     * @param map The static environment geometry grid layout context.
     * @param start Origin point of the visibility line trace.
     * @param end Termination point of the visibility line trace.
     * @return std::optional<float> A normalized fraction between 0.0 and 1.0 indicating where
     * vision is lost, or std::nullopt if sight is unimpeded.
     */
    std::optional<float> segmentCastSightBlockingTiles(
        const TileMap& map,
        glm::vec2 start,
        glm::vec2 end);
}
