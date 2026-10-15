/**
 * @file segment_cast.cpp
 * @brief Linear ray/box sweeping routines and intersection tests against environment tiles and
 * physics boundaries.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#include "simple_platformer/physics/segment_cast.hpp"

#include <algorithm>
#include <optional>
#include <stdexcept>
#include <vector>

#include <glm/common.hpp>
#include <glm/vec2.hpp>

#include "simple_platformer/math/aabb.hpp"
#include "simple_platformer/math/coordinates.hpp"
#include "simple_platformer/math/validation.hpp"
#include "simple_platformer/world/tile_map.hpp"

namespace simple_platformer
{
    namespace
    {
        // Narrows [first, last] to where the segment lies within this axis's bounds.
        // Returns false if it misses those bounds or no shared interval remains.
        bool castAxis(
            float start,
            float movement,
            float minimum,
            float maximum,
            float& first,
            float& last)
        {
            // If there is no trajectory motion on this axis, simply verify if the start coordinate
            // is inside limits
            if (movement == 0.0F)
            {
                return start >= minimum && start <= maximum;
            }

            // Calculate intersection entry and exit fractions along the line segment trajectory
            float enter = (minimum - start) / movement;
            float leave = (maximum - start) / movement;
            if (enter > leave)
            {
                std::swap(enter, leave); // Ensure enter tracks the true near impact point
            }

            // Tighten the global entry and exit window anchors across axes clipping passes
            first = std::max(first, enter);
            last = std::min(last, leave);
            return first <= last; // Return true if a valid overlap window path remains intact
        }

        // The part of the segment inside the box, as fractions along it.
        struct SegmentSpan
        {
            float enter = 0.0F;
            float leave = 0.0F;
        };

        // Performs a complete Liang-Barsky parametric line clipping check to find a segment's
        // overlap timeline with an AABB box
        std::optional<SegmentSpan> segmentSpan(const Aabb& box, glm::vec2 start, glm::vec2 end)
        {
            const glm::vec2 movement = end - start;
            float first = 0.0F;
            float last = 1.0F;

            // Clip the trajectory segment against horizontal boundaries, then vertical boundaries
            // consecutively
            if (!castAxis(start.x, movement.x, box.topLeft.x, rightOf(box), first, last) ||
                !castAxis(start.y, movement.y, box.topLeft.y, bottomOf(box), first, last))
            {
                return std::nullopt; // The vector path completely misses the target box area
            }
            return SegmentSpan{first, last};
        }

        // Candidate cells for the segment, including the moving box's half-size.
        // Exact intersections are checked separately by each kind of cast.
        CellRange segmentTileRange(
            int tileSize,
            glm::vec2 start,
            glm::vec2 end,
            glm::vec2 movingSize)
        {
            if (!isFinite(start) || !isFinite(end) || !isFiniteNonNegative(movingSize))
            {
                throw std::invalid_argument(
                    "Tile segment casts require finite, non-negative-sized data");
            }

            // Inflate bounds by the moving box footprint to capture all candidate collision cells
            const glm::vec2 halfSize = movingSize * 0.5F;
            const glm::vec2 minimum = glm::min(start, end) - halfSize;
            const glm::vec2 maximum = glm::max(start, end) + halfSize;
            return {cellAt(tileSize, minimum), cellAt(tileSize, maximum)};
        }

        // Utility builder transforming a discrete cell coordinate into a solid world-pixel AABB
        // bounding box
        Aabb tileBox(int tileSize, Cell cell)
        {
            const float tileLength = static_cast<float>(tileSize);
            return {cellCorner(tileSize, cell), {tileLength, tileLength}};
        }

        // Scans the calculated grid subset to collect all intersection segment windows hitting
        // sight-obstructing tiles
        std::vector<SegmentSpan> sightBlockingSpans(
            const TileMap& map,
            glm::vec2 start,
            glm::vec2 end)
        {
            const CellRange cells = segmentTileRange(map.tileSize(), start, end, {0.0F, 0.0F});
            std::vector<SegmentSpan> spans;

            // Parse through the grid subsection bounds to extract overlap durations
            for (int row = cells.first.y; row <= cells.last.y; ++row)
            {
                for (int column = cells.first.x; column <= cells.last.x; ++column)
                {
                    const Cell cell{column, row};
                    if (!map.blocksSight(cell))
                    {
                        continue; // Passable airspace node
                    }
                    const auto span = segmentSpan(tileBox(map.tileSize(), cell), start, end);
                    if (span.has_value())
                    {
                        spans.push_back(*span);
                    }
                }
            }
            return spans;
        }

        // Spans that touch or overlap from time zero form the starting cover.
        // The first span after a gap blocks sight; an exact shared corner is not a gap.
        std::optional<float> firstHitAfterStartingCover(std::vector<SegmentSpan> spans)
        {
            // Sort intersection spans chronologically by their entry percentage indices
            std::sort(
                spans.begin(),
                spans.end(),
                [](const SegmentSpan& left, const SegmentSpan& right)
                { return left.enter < right.enter; });

            float startingCoverEnd = 0.0F;

            // Evaluate contiguous cover regions starting from the observer origin coordinates
            for (const SegmentSpan& span : spans)
            {
                // A gap in the cover sequence reveals a distinct solid occlusion boundary farther
                // down the ray path
                if (span.enter > startingCoverEnd)
                {
                    return span.enter; // Return the time fraction where sight is officially lost
                }
                startingCoverEnd = std::max(startingCoverEnd, span.leave);
            }
            return std::nullopt; // Sight vector is clear of separate blocking obstacles
        }
    }

    // Expands a stationary target box using the dimensions of a moving box to implement Minkowski
    // Sum collision physics
    Aabb expandedForMovingBox(const Aabb& target, glm::vec2 movingSize)
    {
        const glm::vec2 halfSize = movingSize * 0.5F;
        return {target.topLeft - halfSize, target.size + movingSize};
    }

    // Parametric line clipper evaluating line segment entry factors against a specific AABB target
    // boundaries node
    std::optional<float> segmentCast(const Aabb& box, glm::vec2 start, glm::vec2 end)
    {
        if (!isFinite(box.topLeft) || !isFinite(box.size) || !isFinite(start) || !isFinite(end) ||
            box.size.x <= 0.0F || box.size.y <= 0.0F)
        {
            throw std::invalid_argument("Segment casts require finite, positive-sized data");
        }

        const std::optional<SegmentSpan> span = segmentSpan(box, start, end);
        if (!span.has_value())
        {
            return std::nullopt; // Ray path completely misses the target box bounds
        }
        return span->enter; // Return the exact timeline entry fraction (0.0 to 1.0)
    }

    // Sweeps a moving bounding box across a level grid corridor path to find the earliest solid
    // obstacle collision event
    std::optional<TileSegmentHit> segmentCastMovementBlockingTiles(
        const TileMap& map,
        glm::vec2 start,
        glm::vec2 end,
        glm::vec2 movingSize)
    {
        const CellRange cells = segmentTileRange(map.tileSize(), start, end, movingSize);
        std::optional<TileSegmentHit> earliest;

        // Scan through all grid blocks along the sweep footprint path matrix
        for (int row = cells.first.y; row <= cells.last.y; ++row)
        {
            for (int column = cells.first.x; column <= cells.last.x; ++column)
            {
                const Cell cell{column, row};
                if (!map.blocksMovement(cell))
                {
                    continue; // Skip passable space nodes
                }

                // Inflate the static floor/wall block using Minkowski geometry parameters to check
                // against a point cast
                const Aabb target = expandedForMovingBox(tileBox(map.tileSize(), cell), movingSize);
                const auto hit = segmentCast(target, start, end);

                // Store the result context data if it yields a closer collision timeline marker
                // than previous scans
                if (hit.has_value() && (!earliest.has_value() || *hit < earliest->segmentTime))
                {
                    earliest = TileSegmentHit{*hit, cell};
                }
            }
        }
        return earliest;
    }

    // Traces visibility lines through level cover configurations, returning the fraction index
    // where line-of-sight terminates
    std::optional<float> segmentCastSightBlockingTiles(
        const TileMap& map,
        glm::vec2 start,
        glm::vec2 end)
    {
        // Extract all sight-obscuring spans along the path vector and filter out the originating
        // entity's starting cover block
        return firstHitAfterStartingCover(sightBlockingSpans(map, start, end));
    }

}
