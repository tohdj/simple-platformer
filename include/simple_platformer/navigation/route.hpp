/**
 * @file route.hpp
 * @brief Search nodes, graph edges, and input replay sequences for low-level navigation routes.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#pragma once

#include <vector>

#include "simple_platformer/input/input_program.hpp"
#include "simple_platformer/math/coordinates.hpp"
#include "simple_platformer/movement/surface_climb.hpp"
#include "simple_platformer/navigation/traversal.hpp"

namespace simple_platformer
{
    /**
     * @brief A precise spatial coordinate bound with its structural surface anchoring state.
     * @note ClimbSurface::None implies the entity is anchored to a standard horizontal walkable
     * floor.
     */
    struct RouteLocation
    {
        Cell cell; /**< Grid coordinate positioning cells. */
        ClimbSurface surface =
            ClimbSurface::None; /**< Specific environmental plane attachment tracking. */
    };

    /** @brief Evaluates two route locations for structural equality. */
    constexpr bool operator==(RouteLocation left, RouteLocation right)
    {
        return left.cell == right.cell && left.surface == right.surface;
    }

    /**
     * @brief Represents a single atomic edge movement traversing towards a destination node.
     */
    struct RouteStep
    {
        RouteLocation destination; /**< The target cell and surface node being navigated to. */
        Traversal traversal =
            Traversal::Fly;  /**< Operational physical movement technique utilized for transit. */
        InputProgram inputs; /**< Frame-by-frame replay macro steps required for advanced jumps,
                                drops, or climbs; empty during trivial walk loops. */
    };

    /**
     * @brief Graph connectivity description linking a source node context to an outgoing transition
     * step.
     * @note Multi-cell spans are compressed into individual directional paths containing unified
     * metrics.
     */
    struct RouteConnection
    {
        RouteStep step; /**< The edge transit behavior and target destination records. */
        int cost = 1;   /**< Positive path evaluation penalty expressed uniformly across the search
                           graph (e.g., cell steps or frames). */
        ClimbSurface sourceSurface = ClimbSurface::None; /**< Originating structural plane axis
                                                            within the source grid cell block. */
    };

    /**
     * @brief Complete linear sequence tracking path transitions from origin to final target
     * destination.
     * @note An empty step sequence denotes that the starting initialization target matches final
     * completion parameters.
     */
    struct Route
    {
        RouteLocation start; /**< Node coordinates pinpointing route initialization parameters. */
        std::vector<RouteStep> steps; /**< Ordered array containing consecutive node steps. */
    };

    /**
     * @brief Safely evaluates the absolute termination node coordinates from a provided route
     * trajectory.
     * @return RouteLocation The final path waypoint node destination, or the baseline origin point
     * if the vector is empty.
     */
    inline RouteLocation endOf(const Route& route)
    {
        return route.steps.empty() ? route.start : route.steps.back().destination;
    }
}
