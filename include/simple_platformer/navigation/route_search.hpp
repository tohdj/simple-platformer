/**
 * @file route_search.hpp
 * @brief Graph search algorithms and functional descriptors for calculating low-level actor routes
 * using A*.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#pragma once

#include <functional>
#include <optional>
#include <vector>

#include "simple_platformer/math/coordinates.hpp"
#include "simple_platformer/navigation/route.hpp"

namespace simple_platformer
{
    // Callback descriptor that generates all valid outgoing graph edges from a source location.
    // Each returned element defines a neighborhood transition along with its traversal costs and
    // input payloads.
    using ConnectionFunction = std::function<std::vector<RouteConnection>(RouteLocation location)>;

    // Callback descriptor evaluating the lower-bound cost approximation between two grid cells.
    // Must be non-negative and must never overestimate the actual remaining path cost to maintain
    // admissibility.
    using HeuristicFunction = std::function<int(Cell cell, Cell goal)>;

    // Encapsulates the final outcome state returned by a graph search operation.
    struct RouteSearchResult
    {
        std::optional<Route>
            route; // The optimized, lowest-cost pathway to the goal node, or std::nullopt.
    };

    // Computes the lowest-cost route from an origin node to any valid surface node inside the
    // target goal cell using A*. Single grid blocks evaluate floor, wall axes, and ceilings as
    // unique, isolated positions. Supplying a heuristic functor that uniformly defaults to zero
    // transforms this routine into Dijkstra's algorithm.
    RouteSearchResult findLowestCostRoute(
        RouteLocation start,
        Cell goal,
        GridSize grid,
        const ConnectionFunction& connections,
        const HeuristicFunction& heuristic);
}
