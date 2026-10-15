/**
 * @file route_occupancy.cpp
 * @brief Spatial occupancy checks and geometric bounds alignment routines for pathfinding nodes.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#include "simple_platformer/navigation/platformer_cells.hpp"

#include <stdexcept>

#include <glm/vec2.hpp>

#include "simple_platformer/math/aabb.hpp"
#include "simple_platformer/math/coordinates.hpp"
#include "simple_platformer/math/validation.hpp"
#include "simple_platformer/movement/surface_climb.hpp"
#include "simple_platformer/navigation/route.hpp"
#include "simple_platformer/physics/body.hpp"
#include "simple_platformer/world/tile_map.hpp"

namespace simple_platformer
{
    namespace
    {
        // Clear of movement-blocking tiles and inside the map's blocking sides and
        // bottom. The map is open above its top edge, as it is for collision.
        bool bodyFits(const TileMap& map, const Aabb& bounds)
        {
            // First pass: verify the physical shape stays completely within the horizontal map
            // walls and bottom floor line
            if (bounds.topLeft.x < 0.0F || rightOf(bounds) > map.pixelWidth() ||
                bottomOf(bounds) > map.pixelHeight())
            {
                return false;
            }

            // Map the float pixel dimensions down to inclusive integer grid cell range blocks
            const CellRange cells = cellsCovered(map.tileSize(), bounds);

            // Second pass: loop through all covered grid tiles to ensure no solid geometry
            // intersects the body space
            for (int row = cells.first.y; row <= cells.last.y; ++row)
            {
                for (int column = cells.first.x; column <= cells.last.x; ++column)
                {
                    if (map.blocksMovement({column, row}))
                    {
                        return false; // Struck a movement-blocking cell: the actor body cannot fit
                                      // here
                    }
                }
            }
            return true; // The candidate position layout is completely clear and open
        }

        // Structural integrity checker verifying that input dimension vectors are positive and
        // stable
        void requireBodySize(glm::vec2 bodySize)
        {
            if (!isFinitePositive(bodySize))
            {
                throw std::invalid_argument("Navigation body size must be finite and positive");
            }
        }
    }

    // Verifies whether a character layout can stand flatly on standard terrain footing at a
    // targeted grid cell
    bool canStandAt(const TileMap& map, Cell cell, glm::vec2 bodySize)
    {
        return canOccupy(map, {cell, ClimbSurface::None}, bodySize);
    }

    // Loops through spatial alignment flags to determine if an entity can cling to an adjacent
    // climbable asset cell
    bool canClimbAt(const TileMap& map, Cell cell, glm::vec2 bodySize)
    {
        for (const ClimbSurface surface :
             {ClimbSurface::LeftWall, ClimbSurface::RightWall, ClimbSurface::Ceiling})
        {
            if (canOccupy(map, {cell, surface}, bodySize))
            {
                return true; // Successfully matched an available attachment anchor point placement
            }
        }
        return false;
    }

    // Projects physical pixel Aabb bounds for an actor standing or clinging along a particular tile
    // structural axis
    Aabb boundsAtSurface(int tileSize, RouteLocation location, glm::vec2 bodySize)
    {
        Aabb bounds = boxInCell(tileSize, location.cell, bodySize);
        const float left = static_cast<float>(location.cell.x * tileSize);
        const float top = static_cast<float>(location.cell.y * tileSize);

        // Transpose the structural layout box coordinates to press securely against the targeting
        // perimeter edge
        switch (location.surface)
        {
        case ClimbSurface::None:
            break; // Default standing alignment centered directly on the cell bottom floor line

        case ClimbSurface::LeftWall:
            bounds.topLeft.x = left; // Snap the absolute left side box border directly to the
                                     // tile's left perimeter boundary
            break;

        case ClimbSurface::RightWall:
            bounds.topLeft.x = left + static_cast<float>(tileSize) -
                               bodySize.x; // Flush the right side box border against the tile's
                                           // right perimeter boundary
            break;

        case ClimbSurface::Ceiling:
            bounds.topLeft.y =
                top; // Snap the top box edge directly up against the tile's inside ceiling edge
            break;
        }
        return bounds;
    }

    // Core validation routing checker confirming spatial clearance and verifying structural anchor
    // points are present to hold the body
    bool canOccupy(const TileMap& map, RouteLocation location, glm::vec2 bodySize)
    {
        requireBodySize(bodySize);
        if (!map.contains(location.cell))
        {
            return false; // Cell coordinate location falls completely outside map configuration
                          // arrays
        }

        // Calculate the relative pixel box projection matching this specific surface layout mode
        const Aabb bounds = boundsAtSurface(map.tileSize(), location, bodySize);
        if (!bodyFits(map, bounds))
        {
            return false; // Abort instantly if solid tile elements overlap the calculated volume
                          // space
        }

        // What holds the body up: the cell below for the floor, or a climbable tile
        // beside or above it.
        if (location.surface == ClimbSurface::None)
        {
            return map.blocksMovement(
                {location.cell.x,
                 location.cell.y +
                     1}); // Enforce solid tile ground existence on the row immediately below
        }

        // Query target property grids to ensure ladder or fence attributes populate the matching
        // contact plane
        return touchesClimbable(map, bounds, location.surface);
    }
}
