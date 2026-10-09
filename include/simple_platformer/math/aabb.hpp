#pragma once

#include <glm/vec2.hpp>

#include "simple_platformer/math/coordinates.hpp"

namespace simple_platformer
{
    // An axis-aligned bounding box (AABB) in world space.
    // The box is defined by its top-left corner and its size, so its
    // right and bottom edges are derived from those two values.
    struct Aabb
    {
        // Top-left corner of the box, in world pixels.
        // Defaults to the origin.
        glm::vec2 topLeft = {0.0F, 0.0F};

        // Width (x) and height (y) of the box, in world pixels.
        // Defaults to zero.
        glm::vec2 size = {0.0F, 0.0F};
    };

    // Returns the x coordinate of the box's right edge (the far vertical edge).
    // The left edge is topLeft.x.
    float rightOf(const Aabb& box);

    // Returns the y coordinate of the box's bottom edge (the far horizontal edge).
    // The top edge is topLeft.y.
    float bottomOf(const Aabb& box);

    // Returns the centre point of the box.
    glm::vec2 centerOf(const Aabb& box);

    // Returns the middle of the box's top edge.
    glm::vec2 topCenterOf(const Aabb& box);

    // Returns the box's feet: the middle of its bottom edge, where a standing
    // body meets the ground.
    glm::vec2 feetOf(const Aabb& box);

    // Creates a box of the given size whose centre is at the given point.
    Aabb boxCenteredOn(glm::vec2 center, glm::vec2 size);

    // Creates a box of the given size whose feet (bottom-centre) are at the given point.
    Aabb boxStandingOn(glm::vec2 feet, glm::vec2 size);

    // Moves the box in place so that its feet are at the given point.
    // The box's size is unchanged.
    void moveFeetTo(Aabb& box, glm::vec2 feet);

    // Creates a box of the given size standing inside the given cell.
    // Its feet are placed at the middle of the cell's bottom edge.
    // tileSize is the width and height of one cell in world pixels.
    Aabb boxInCell(int tileSize, Cell cell, glm::vec2 size);

    // Returns the range of cells that the box overlaps.
    // tileSize is the width and height of one cell in world pixels.
    // The box's edges are read EdgeTolerance pixels inside its bounds, so a box
    // resting exactly on a cell boundary does not also count the neighbouring
    // cell beyond that boundary.
    CellRange cellsCovered(int tileSize, const Aabb& box);

    // Returns true if the two boxes overlap.
    // Boxes that only touch along an edge are not considered overlapping.
    bool overlaps(const Aabb& first, const Aabb& second);

    // Returns true if the point lies inside the box.
    // The left and top edges belong to the box; the right and bottom edges do not.
    bool contains(const Aabb& box, glm::vec2 point);
}
