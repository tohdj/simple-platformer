/**
 * @file body.cpp
 * @brief Rigid body components, gravity simulation, and axis-aligned collision sweeps against map
 * geometry.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#include "simple_platformer/physics/body.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include <glm/vec2.hpp>

#include "simple_platformer/math/aabb.hpp"
#include "simple_platformer/math/coordinates.hpp"
#include "simple_platformer/math/validation.hpp"
#include "simple_platformer/world/tile_map.hpp"

namespace simple_platformer
{
    namespace
    {
        // Converts a world-space minimum pixel coordinate to the starting overlapping tile index
        // row/column
        int firstOverlappingTile(int tileSize, float minimum)
        {
            return static_cast<int>(std::floor(minimum / static_cast<float>(tileSize)));
        }

        // Converts a world-space maximum pixel coordinate to the final overlapping tile index
        // row/column
        int lastOverlappingTile(int tileSize, float maximum)
        {
            return static_cast<int>(std::ceil(maximum / static_cast<float>(tileSize))) - 1;
        }

        // Asserves mathematical integrity and guarantees that the bounding dimensions originate
        // inside level boundaries
        void validateBounds(const TileMap& map, const Aabb& bounds, glm::vec2 displacement)
        {
            if (!isFinite(bounds.topLeft) || !isFinite(bounds.size) || !isFinite(displacement) ||
                bounds.size.x <= 0.0F || bounds.size.y <= 0.0F)
            {
                throw std::invalid_argument("Collision requires finite, positive-sized bounds");
            }

            if (bounds.topLeft.x < 0.0F || rightOf(bounds) > map.pixelWidth() ||
                bottomOf(bounds) > map.pixelHeight())
            {
                throw std::invalid_argument("Collision bounds must begin inside the map walls");
            }
        }

        // Internal categorisation matching target property blocks used during sweeps
        enum class SurfaceKind
        {
            Blocking,
            Climbable
        };

        // Queries the tile map to determine if a cell contains the specific physical trait
        // requested
        bool hasSurface(const TileMap& map, Cell cell, SurfaceKind kind)
        {
            return kind == SurfaceKind::Climbable ? map.climbableAt(cell)
                                                  : map.blocksMovement(cell);
        }

        // Scans a specific column index between row limits to locate target blocking properties
        bool columnHasSurface(
            const TileMap& map,
            int column,
            int firstRow,
            int lastRow,
            SurfaceKind kind)
        {
            for (int row = firstRow; row <= lastRow; ++row)
            {
                if (hasSurface(map, {column, row}, kind))
                {
                    return true; // Struck a valid collision surface property inside the column
                                 // slice
                }
            }
            return false;
        }

        // Scans a specific row index between column limits to locate target blocking properties
        bool rowHasSurface(
            const TileMap& map,
            int row,
            int firstColumn,
            int lastColumn,
            SurfaceKind kind)
        {
            for (int column = firstColumn; column <= lastColumn; ++column)
            {
                if (hasSurface(map, {column, row}, kind))
                {
                    return true; // Struck a valid collision surface property inside the row slice
                }
            }
            return false;
        }

        // Verifies whether a calculated tile intercept distance falls properly within and curtails
        // the requested move slice
        bool stopsRequestedMovement(float candidate, float requested)
        {
            if (requested > 0.0F)
            {
                return candidate >= 0.0F && candidate <= requested;
            }

            return candidate <= 0.0F && candidate >= requested;
        }

        // Simple structural wrapper tracking resulting traversal space and obstacle interception
        // statuses
        struct CollisionSweepResult
        {
            float distance = 0.0F;
            bool hitTile = false;
        };

        // Horizontal tile collision: scan columns nearest first, checking the body's rows.
        // Include the outside column at either map wall, but never scan beyond it.
        CollisionSweepResult sweepHorizontalCollision(
            const TileMap& map,
            const Aabb& bounds,
            float requested,
            SurfaceKind kind)
        {
            if (requested == 0.0F)
            {
                return {0.0F, false};
            }
            const int tileSize = map.tileSize();
            // Determine vertical span ranges by mapping bounding coordinates to grid rows
            const int firstRow = std::max(0, firstOverlappingTile(tileSize, bounds.topLeft.y));
            const int lastRow =
                std::min(map.height() - 1, lastOverlappingTile(tileSize, bottomOf(bounds)));

            // 1. Horizontal Right Passing Axis
            if (requested > 0.0F)
            {
                const float rightEdge = rightOf(bounds);
                const int firstColumn = firstOverlappingTile(tileSize, rightEdge);
                const float destination = rightEdge + requested;
                const int lastColumn = destination >= map.pixelWidth()
                                           ? map.width()
                                           : firstOverlappingTile(tileSize, destination);

                // Sweep outward horizontally column-by-column to detect incoming obstructions
                for (int column = firstColumn; column <= lastColumn; ++column)
                {
                    if (!columnHasSurface(map, column, firstRow, lastRow, kind))
                    {
                        continue;
                    }
                    const float tileLeft = static_cast<float>(column * tileSize);
                    const float distance = tileLeft - rightEdge;
                    if (stopsRequestedMovement(distance, requested))
                    {
                        return {distance, true}; // Solid impact registered: stop travel slice here
                    }
                }
            }
            // 2. Horizontal Left Passing Axis
            else
            {
                const float leftEdge = bounds.topLeft.x;
                const int firstColumn = lastOverlappingTile(tileSize, leftEdge);
                const float destination = leftEdge + requested;
                const int lastColumn =
                    destination <= 0.0F ? -1 : firstOverlappingTile(tileSize, destination);

                // Sweep backward horizontally through columns to process reverse impacts
                for (int column = firstColumn; column >= lastColumn; --column)
                {
                    if (!columnHasSurface(map, column, firstRow, lastRow, kind))
                    {
                        continue;
                    }
                    const float tileRight = static_cast<float>((column + 1) * tileSize);
                    const float distance = tileRight - leftEdge;
                    if (stopsRequestedMovement(distance, requested))
                    {
                        return {distance, true};
                    }
                }
            }
            return {requested, false}; // Path is entirely open over the evaluated trajectory
        }

        // Vertical tile collision: scan rows nearest first, checking the body's columns.
        // The outside bottom row blocks movement; the outside top row remains open.
        CollisionSweepResult sweepVerticalCollision(
            const TileMap& map,
            const Aabb& bounds,
            float requested,
            SurfaceKind kind)
        {
            if (requested == 0.0F)
            {
                return {0.0F, false};
            }
            const int tileSize = map.tileSize();
            // Determine horizontal span ranges by mapping bounding coordinates to grid columns
            const int firstColumn = std::max(0, firstOverlappingTile(tileSize, bounds.topLeft.x));
            const int lastColumn =
                std::min(map.width() - 1, lastOverlappingTile(tileSize, rightOf(bounds)));

            // 1. Vertical Downward Passing Axis
            if (requested > 0.0F)
            {
                const float bottomEdge = bottomOf(bounds);
                const int firstRow = firstOverlappingTile(tileSize, bottomEdge);
                const float destination = bottomEdge + requested;
                const int lastRow = destination >= map.pixelHeight()
                                        ? map.height()
                                        : firstOverlappingTile(tileSize, destination);

                // Sweep downward row-by-row to test for solid footing landings
                for (int row = firstRow; row <= lastRow; ++row)
                {
                    if (!rowHasSurface(map, row, firstColumn, lastColumn, kind))
                    {
                        continue;
                    }
                    const float tileTop = static_cast<float>(row * tileSize);
                    const float distance = tileTop - bottomEdge;
                    if (stopsRequestedMovement(distance, requested))
                    {
                        return {distance, true};
                    }
                }
            }
            // 2. Vertical Upward Passing Axis
            else
            {
                const float topEdge = bounds.topLeft.y;
                const int firstRow = lastOverlappingTile(tileSize, topEdge);
                const float destination = topEdge + requested;
                const int lastRow =
                    destination <= 0.0F ? -1 : firstOverlappingTile(tileSize, destination);

                // Sweep upward through rows to evaluate overhead platform collisions
                for (int row = firstRow; row >= lastRow; --row)
                {
                    if (!rowHasSurface(map, row, firstColumn, lastColumn, kind))
                    {
                        continue;
                    }
                    const float tileBottom = static_cast<float>((row + 1) * tileSize);
                    const float distance = tileBottom - topEdge;
                    if (stopsRequestedMovement(distance, requested))
                    {
                        return {distance, true};
                    }
                }
            }
            return {requested, false}; // Trajectory path slice remains completely unobstructed
        }

        // Reuse collision sweeps as short contact probes. Climbable selects grip
        // surfaces; ordinary movement always selects blocking surfaces.
        CollisionContacts probeSurfaces(const TileMap& map, const Aabb& bounds, SurfaceKind kind)
        {
            validateBounds(map, bounds, {0.0F, 0.0F});

            // Cast a miniature fractional epsilon step out from the margins to verify surface
            // proximity
            constexpr float ProbeDistance = 0.01F;

            return {
                sweepHorizontalCollision(map, bounds, -ProbeDistance, kind)
                    .hitTile, // Test Left border touch
                sweepHorizontalCollision(map, bounds, ProbeDistance, kind)
                    .hitTile, // Test Right border touch
                sweepVerticalCollision(map, bounds, ProbeDistance, kind)
                    .hitTile, // Test Ground baseline touch
                sweepVerticalCollision(map, bounds, -ProbeDistance, kind)
                    .hitTile // Test Ceiling border touch
            };
        }
    }

    // Integrates gravitational forces onto the vertical velocity component, clamping it at terminal
    // fall speed limits
    void applyGravity(Body& body, float gravity, float maximumFallSpeed, float deltaTime)
    {
        body.velocity.y = std::min(body.velocity.y + gravity * deltaTime, maximumFallSpeed);
    }

    // Translates positions axis-by-axis, checking and resolving horizontal X movement rules before
    // applying vertical Y movement slices
    CollisionContacts moveBody(const TileMap& map, Body& body, float deltaTime)
    {
        Aabb& bounds = body.bounds;
        const glm::vec2 displacement = body.velocity * deltaTime;
        validateBounds(map, bounds, displacement);

        CollisionContacts contacts;

        // 1. Horizontal Pass: Scan columns for structural impacts and translate along the X axis
        const CollisionSweepResult horizontal =
            sweepHorizontalCollision(map, bounds, displacement.x, SurfaceKind::Blocking);
        bounds.topLeft.x += horizontal.distance;
        if (horizontal.hitTile)
        {
            if (displacement.x > 0.0F)
            {
                contacts.right = true;
            }
            else
            {
                contacts.left = true;
            }
            body.velocity.x = 0.0F; // Stop horizontal momentum instantly upon hitting walls
        }

        // 2. Vertical Pass: Sweep Y from the new horizontal position coordinates to process floor
        // or ceiling contacts
        const CollisionSweepResult vertical =
            sweepVerticalCollision(map, bounds, displacement.y, SurfaceKind::Blocking);
        bounds.topLeft.y += vertical.distance;
        if (vertical.hitTile)
        {
            if (displacement.y > 0.0F)
            {
                contacts.ground = true; // Stable landing registration
            }
            else
            {
                contacts.ceiling = true; // Head bumper interception
            }
            body.velocity.y = 0.0F; // Force vertical velocity down to zero upon landing or bumping
                                    // overhead structures
        }
        return contacts;
    }

    // Probes solid tile blocks directly along the boundary margins of a stationary area box context
    CollisionContacts touchingSurfaces(const TileMap& map, const Aabb& bounds)
    {
        return probeSurfaces(map, bounds, SurfaceKind::Blocking);
    }

    // Probes perimeter tiles exclusively for elements flagged with active climbable surfaces
    // mappings
    CollisionContacts touchingClimbableSurfaces(const TileMap& map, const Aabb& bounds)
    {
        return probeSurfaces(map, bounds, SurfaceKind::Climbable);
    }
}
