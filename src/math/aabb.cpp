/**
 * @file aabb.cpp
 * @brief Axis-aligned bounding box (AABB) definitions and spatial intersection utilities.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#include "simple_platformer/math/aabb.hpp"

#include <glm/vec2.hpp>

#include "simple_platformer/math/coordinates.hpp"

namespace simple_platformer
{
    // Returns the horizontal coordinate of the rightmost edge of the box
    float rightOf(const Aabb& box)
    {
        return box.topLeft.x + box.size.x;
    }

    // Returns the vertical coordinate of the lowest edge of the box
    float bottomOf(const Aabb& box)
    {
        return box.topLeft.y + box.size.y;
    }

    // Computes the geometric 2D center coordinate of the axis-aligned bounding box
    glm::vec2 centerOf(const Aabb& box)
    {
        return box.topLeft + box.size * 0.5F;
    }

    // Computes the horizontal center coordinate aligned along the top baseline edge
    glm::vec2 topCenterOf(const Aabb& box)
    {
        return {centerOf(box).x, box.topLeft.y};
    }

    // Computes the horizontal center coordinate aligned along the bottom ground contact edge
    glm::vec2 feetOf(const Aabb& box)
    {
        return {centerOf(box).x, bottomOf(box)};
    }

    // Factory method building an Aabb centered precisely over a specified point coordinate
    Aabb boxCenteredOn(glm::vec2 center, glm::vec2 size)
    {
        return {center - size * 0.5F, size};
    }

    // Factory method building an Aabb whose baseline center stands exactly on a specified footing
    // anchor point
    Aabb boxStandingOn(glm::vec2 feet, glm::vec2 size)
    {
        return {{feet.x - size.x * 0.5F, feet.y - size.y}, size};
    }

    // Relocates an existing bounding box layout, snapping its bottom-center feet to a new location
    // anchor
    void moveFeetTo(Aabb& box, glm::vec2 feet)
    {
        box = boxStandingOn(feet, box.size);
    }

    // Factory method creating a bounding box centered standing baseline inside a discrete grid cell
    // cell location
    Aabb boxInCell(int tileSize, Cell cell, glm::vec2 size)
    {
        return boxStandingOn(feetInCell(tileSize, cell), size);
    }

    // Computes the range span of integer tile grid coordinates overlapped by the bounding volume
    // box
    CellRange cellsCovered(int tileSize, const Aabb& box)
    {
        // Incorporate a miniature floating epsilon parameter (EdgeTolerance) to prevent fractional
        // rounding from grabbing unintended adjacent cells
        return {
            cellAt(tileSize, {box.topLeft.x + EdgeTolerance, box.topLeft.y + EdgeTolerance}),
            cellAt(tileSize, {rightOf(box) - EdgeTolerance, bottomOf(box) - EdgeTolerance})};
    }

    // Evaluates separating axis theorem rules to identify intersection collisions between two
    // distinct Aabb structures
    bool overlaps(const Aabb& first, const Aabb& second)
    {
        return first.topLeft.x < rightOf(second) && rightOf(first) > second.topLeft.x &&
               first.topLeft.y < bottomOf(second) && bottomOf(first) > second.topLeft.y;
    }

    // Verifies whether a target 2D point vector falls fully inside the boundary dimensions of an
    // Aabb structure
    bool contains(const Aabb& box, glm::vec2 point)
    {
        return point.x >= box.topLeft.x && point.y >= box.topLeft.y && point.x < rightOf(box) &&
               point.y < bottomOf(box);
    }
}
