/**
 * @file coordinates.hpp
 * @brief Viewport dimensions, 2D grid cell structures, cell ranges, and pixel-to-grid coordinate
 * transformations.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#pragma once

#include <cstddef>

#include <glm/vec2.hpp>

namespace simple_platformer
{
    // Target internal width of the screen viewport in pixels. World coordinates scale horizontally
    // with X increasing towards the right.
    constexpr int InternalWidth = 320;

    // Target internal height of the screen viewport in pixels. World coordinates scale vertically
    // with Y increasing downwards.
    constexpr int InternalHeight = 180;

    // Complete 2D rendering viewport boundaries expressed as floating-point vector dimensions.
    constexpr glm::vec2 InternalViewportSize = {
        static_cast<float>(InternalWidth),
        static_cast<float>(InternalHeight)};

    // A discrete 2D point mapping a column and row location within a tile map grid.
    struct Cell
    {
        int x = 0; // Horizontal coordinate index (column position from left-to-right).
        int y = 0; // Vertical coordinate index (row position from top-to-bottom).
    };

    // Compares two Cell structures for exact column and row equality.
    constexpr bool operator==(Cell left, Cell right)
    {
        return left.x == right.x && left.y == right.y;
    }

    // Compares two Cell structures for value inequality.
    constexpr bool operator!=(Cell left, Cell right)
    {
        return !(left == right);
    }

    // Custom hashing functor to allow using a Cell as a key in unordered containers like
    // std::unordered_map.
    struct CellHash
    {
        // Computes a combined hash value from a cell's column and row coordinates.
        std::size_t operator()(Cell cell) const;
    };

    // Defines the discrete 2D spatial dimensions of a grid system.
    struct GridSize
    {
        int width = 0;  // Number of columns spanning across the horizontal axis.
        int height = 0; // Number of rows spanning down the vertical axis.
    };

    // Checks if a given cell position falls within the valid non-negative boundaries of a grid
    // layout.
    constexpr bool contains(GridSize grid, Cell cell)
    {
        return cell.x >= 0 && cell.x < grid.width && cell.y >= 0 && cell.y < grid.height;
    }

    // Defines an inclusive rectangular bounding region of cells spanning from a starting corner to
    // an ending corner.
    struct CellRange
    {
        Cell first; // The starting boundary cell (typically top-left).
        Cell last; // The ending boundary cell (typically bottom-right). This boundary is inclusive.
    };

    // Checks if the designated grid cell falls completely within the boundary limits of the cell
    // range.
    bool contains(const CellRange& range, Cell cell);

    // Computes the smallest possible enclosing cell range that aggregates both parameters.
    CellRange unionOf(const CellRange& left, const CellRange& right);

    // Floating-point margin of tolerance in world pixels used for precise tile-edge collision
    // tests. Feet resting exactly on a floor belong to the cell above it; bounding box edges do not
    // include the cell beyond them.
    constexpr float EdgeTolerance = 0.001F;

    // Resolves the grid cell context enclosing a given absolute world pixel coordinate point.
    // A point falling exactly on a tile border edge resolves to the cell to its right or below it.
    Cell cellAt(int tileSize, glm::vec2 worldPosition);

    // Computes absolute world pixel coordinates pointing to the top-left corner vertex of a cell.
    glm::vec2 cellCorner(int tileSize, Cell cell);

    // Resolves the cell index something stands in based on its footprint coordinates.
    // Coordinates falling exactly on a tile's top border line map to the cell block directly above
    // it.
    Cell cellAtFeet(int tileSize, glm::vec2 feet);

    // Computes baseline world pixel coordinates matching the horizontal midpoint center of a cell's
    // bottom edge.
    glm::vec2 feetInCell(int tileSize, Cell cell);
}
