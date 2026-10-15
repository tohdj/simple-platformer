/**
 * @file platformer_connection_table.cpp
 * @brief Thread-safe graph cache containing cell connections and dynamic reconstruction tracking
 * for tile breaks.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#include "simple_platformer/navigation/platformer_connection_table.hpp"

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <utility>
#include <vector>

#include "simple_platformer/math/coordinates.hpp"
#include "simple_platformer/math/validation.hpp"
#include "simple_platformer/navigation/route.hpp"
#include "simple_platformer/navigation/platformer_connections.hpp"
#include "simple_platformer/navigation/platformer_traversal_profile.hpp"
#include "simple_platformer/world/tile_map.hpp"

namespace simple_platformer
{
    namespace
    {
        // Projects a 2D tile cell index down into a continuous row-major 1D array offset coordinate
        std::size_t indexOf(GridSize grid, Cell cell)
        {
            return static_cast<std::size_t>(cell.y) * static_cast<std::size_t>(grid.width) +
                   static_cast<std::size_t>(cell.x);
        }
    }

    // Inspects and instantiates full navigation graph tables for an incoming capabilities profile
    void PlatformerConnectionTable::prepare(
        const TileMap& map,
        const PlatformerTraversalProfile& profile)
    {
        if (!isFinitePositive(profile.size) || !isFinitePositive(profile.stepSeconds))
        {
            throw std::invalid_argument(
                "Connections require a finite, positive profile size and step");
        }

        const GridSize grid = map.size();
        if (!profileTables.empty() && (profileTables.front().grid.width != grid.width ||
                                       profileTables.front().grid.height != grid.height))
        {
            throw std::logic_error("A connection table belongs to one map");
        }

        // Process dynamic environmental geometry updates to keep historical node connections
        // completely clean
        applyRecordedTileBreaks(map);
        if (isBuilt(profile))
        {
            return; // Target profile caching table is already optimized and stored
        }

        ProfileTable table{profile, grid, {}};
        table.cells.reserve(static_cast<std::size_t>(grid.width) * grid.height);

        // Nested column-by-row traversal building route connection vectors across every map cell
        // coordinate
        for (int row = 0; row < grid.height; ++row)
        {
            for (int column = 0; column < grid.width; ++column)
            {
                BuiltPlatformerConnections built =
                    buildPlatformerConnections(map, {column, row}, profile);

                table.cells.push_back({std::move(built.connections), built.footprint});
            }
        }
        profileTables.push_back(std::move(table));
    }

    // Identifies graph segments whose predictive sweeps intersect newly broken walls/floors and
    // recalculates them incrementally
    void PlatformerConnectionTable::applyRecordedTileBreaks(const TileMap& map)
    {
        const std::vector<Cell>& broken = map.brokenCells();
        if (breaksSeen > broken.size())
        {
            throw std::logic_error("A connection table belongs to one map");
        }
        if (breaksSeen == broken.size())
        {
            return; // Cache remains fully synchronized with destructible tile maps
        }

        // Isolate the un-processed chunk of the map modification log array
        const auto unseen = broken.begin() + static_cast<std::ptrdiff_t>(breaksSeen);

        for (ProfileTable& table : profileTables)
        {
            for (int row = 0; row < table.grid.height; ++row)
            {
                for (int column = 0; column < table.grid.width; ++column)
                {
                    const Cell cell{column, row};
                    CellConnections& stored = table.cells[indexOf(table.grid, cell)];

                    // Footprint Intersection Rule: invalidate if newly broken cells fall inside
                    // this node's original verification bounds
                    const bool touched = std::any_of(
                        unseen,
                        broken.end(),
                        [&stored](Cell brokenCell)
                        { return contains(stored.footprint, brokenCell); });
                    if (!touched)
                    {
                        continue; // Underlying geometry remains stable for this node: preserve
                                  // cache rows
                    }

                    // Re-calculate edge routes exclusively for the corrupted cell entry point
                    BuiltPlatformerConnections built =
                        buildPlatformerConnections(map, cell, table.profile);

                    stored = {std::move(built.connections), built.footprint};
                }
            }
        }
        breaksSeen =
            broken.size(); // Advance index tracking limits to close the synchronization step
    }

    // Returns true if a profile table instance matches the incoming capabilities template
    // signatures
    bool PlatformerConnectionTable::isBuilt(const PlatformerTraversalProfile& profile) const
    {
        return findTable(profile) != nullptr;
    }

    // Public reader extracting the cached list of valid outgoing route links matching a target cell
    // layout
    const std::vector<RouteConnection>& PlatformerConnectionTable::connections(
        Cell cell,
        const PlatformerTraversalProfile& profile) const
    {
        return entry(cell, profile).connections;
    }

    // Public reader extracting the complete sensory scanning footprint box bounding a targeted node
    // cell
    const CellRange& PlatformerConnectionTable::footprint(
        Cell cell,
        const PlatformerTraversalProfile& profile) const
    {
        return entry(cell, profile).footprint;
    }

    // Traverses internal containers linearly to isolate table data matches using capabilities
    // profile keys
    const PlatformerConnectionTable::ProfileTable* PlatformerConnectionTable::findTable(
        const PlatformerTraversalProfile& profile) const
    {
        const auto table = std::find_if(
            profileTables.begin(),
            profileTables.end(),
            [&profile](const ProfileTable& candidate) { return candidate.profile == profile; });
        return table == profileTables.end() ? nullptr : &*table;
    }

    // Handles index boundaries validation and provides direct memory lookups for requested
    // sub-nodes
    const PlatformerConnectionTable::CellConnections& PlatformerConnectionTable::entry(
        Cell cell,
        const PlatformerTraversalProfile& profile) const
    {
        const ProfileTable* table = findTable(profile);
        if (table == nullptr)
        {
            throw std::logic_error("The connection table has not built this profile");
        }
        if (!contains(table->grid, cell))
        {
            throw std::out_of_range("The cell is outside the connection table's map");
        }
        return table->cells[indexOf(table->grid, cell)];
    }
}
