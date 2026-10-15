/**
 * @file route_search.cpp
 * @brief Graph search algorithms and functional descriptors for calculating low-level actor routes
 * using A*.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#include "simple_platformer/navigation/route_search.hpp"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <optional>
#include <queue>
#include <stdexcept>
#include <utility>
#include <vector>

#include "simple_platformer/math/coordinates.hpp"
#include "simple_platformer/navigation/route.hpp"

namespace simple_platformer
{
    namespace
    {
        // How a node was reached in the cheapest way, for walking the route back to the start.
        struct IncomingStep
        {
            std::size_t parentIndex;
            RouteStep step;
        };

        // A location the search has reached. Closed once expanded, and opened again if a
        // cheaper way to it turns up.
        struct SearchNode
        {
            RouteLocation location;
            int costFromStart = 0;
            std::optional<IncomingStep> incoming;
            bool closed = false;
        };

        // Every location of a cell has its own node slot.
        constexpr int SurfacesPerCell = 4;

        // Evaluates the heuristic cost estimator function and validates that the approximation is
        // strictly non-negative.
        int estimateRemainingCost(
            const HeuristicFunction& heuristic,
            RouteLocation location,
            Cell goal)
        {
            const int estimate = heuristic(location.cell, goal);
            if (estimate < 0)
            {
                throw std::invalid_argument("A route heuristic cannot return a negative cost");
            }
            return estimate;
        }

        // A node waiting to be expanded. It holds the node's index rather than a copy, so
        // the frontier stays cheap to reorder, and the index survives nodes growing.
        struct FrontierEntry
        {
            int estimatedTotalCost;
            std::size_t nodeIndex;
        };

        // The frontier's ordering. A std::priority_queue hands out its highest-priority
        // entry first, whatever order entries went in. This comparison makes the cheapest
        // estimate the highest priority. An entry that costs more comes out later.
        struct MoreExpensive
        {
            // Binary predicate sorting entries into a min-heap structure based on total cost
            // weights.
            bool operator()(const FrontierEntry& left, const FrontierEntry& right) const
            {
                // Keep discovery order for equal estimates, including after a node reopens.
                return left.estimatedTotalCost == right.estimatedTotalCost
                           ? left.nodeIndex > right.nodeIndex
                           : left.estimatedTotalCost > right.estimatedTotalCost;
            }
        };

        // Unwinds parent graph nodes sequentially to re-construct a chronologically ordered path
        // instruction array.
        Route reconstructRoute(const std::vector<SearchNode>& nodes, std::size_t goalIndex)
        {
            std::vector<RouteStep> steps;
            std::size_t current = goalIndex;
            while (true)
            {
                const SearchNode& currentNode = nodes[current];
                if (!currentNode.incoming.has_value())
                {
                    break; // Origin root isolated: termination criterion met.
                }

                const IncomingStep& incoming = currentNode.incoming.value();
                steps.push_back(incoming.step); // Collect sequence layers backwards.
                current = incoming.parentIndex; // Pivot state directly up to parent node.
            }
            std::reverse(
                steps.begin(),
                steps.end()); // Flip reverse tracking logs into forward instruction sets.
            return {nodes[current].location, std::move(steps)};
        }

        // Serializes a multi-dimensional route location coordinate struct into a single, flat
        // vector map lookup row index.
        std::size_t slotOf(GridSize grid, RouteLocation location)
        {
            const std::size_t cell =
                static_cast<std::size_t>(location.cell.y) * static_cast<std::size_t>(grid.width) +
                static_cast<std::size_t>(location.cell.x);
            return cell * static_cast<std::size_t>(SurfacesPerCell) +
                   static_cast<std::size_t>(location.surface);
        }

        // Asserves whether a targeted cell index exists inside level dimensions and hosts valid
        // surface vectors.
        bool containsLocation(GridSize grid, RouteLocation location)
        {
            const int surface = static_cast<int>(location.surface);
            return contains(grid, location.cell) && surface >= 0 && surface < SurfacesPerCell;
        }
    }


    // Executes an admissible A* search calculation to isolate the absolute lowest-cost node
    // transition track to the destination target cell
    RouteSearchResult findLowestCostRoute(
        RouteLocation start,
        Cell goal,
        GridSize grid,
        const ConnectionFunction& connections,
        const HeuristicFunction& heuristic)
    {
        // Safety Guard Pass: Verify algorithm pointers and map space bounds are valid before
        // running search allocations
        if (!connections)
        {
            throw std::invalid_argument("Route search requires a connection function");
        }
        if (!heuristic)
        {
            throw std::invalid_argument("Route search requires a heuristic function");
        }
        if (grid.width <= 0 || grid.height <= 0)
        {
            throw std::invalid_argument("Route search requires a grid with cells");
        }
        if (!containsLocation(grid, start))
        {
            throw std::invalid_argument("Route search start must lie within the grid");
        }

        // Every location reached so far, in the order first reached; the start is node 0.
        std::vector<SearchNode> nodes{{start, 0, std::nullopt, false}};

        // Every location on the grid gets a slot in nodeAt, holding the index of its node
        // once the search reaches it, or NoNode until then.
        constexpr int NoNode = -1;
        std::vector<int> nodeAt(
            static_cast<std::size_t>(grid.width) * static_cast<std::size_t>(grid.height) *
                static_cast<std::size_t>(SurfacesPerCell),
            NoNode);
        nodeAt[slotOf(grid, start)] = 0;

        // The open nodes to expand, cheapest estimated total first.
        std::priority_queue<FrontierEntry, std::vector<FrontierEntry>, MoreExpensive> frontier;
        frontier.push({estimateRemainingCost(heuristic, start, goal), 0});

        // Checks whether going through the parent is the cheapest way found so far to the
        // place the connection leads. If it is, that place remembers the parent as its way
        // in and goes on the frontier to be expanded; if not, nothing changes.
        const auto relax =
            [&](const RouteConnection& connection, std::size_t parentIndex, int parentCost)
        {
            const RouteLocation destination = connection.step.destination;

            // Validate incoming edge structural parameters to protect tracking tables from
            // corruption
            if (connection.cost <= 0)
            {
                throw std::invalid_argument("A route connection must have positive cost");
            }
            if (!containsLocation(grid, destination))
            {
                throw std::invalid_argument("A connection leads outside the grid");
            }
            if (parentCost > std::numeric_limits<int>::max() - connection.cost)
            {
                throw std::overflow_error("A route connection cost is too large");
            }

            const int nextCost = parentCost + connection.cost;
            int& existing = nodeAt[slotOf(grid, destination)];

            // 1. The place already has a way in that costs no more than going through the
            //    parent so nothing changes.
            if (existing != NoNode &&
                nextCost >= nodes[static_cast<std::size_t>(existing)].costFromStart)
            {
                return;
            }

            // Verify calculation stability to shield priority queues from integer wrapping
            // artifacts
            const int estimate = estimateRemainingCost(heuristic, destination, goal);
            if (nextCost > std::numeric_limits<int>::max() - estimate)
            {
                throw std::overflow_error("A route estimate is too large");
            }

            const int total = nextCost + estimate;

            // 2. The place is reached for the first time. Add its node, with the parent as
            //    its way in, and put it on the frontier.
            if (existing == NoNode)
            {
                existing = static_cast<int>(nodes.size());
                nodes.push_back(
                    {destination, nextCost, IncomingStep{parentIndex, connection.step}, false});
                frontier.push({total, static_cast<std::size_t>(existing)});
                return;
            }

            // 3. Going through the parent is cheaper than the place's way in so far: make the
            //    parent its way in, and put it back on the frontier. If it was already
            //    expanded this reopens it, so the saving reaches the places beyond it.
            SearchNode& known = nodes[static_cast<std::size_t>(existing)];
            known.costFromStart = nextCost;
            known.incoming = IncomingStep{parentIndex, connection.step};
            known.closed = false; // Re-open node to propagate optimal cost corrections downstream
            frontier.push({total, static_cast<std::size_t>(existing)});
        };

        // Main algorithm evaluation loop: expand cheapest nodes sequentially until the target cell
        // is isolated
        while (!frontier.empty())
        {
            // 1. Take the cheapest entry. Skip it if its node has already been expanded. That
            //    happens when a cheaper way to a node was found: relax pushes a new entry but
            //    doesn't remove the old one and the new one comes out first.
            const FrontierEntry next = frontier.top();
            frontier.pop();
            SearchNode& node = nodes[next.nodeIndex];
            if (node.closed)
            {
                continue; // Discard stale higher-cost paths remaining in queue
            }

            // 2. The first location in the goal cell taken off the frontier is the
            //    cheapest one because the heuristic never overestimates.
            if (node.location.cell == goal)
            {
                return {reconstructRoute(nodes, next.nodeIndex)}; // Goal reached: trace back parent
                                                                  // steps to return optimal path
            }

            const RouteLocation location = node.location;

            // 3. Expand it: close it, then for each place one step away, check whether going
            //    through this node is the cheapest way there found so far.
            const int parentCost = node.costFromStart;
            node.closed = true; // Finalize cost tracks for this node layout context
            for (const RouteConnection& connection : connections(location))
            {
                relax(connection, next.nodeIndex, parentCost);
            }
        }

        return {}; // Return empty optional payload if the frontier drains completely with zero
                   // solutions located
    }
}
