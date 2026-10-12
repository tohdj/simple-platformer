/**
 * @file platformer_connection_builder.hpp
 * @brief Predictively simulates and compiles valid physics movement connections between grid cells.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#pragma once

#include <vector>

#include "simple_platformer/math/coordinates.hpp"
#include "simple_platformer/navigation/route.hpp"
#include "simple_platformer/navigation/platformer_traversal_profile.hpp"

namespace simple_platformer
{
    class TileMap;

    /**
     * @brief Encapsulates completed edge connections alongside their evaluated calculation bounds.
     */
    struct BuiltPlatformerConnections
    {
        std::vector<RouteConnection> connections; /**< List of successful structural movements
                                                     available from this grid cell. */
        CellRange footprint; /**< Conservative grid boundary encapsulating all tiles probed or swept
                                by the lookahead simulation. */
    };

    /**
     * @brief Simulates physical movement trajectories from every traversal surface supported by the
     * profile in a cell.
     *
     * Runs short-term kinematic lookaheads (walks, jumps, drops, climbs) to evaluate which nearby
     * cells are safely reachable. The returned footprint logs all checked cells, allowing graph
     * regenerators to accurately invalidate this node if any surrounding tiles break.
     *
     * @param map The static environment geometry grid layout context used to compute solid blocks.
     * @param cell The central grid cell position from which to simulate outgoing movements.
     * @param profile The static movement limits and physical attributes of the actor category.
     * @return BuiltPlatformerConnections A structure wrapping valid target edges and their total
     * evaluation footprint.
     */
    BuiltPlatformerConnections buildPlatformerConnections(
        const TileMap& map,
        Cell cell,
        const PlatformerTraversalProfile& profile);
}
