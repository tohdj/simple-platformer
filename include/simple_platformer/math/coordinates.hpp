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
    /** @brief Target internal width of the screen viewport in pixels. World coordinates scale
     * horizontally with X increasing towards the right. */
    constexpr int InternalWidth = 320;

    /** @brief Target internal height of the screen viewport in pixels. World coordinates scale
     * vertically with Y increasing downwards. */
    constexpr int InternalHeight = 180;

    /** @brief Complete 2D rendering viewport boundaries expressed as floating-point vector
     * dimensions. */
    constexpr glm::vec2 InternalViewportSize = {
        static_cast<float>(InternalWidth),
        static_cast<float>(InternalHeight)};

    /**
     * @brief A discrete 2D point mapping a column and row location within a tile map grid.
     */
    struct Cell
    {
        int x = 0; /**< Horizontal coordinate index (column position from left-to-right). */
        int y = 0; /**< Vertical coordinate index (row position from top-to-bottom). */
    };

    /**
     * @brief Compares two Cell structures for equality.
     * @param left The first cell coordinate block.
     * @param right The second cell coordinate block.
     * @return true if both column and row positions match perfectly, false otherwise.
     */
    constexpr bool operator==(Cell left, Cell right)
    {
        return left.x == right.x && left.y == right.y;
    }

    /**
     * @brief Compares two Cell structures for inequality.
     * @param left The first cell coordinate block.
     * @param right The second cell coordinate block.
     * @return true if either the column or row positions differ, false otherwise.
     */
    constexpr bool operator!=(Cell left, Cell right)
    {
        return !(left == right);
    }

    /**
     * @brief Custom hashing functor to allow using a `Cell` as a key in unordered containers.
     *
     * Enables using standard lookup maps like `std::unordered_map<Cell, Entity, CellHash>`.
     */
    struct CellHash
    {
        /**
         * @brief Computes a combined hash value from a cell's coordinates.
         * @param cell The 2D cell coordinate structure to hash.
         * @return A unique size_t hash token representing the location.
         */
        std::size_t operator()(Cell cell) const;
    };

    /**
     * @brief Defines the discrete 2D spatial dimensions of a grid system.
     */
    struct GridSize
    {
        int width = 0;  /**< Number of columns spanning across the horizontal axis. */
        int height = 0; /**< Number of rows spanning down the vertical axis. */
    };

    /**
     * @brief Checks if a given cell position falls within the boundaries of a grid.
     *
     * @param grid The dimensions (width and height) of the active grid layout.
     * @param cell The 2D coordinate location to validate.
     * @return true if the cell coordinates are non-negative and sit strictly within the grid
     * extents, false otherwise.
     */
    constexpr bool contains(GridSize grid, Cell cell)
    {
        return cell.x >= 0 && cell.x < grid.width && cell.y >= 0 && cell.y < grid.height;
    }

    /**
     * @brief Defines a rectangular bounding region of cells spanning from a starting corner to an
     * ending corner.
     */
    struct CellRange
    {
        Cell first; /**< The starting boundary cell (typically top-left). */
        Cell last;  /**< The ending boundary cell (typically bottom-right). This boundary is
                       inclusive. */
    };

    /** @brief Checks if the designated grid cell falls completely within the boundary limits of the
     * cell range. */
    bool contains(const CellRange& range, Cell cell);

    /** @brief Computes the smallest possible enclosing cell range that aggregates both parameters.
     */
    CellRange unionOf(const CellRange& left, const CellRange& right);

    /**
     * @brief Floating-point margin of tolerance in world pixels used for precise tile-edge
     * collision tests.
     * @note Feet resting exactly on a floor belong to the cell above it; bounding box edges do not
     * include the cell beyond them.
     */
    constexpr float EdgeTolerance = 0.001F;

    /**
     * @brief Resolves the grid cell context enclosing a given absolute world pixel coordinate
     * point.
     * @note A point falling exactly on a tile border edge resolves to the cell to its right or
     * below it.
     * @param tileSize The length of one square tile cell in world pixels.
     * @param worldPosition Global pixel coordinates.
     * @return Cell Enclosing cell index location values.
     */
    Cell cellAt(int tileSize, glm::vec2 worldPosition);

    /** @brief Computes absolute world pixel coordinates pointing to the top-left corner vertex of a
     * cell. */
    glm::vec2 cellCorner(int tileSize, Cell cell);

    /**
     * @brief Resolves the cell index something stands in based on its footprint coordinates.
     * @note Coordinates falling exactly on a tile's top border line map to the cell block directly
     * above it.
     * @param tileSize The length of one square tile cell in world pixels.
     * @param feet Baseline pixel footprint position.
     * @return Cell Enclosing cell coordinate index.
     */
    Cell cellAtFeet(int tileSize, glm::vec2 feet);

    /** @brief Computes baseline world pixel coordinates matching the horizontal midpoint center of
     * a cell's bottom edge. */
    glm::vec2 feetInCell(int tileSize, Cell cell);
}
