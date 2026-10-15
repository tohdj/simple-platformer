/**
 * @file tile_map.cpp
 * @brief Manages the 2D grid structure, collisions, destruction, and types of tiles making up a
 * game world level.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#include "simple_platformer/world/tile_map.hpp"

#include <cstddef>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "simple_platformer/math/coordinates.hpp"

namespace simple_platformer
{
    // Constructor: Initializes grid dimensions, tiles data, and structural constraints
    TileMap::TileMap(
        int tileSize,
        int width,
        int height,
        std::vector<int> tiles,
        std::vector<TileDefinition> definitions)
        : mapTileSize(tileSize),
          mapWidth(width),
          mapHeight(height),
          tileIds(std::move(tiles)),
          tileDefinitions(std::move(definitions))
    {
        // Enforce physical constraints on layout scaling metrics
        if (mapTileSize <= 0)
        {
            throw std::invalid_argument("A tile map must have a positive tile size");
        }
        if (mapWidth <= 0 || mapHeight <= 0)
        {
            throw std::invalid_argument("A tile map must have positive dimensions");
        }

        // Verify data block length perfectly aligns with target grid boundary metrics
        const std::size_t expectedTileCount =
            static_cast<std::size_t>(mapWidth) * static_cast<std::size_t>(mapHeight);
        if (tileIds.size() != expectedTileCount)
        {
            throw std::invalid_argument("Tile count does not match the map dimensions");
        }

        // Strict Engine Convention: Tile ID 0 is reserved strictly for fully passable air space
        if (tileDefinitions.empty() || tileDefinitions.front().blocksMovement ||
            tileDefinitions.front().blocksSight || tileDefinitions.front().climbable)
        {
            throw std::invalid_argument("Tile ID zero must be defined as empty");
        }

        // Validate platform mechanics constraints (e.g., Ladders/climbables must have solid
        // attributes)
        for (const TileDefinition& definition : tileDefinitions)
        {
            if (definition.climbable && !definition.blocksMovement)
            {
                throw std::invalid_argument("A climbable tile must block movement");
            }
        }

        // Verify out-of-bounds IDs do not exist in the active buffer matrix
        for (const int tile : tileIds)
        {
            if (tile < 0 || static_cast<std::size_t>(tile) >= tileDefinitions.size())
            {
                throw std::invalid_argument("A tile map contains an undefined tile ID");
            }
        }
    }

    // Factory Method: Builds a formal TileMap instance directly out of descriptive string streams
    TileMap TileMap::fromAscii(
        int tileSize,
        const std::vector<std::string>& rows,
        std::vector<TileDefinition> definitions,
        const std::map<char, int>& legend)
    {
        if (rows.empty())
        {
            throw std::invalid_argument("An ASCII tile map needs at least one row");
        }

        const int mapWidth = static_cast<int>(rows.front().size());
        if (mapWidth == 0)
        {
            throw std::invalid_argument("An ASCII tile map cannot have empty rows");
        }

        std::vector<int> tiles;
        tiles.reserve(static_cast<std::size_t>(mapWidth) * rows.size());

        // Process rows sequentially, matching raw symbols against provided dictionary entries
        for (const std::string& row : rows)
        {
            if (static_cast<int>(row.size()) != mapWidth)
            {
                throw std::invalid_argument("Every ASCII tile map row must have the same width");
            }

            for (const char symbol : row)
            {
                const auto entry = legend.find(symbol);
                if (entry == legend.end())
                {
                    throw std::invalid_argument("An ASCII tile map contains an unknown symbol");
                }
                tiles.push_back(entry->second);
            }
        }

        return TileMap(
            tileSize,
            mapWidth,
            static_cast<int>(rows.size()),
            std::move(tiles),
            std::move(definitions));
    }

    // Accessors for native grid sizing values
    int TileMap::width() const
    {
        return mapWidth;
    }

    int TileMap::height() const
    {
        return mapHeight;
    }

    GridSize TileMap::size() const
    {
        return {mapWidth, mapHeight};
    }

    int TileMap::tileSize() const
    {
        return mapTileSize;
    }

    // Returns absolute total pixel sizing bounds of the level map layout
    float TileMap::pixelWidth() const
    {
        return static_cast<float>(mapWidth * mapTileSize);
    }

    float TileMap::pixelHeight() const
    {
        return static_cast<float>(mapHeight * mapTileSize);
    }

    // Bounds checking routine for cell coordinate sets
    bool TileMap::contains(Cell cell) const
    {
        return simple_platformer::contains(size(), cell);
    }

    // Resolves raw internal ID assigned at a target grid cell coordinate
    int TileMap::tileAt(Cell cell) const
    {
        if (!contains(cell))
        {
            throw std::out_of_range("Cell is outside the map");
        }

        return tileIds[indexOf(cell)];
    }

    // Fetches full behavioral property definitions associated with a designated cell coordinate
    const TileDefinition& TileMap::definitionAt(Cell cell) const
    {
        const int tileId = tileAt(cell);
        return tileDefinitions[static_cast<std::size_t>(tileId)];
    }

    // Line-of-sight test logic: defaults to fallback movement blocking properties if out-of-bounds
    bool TileMap::blocksSight(Cell cell) const
    {
        return contains(cell) ? definitionAt(cell).blocksSight : blocksMovement(cell);
    }

    // Collision boundaries handler: returns safe defaults for cells extending past map boundaries
    bool TileMap::blocksMovement(Cell cell) const
    {
        // Infinite solid horizontal side walls
        if (cell.x < 0 || cell.x >= mapWidth)
        {
            return true;
        }

        // Bottomless pit (allows actors to drop offscreen safely before trigger zones fire)
        if (cell.y < 0)
        {
            return false;
        }

        // Solid ceiling boundary protection
        if (cell.y >= mapHeight)
        {
            return true;
        }

        return definitionAt(cell).blocksMovement;
    }

    // Checks if the tile at the cell coordinate is designated as a climbable surface
    bool TileMap::climbableAt(Cell cell) const
    {
        return contains(cell) && definitionAt(cell).climbable;
    }

    // Destructible Environmental Logic: swaps out a cell with a broken alternative if valid
    bool TileMap::breakTile(Cell cell)
    {
        if (!contains(cell))
        {
            return false;
        }

        const std::optional<int> broken = definitionAt(cell).breaksIntoTileId;
        if (!broken.has_value())
        {
            return false; // Tile is unbreakable/invulnerable
        }

        // Modify database index array state and append change log records
        tileIds[indexOf(cell)] = *broken;
        brokenCellLog.push_back(cell);
        return true;
    }

    // Returns structural tracking logs detailing broken map sections for serialization or particle
    // emitters
    const std::vector<Cell>& TileMap::brokenCells() const
    {
        return brokenCellLog;
    }

    // Transforms internal 2D grid vector mappings down into an absolute 1D linear array buffer
    // offset index
    std::size_t TileMap::indexOf(Cell cell) const
    {
        return static_cast<std::size_t>(cell.y) * static_cast<std::size_t>(mapWidth) +
               static_cast<std::size_t>(cell.x);
    }
}
