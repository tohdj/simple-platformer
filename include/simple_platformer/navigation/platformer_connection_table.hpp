/**
 * @file platformer_connection_table.hpp
 * @brief Thread-safe graph cache containing cell connections and dynamic reconstruction tracking
 * for tile breaks.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#pragma once

#include <cstddef>
#include <vector>

#include "simple_platformer/math/coordinates.hpp"
#include "simple_platformer/navigation/route.hpp"
#include "simple_platformer/navigation/platformer_traversal_profile.hpp"

namespace simple_platformer
{
    class TileMap;

    /**
     * @brief Stores every cell's valid paths for a level map, organized by navigation capability
     * profile.
     *
     * Introducing a new profile generates full map navigation data. Environment updates like
     * tile destructions incrementally trigger graph alterations, recalculating only the nodes
     * whose spatial simulation footprints intersect the broken cells.
     */
    class PlatformerConnectionTable
    {
    public:
        /**
         * @brief Integrates historic map changes onto the graph, then predictively compiles the
         * table if not present.
         * @param map The static environment geometry grid layout context.
         * @param profile The traversal boundaries and configuration of the actor group.
         * @throws std::invalid_argument If the profile spatial scales or integration timelines are
         * non-positive or non-finite.
         */
        void prepare(const TileMap& map, const PlatformerTraversalProfile& profile);

        /**
         * @brief Scans active graph caches and rebuilds cells whose structural lookahead swept
         * across newly broken blocks.
         * @param map The static level configuration structure layout updated after tile breaks.
         */
        void applyRecordedTileBreaks(const TileMap& map);

        /** @brief Verifies whether the tracking table cache has been allocated and compiled for a
         * given profile. */
        bool isBuilt(const PlatformerTraversalProfile& profile) const;

        /**
         * @brief Provides read-only paths exiting all faces of a given cell for a compiled profile.
         * @param cell The targeted origin coordinate location grid position.
         * @param profile The movement identity ruleset template being validated against.
         * @return The cached list of valid outgoing structural route edges.
         * @throws std::out_of_bound or std::runtime_error if off-map coordinates are given or if
         * the profile is uncompiled.
         */
        const std::vector<RouteConnection>& connections(
            Cell cell,
            const PlatformerTraversalProfile& profile) const;

        /**
         * @brief Retrieves the environmental tile range probed when simulating the cell's
         * kinematics.
         * @param cell The targeted origin coordinate location grid position.
         * @param profile The movement identity ruleset template being validated against.
         * @return CellRange Visual boundaries containing the full sensor sweeping footprint of the
         * cell.
         * @throws std::out_of_bound or std::runtime_error if off-map coordinates are given or if
         * the profile is uncompiled.
         */
        const CellRange& footprint(Cell cell, const PlatformerTraversalProfile& profile) const;

    private:
        struct CellConnections
        {
            std::vector<RouteConnection> connections; /** cells;     /**< Row-major flat matrix
                                                         storage mapping grid coordinate records. */
            CellRange footprint;
        };

        struct ProfileTable
        {
            PlatformerTraversalProfile profile;
            GridSize grid;
            // Row-major storage: the cell at (x, y) is at y * grid.width + x.
            std::vector<CellConnections> cells;
        };

        const ProfileTable* findTable(const PlatformerTraversalProfile& profile) const;
        const CellConnections& entry(Cell cell, const PlatformerTraversalProfile& profile) const;

        std::vector<ProfileTable> profileTables; /**< Dynamic table instances separating individual
                                                    path capability configurations. */
        std::size_t breaksSeen = 0; /**< Read pointer offset tracking processed rows from the map's
                                       structural destruction log. */
    };
}
