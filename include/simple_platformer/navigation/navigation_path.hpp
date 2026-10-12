/**
 * @file navigation_path.hpp
 * @brief Spatial waypoints, path containers, and search status results for high-level actor
 * navigation.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#pragma once

#include <optional>
#include <vector>

#include <glm/vec2.hpp>

#include "simple_platformer/input/input_program.hpp"
#include "simple_platformer/navigation/traversal.hpp"

namespace simple_platformer
{
    /**
     * @brief A precise spatial checkpoint position recording the locomotion technique and input
     * macro used to reach it.
     * @note Ground walking or unconstrained flight traversals leave the input sequence empty.
     */
    struct Waypoint
    {
        glm::vec2 feet{0.0F, 0.0F}; /** waypoints;     /**< Ordered array containing consecutive
                                       path tracking checkpoints. */
        Traversal traversal = Traversal::Fly;
        InputProgram inputs;
    };

    /**
     * @brief Where the body's feet rest at the start, and the waypoints that lead from there
     * in the order travelled. No waypoints means the start is the end.
     */
    struct NavigationPath
    {
        glm::vec2 startFeet{0.0F, 0.0F};
        std::vector<Waypoint> waypoints;
    };

    /**
     * @brief Safely evaluates the absolute termination pixel coordinates from a provided navigation
     * trajectory.
     * @return glm::vec2 The final path waypoint coordinates, or the baseline origin point if the
     * vector is empty.
     */
    inline glm::vec2 endOf(const NavigationPath& path)
    {
        return path.waypoints.empty() ? path.startFeet : path.waypoints.back().feet;
    }

    /**
     * @brief Identifies the categorical outcome status of a high-level navigation path request.
     */
    enum class NavigationPathStatus
    {
        Found,      /**< A valid pathway terminating successfully inside the target goal cell was
                       resolved. */
        Unreachable /**< No successful or valid pathway could be constructed to the destination goal
                       cell. */
    };

    /**
     * @brief Encapsulates a resolved path sequence alongside its final tracking and distance
     * metrics.
     */
    struct NavigationPathResult
    {
        NavigationPathStatus status =
            NavigationPathStatus::Unreachable; /**< Outcome validation tracking category. */
        std::optional<NavigationPath>
            path; /**< The resolved waypoint timeline sequence, or std::nullopt if unreachable. */
        float remainingDistance = 0.0F; /**< Absolute scalar distance in pixels from the final path
                                           waypoint to the actual goal point. */
    };
}
