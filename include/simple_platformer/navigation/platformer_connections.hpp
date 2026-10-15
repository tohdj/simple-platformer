/**
 * @file platformer_connections.hpp
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

    // Encapsulates completed edge connections alongside their evaluated calculation bounds
    struct BuiltPlatformerConnections
    {
        // List of successful structural movements available from this grid cell
        std::vector<RouteConnection> connections;
        // Conservative grid boundary encapsulating all tiles probed or swept by the lookahead
        // simulation
        CellRange footprint;
    };

    // Runs short-term lookaheads (walks, jumps, drops, climbs) to find reachable neighboring cells
    // The footprint logs checked cells so graph regenerators can invalidate this node if nearby
    // tiles break
    BuiltPlatformerConnections buildPlatformerConnections(
        const TileMap& map,
        Cell cell,
        const PlatformerTraversalProfile& profile);
}
