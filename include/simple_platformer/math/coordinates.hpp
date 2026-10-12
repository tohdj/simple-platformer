#pragma once

#include <cstddef>

#include <glm/vec2.hpp>

namespace simple_platformer
{
    /// Target internal width of the screen viewport in pixels.
    /// World coordinates scale horizontally with X increasing towards the right.
    constexpr int InternalWidth = 320;

    /// Target internal height of the screen viewport in pixels.
    /// World coordinates scale vertically with Y increasing downwards.
    constexpr int InternalHeight = 180;

    /// Complete 2D rendering viewport boundaries expressed as floating-point vector dimensions.
    constexpr glm::vec2 InternalViewportSize = {
        static_cast<float>(InternalWidth),
        static_cast<float>(InternalHeight)};

    /**
     * @brief A discrete 2D point mapping a column and row location within a tile map grid.
     */
    struct Cell
    {
        /// Horizontal coordinate index (column position from left-to-right).
        int x = 0;
        /// Vertical coordinate index (row position from top-to-bottom).
        int y = 0;
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
     * @brief Custom hashing functor to allow Using a `Cell` as a key in unordered containers.
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
        /// Number of columns spanning across the horizontal axis.
        int width = 0;

        /// Number of rows spanning down the vertical axis.
        int height = 0;
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
        /// The starting boundary cell (typically top-left).
        Cell first;

        /// The ending boundary cell (typically bottom-right). This boundary is inclusive.
        Cell last;
    };


    bool contains(const CellRange& range, Cell cell);
    // The smallest range holding both.
    CellRange unionOf(const CellRange& left, const CellRange& right);

    // In world pixels: tolerance used for tile-edge tests. Feet on a floor belong to the
    // cell above it; box edges do not include the cell beyond them. Map-edge contact uses
    // the same tolerance.
    constexpr float EdgeTolerance = 0.001F;

    // The cell containing the point. A point on a tile edge belongs to the cell to its right
    // or below, and points left of or above the map give negative cells.
    Cell cellAt(int tileSize, glm::vec2 worldPosition);
    // The cell's top-left corner.
    glm::vec2 cellCorner(int tileSize, Cell cell);

    // The cell something with these feet stands in. Feet exactly on a tile's top edge
    // belong to the cell above it, the one the actor occupies.
    Cell cellAtFeet(int tileSize, glm::vec2 feet);
    // The feet of something standing in the cell: the middle of the cell's bottom edge.
    glm::vec2 feetInCell(int tileSize, Cell cell);
}
