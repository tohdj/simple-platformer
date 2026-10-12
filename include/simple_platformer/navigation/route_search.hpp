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
    /**
     * @brief Callback descriptor that generates all valid outgoing graph edges from a source
     * location.
     *
     * Each returned element defines an immediate neighborhood transition along with its traversal
     * costs and frame input recording payloads.
     */
    using ConnectionFunction = std::function<std::vector<RouteConnection>(RouteLocation location)>;

    /**
     * @brief Callback descriptor evaluating the lower-bound cost approximation between two grid
     * cells.
     * @note To maintain mathematical admissibility for A*, the returned value must be non-negative
     * and must never overestimate the actual remaining path cost.
     */
    using HeuristicFunction = std::function<int(Cell cell, Cell goal)>;

    /**
     * @brief Encapsulates the final outcome state returned by a graph search operation.
     */
    struct RouteSearchResult
    {
        std::optional<Route> route; /**< The optimized, lowest-cost pathway to the goal node, or
                                       std::nullopt if no solution exists. */
    };

    /**
     * @brief Computes the lowest-cost route from an origin node to any valid surface node inside
     * the target goal cell using A*.
     *
     * Because node tracking binds both raw cell indexes and structural alignment matrices together,
     * a single grid block's floor, wall axes, and ceilings are evaluated as isolated, unique
     * positions. Configurations lacking climbing features naturally prune evaluations down to
     * standard horizontal surfaces.
     *
     * @note Supplying a heuristic functor that uniformly defaults to zero shifts this routine's
     * execution behavior into Dijkstra's algorithm.
     * @param start The initial node coordinates and anchor surface initializing the search context.
     * @param goal The target grid block bounding box target criteria.
     * @param grid Structural boundaries defining the size constraints of the operational grid map.
     * @param connections Functional evaluator supplying active neighbor expansion edges.
     * @param heuristic Approximator function computing node weight estimations towards the target
     * destination cell.
     * @return RouteSearchResult Result payload mapping the calculated transition node timeline
     * block.
     */
    RouteSearchResult findLowestCostRoute(
        RouteLocation start,
        Cell goal,
        GridSize grid,
        const ConnectionFunction& connections,
        const HeuristicFunction& heuristic);
}
