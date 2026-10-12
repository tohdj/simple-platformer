/**
 * @file tile_map.hpp
 * @brief Manages the 2D grid structure, collisions, destruction, and types of tiles making up a
 * game world level.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#pragma once

#include <cstddef>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "simple_platformer/math/coordinates.hpp"
#include "simple_platformer/render/sprite.hpp"

namespace simple_platformer
{
    /**
     * @brief Shared rules and art properties for a specific tile ID.
     *
     * Movement and sight blocking properties are independent:
     * - Glass stops bodies but does not block sight.
     * - Grass blocks sight but allows bodies to enter it.
     */
    struct TileDefinition
    {
        bool blocksMovement =
            false; /**< If true, entities cannot walk or pass through this tile. */
        bool blocksSight =
            false;           /**< If true, this tile obstructs visibility or line-of-sight casts. */
        SpriteRegion sprite; /**< Visual texture details representing the tile on screen. */
        std::optional<int> breaksIntoTileId =
            std::nullopt; /**< The tile ID this tile changes into when destroyed. A nullopt means it
                             cannot be broken. */
        bool climbable = false; /**< If true, entities can perform climbing mechanics (e.g.,
                                   ladders) inside this tile. */
    };

    /**
     * @brief Manages the 2D grid structure, collisions, destruction, and types of tiles making up a
     * game world level.
     */
    class TileMap
    {
    public:
        /**
         * @brief Constructs a new TileMap object.
         * @param tileSize The length of one square tile cell in world pixels.
         * @param width Total number of columns in the grid layout.
         * @param height Total number of rows in the grid layout.
         * @param tiles A flat array listing the tile IDs layout from top-left to bottom-right.
         * @param definitions Properties associated with each valid tile ID.
         */
        TileMap(
            int tileSize,
            int width,
            int height,
            std::vector<int> tiles,
            std::vector<TileDefinition> definitions);

        /**
         * @brief Helper function to generate a TileMap out of a text-based ASCII grid
         * representation.
         * @param tileSize The length of one square tile cell in world pixels.
         * @param rows The lines of characters representing columns and rows.
         * @param definitions Properties associated with each valid tile ID.
         * @param legend A lookup mapping characters to their matching integer tile IDs.
         * @return The parsed TileMap object.
         */
        static TileMap fromAscii(
            int tileSize,
            const std::vector<std::string>& rows,
            std::vector<TileDefinition> definitions,
            const std::map<char, int>& legend);

        /** @brief Returns the side length of one square tile cell in world pixels. */
        int tileSize() const;

        /** @brief Returns the total number of columns in the grid layout. */
        int width() const;

        /** @brief Returns the total number of rows in the grid layout. */
        int height() const;

        /** @brief Returns the structured column and row bounds dimensions of the grid. */
        GridSize size() const;

        /** @brief Returns the absolute horizontal expanse of the map layout in world pixels. */
        float pixelWidth() const;

        /** @brief Returns the absolute vertical expanse of the map layout in world pixels. */
        float pixelHeight() const;

        /** @brief Checks if the given cell coordinates are within the layout limits of the map. */
        bool contains(Cell cell) const;

        /** @brief Returns the raw integer tile ID located at the designated cell. */
        int tileAt(Cell cell) const;

        /** @brief Retrieves the behavior and sprite definitions for the tile at the designated
         * cell. */
        const TileDefinition& definitionAt(Cell cell) const;

        /**
         * @brief Checks if a cell restricts movement.
         * @note Out-of-bounds cells block movement on the left, right, and bottom borders. The sky
         * above is open.
         */
        bool blocksMovement(Cell cell) const;

        /**
         * @brief Checks if a cell restricts visibility.
         * @note Out-of-bounds cells block sight on the left, right, and bottom borders. The sky
         * above is open.
         */
        bool blocksSight(Cell cell) const;

        /**
         * @brief Checks if the cell is climbable.
         * @note Out-of-bounds borders are never climbable.
         */
        bool climbableAt(Cell cell) const;

        /**
         * @brief Attempts to break the tile at the targeted cell location.
         *
         * Replaces the tile ID with its configured `breaksIntoTileId`.
         *
         * @param cell The grid coordinates of the tile to break.
         * @return true if the tile was successfully broken, false if unbreakable or off-map.
         */
        bool breakTile(Cell cell);

        /**
         * @brief Retrieves a chronological log of all successful tile breaks.
         *
         * Systems like pathfinding navigation can monitor how far they have read this log
         * to incrementally update graph connections and paths when a structural change happens.
         */
        const std::vector<Cell>& brokenCells() const;

    private:
        /**
         * @brief Calculates a 1D row-major array index from a 2D grid position.
         * @warning The cell **must** be inside the valid map boundaries before calling this
         * function.
         */
        std::size_t indexOf(Cell cell) const;

        int mapTileSize = 0;      /**< Side length of a single cell in world pixels. */
        int mapWidth = 0;         /**< Total number of columns in the grid. */
        int mapHeight = 0;        /**< Total number of rows in the grid. */
        std::vector<int> tileIds; /**< Flat container holding the active layout IDs of the map. */
        std::vector<TileDefinition>
            tileDefinitions; /**< Shared configuration lookup based on tile ID. */
        std::vector<Cell>
            brokenCellLog; /**< Historical order tracking where successful breaks occurred. */
    };
}
