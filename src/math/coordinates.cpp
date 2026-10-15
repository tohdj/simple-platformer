/**
 * @file coordinates.cpp
 * @brief Viewport dimensions, 2D grid cell structures, cell ranges, and pixel-to-grid coordinate
 * transformations.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#include "simple_platformer/math/coordinates.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>

namespace simple_platformer
{
    // Generates a collision-free 64-bit hash index from a 2D grid cell by packing 32-bit
    // coordinates side-by-side
    std::size_t CellHash::operator()(Cell cell) const
    {
        const auto column = static_cast<std::uint64_t>(static_cast<std::uint32_t>(cell.x));
        const auto row = static_cast<std::uint64_t>(static_cast<std::uint32_t>(cell.y));

        // Shift columns to the upper 32 bits and merge with rows in the lower 32 bits before
        // hashing
        return std::hash<std::uint64_t>{}((column << 32U) | row);
    }

    // Identifies if a specific 2D cell coordinate lies fully inside an inclusive bounding range
    // block
    bool contains(const CellRange& range, Cell cell)
    {
        return cell.x >= range.first.x && cell.x <= range.last.x && cell.y >= range.first.y &&
               cell.y <= range.last.y;
    }

    // Generates an expansive bounding cell range box by computing extreme outer coordinates from
    // two inputs
    CellRange unionOf(const CellRange& left, const CellRange& right)
    {
        return {
            {std::min(left.first.x, right.first.x), std::min(left.first.y, right.first.y)},
            {std::max(left.last.x, right.last.x), std::max(left.last.y, right.last.y)}};
    }

    // Projects absolute continuous 2D world pixel values into discrete integer grid cell index
    // locations
    Cell cellAt(int tileSize, glm::vec2 worldPosition)
    {
        return {
            static_cast<int>(std::floor(worldPosition.x / static_cast<float>(tileSize))),
            static_cast<int>(std::floor(worldPosition.y / static_cast<float>(tileSize))),
        };
    }

    // Computes the absolute top-left world pixel coordinate matching the baseline corner of a grid
    // cell
    glm::vec2 cellCorner(int tileSize, Cell cell)
    {
        return {
            static_cast<float>(cell.x * tileSize),
            static_cast<float>(cell.y * tileSize),
        };
    }

    // Isolates the specific floor cell directly beneath a continuous feet coordinate layout anchor
    // point
    Cell cellAtFeet(int tileSize, glm::vec2 feet)
    {
        // Incorporate a miniature floating tolerance margin (EdgeTolerance) to prevent boundary
        // precision bleed
        return cellAt(tileSize, {feet.x, feet.y - EdgeTolerance});
    }

    // Computes the absolute bottom-center world pixel coordinates representing the standing
    // baseline anchor inside a cell
    glm::vec2 feetInCell(int tileSize, Cell cell)
    {
        const glm::vec2 topLeft = cellCorner(tileSize, cell);
        return {
            topLeft.x + static_cast<float>(tileSize) * 0.5F, // Halfway across the tile width
            topLeft.y + static_cast<float>(tileSize)}; // At the absolute bottom tile pixel line
    }
}
