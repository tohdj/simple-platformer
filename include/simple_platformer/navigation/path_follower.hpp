/**
 * @file path_follower.hpp
 * @brief Waypoint evaluation state machines and input macro replay loops for following navigation
 * paths.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#pragma once

#include <cstddef>
#include <optional>

#include <glm/vec2.hpp>

#include "simple_platformer/navigation/navigation_path.hpp"

namespace simple_platformer
{
    struct Aabb;
    struct Body;
    struct FlyingMovement;
    struct InputIntentions;
    struct PlatformerMovement;
    struct SurfaceClimb;

    /**
     * @brief Identifies the functional verification phase inside an active waypoint transition
     * sequence.
     */
    enum class PathStepPhase
    {
        ApproachStart, /**< Aligning the physical body with the starting point of the current
                          waypoint edge. */
        ReplayInputs,  /**< Executing a pre-recorded frame-by-frame macro program (e.g., executing a
                          precise jump code). */
        AwaitArrival   /**< Monitoring kinematic conditions until physics updates settle the body at
                          the target destination bounds. */
    };

    /**
     * @brief Runtime state tracking an actor's progress along a planned waypoint trajectory.
     * @note Advancing the follower generates steering command intentions, whereas external
     * kinematics updates resolve actual translation.
     */
    struct PathFollower
    {
        std::optional<NavigationPath>
            path; /**< The active waypoint collection sequence currently being tracked. */
        std::size_t nextStep = 0; /**< Index identifier matching the target step node. Points to
                                     array bounds length when complete. */
        PathStepPhase phase = PathStepPhase::ApproachStart; /**< Active sub-step tracking condition
                                                               guiding steering macros. */
        float programElapsed = 0.0F;   /**< Accumulated duration in seconds spent processing the
                                          active step's pre-recorded macro program. */
        std::optional<glm::vec2> goal; /**< The original spatial target position; used to throttle
                                          redundant search updates for stationary goals. */
        std::size_t breaksWhenPlanned =
            0; /**< Historic read offset tracking the map's tile break registry during path
                  initialization; mismatches trigger automatic path recalculations. */
    };

    /** @brief Initializes the follower state tracking fields and transitions alignment to the first
     * waypoint node. */
    void setPath(PathFollower& follower, NavigationPath path);

    /** @brief Purges all loaded waypoint sequences and resets internal tracking phase indicators.
     */
    void clearPath(PathFollower& follower);

    /** @brief Verifies if all registered waypoint nodes have been successfully cleared. Always
     * false if no path is bound. */
    bool pathComplete(const PathFollower& follower);

    /**
     * @brief Calculates steering vectors and input intentions to guide an airborne agent along its
     * path layout.
     *
     * Computes direct line-of-sight tracking vectors towards successive nodes, applying damping
     * parameters on the terminal frame to ensure precise positioning atop waypoints.
     *
     * @param bounds Bounding parameters mapping the spatial footprint of the flying agent.
     * @param movement Physics parameters defining maximum speed capacities for flight locomotion.
     * @param follower Runtime tracking module containing path sequences and active step states.
     * @param deltaTime The discrete frame simulation duration step slice measured in seconds.
     * @return InputIntentions Computed steering bits driving flight propulsion.
     */
    InputIntentions followFlyingPath(
        const Aabb& bounds,
        const FlyingMovement& movement,
        PathFollower& follower,
        float deltaTime);

    /**
     * @brief Calculates steering input macros to guide a platforming agent along its trajectory.
     *
     * Handles complex transition states depending on the active traversal type:
     * - **Walk:** Navigates directly toward target coordinates and initiates braking sequences at
     * destination limits.
     * - **Jump/Fall:** Aligns with takeoff edges, executes recorded macro inputs, and verifies
     * arrival once grounding states stabilize.
     * - **Climb:** Commands alignment to surface entry planes, then loops input recordings.
     * Requires a valid climb config context.
     *
     * @note Unexpected deviations from waypoint perimeters terminate tracking. Grip commands
     * persist between transitions.
     *
     * @param body Raw physics configuration tracking positioning coordinates and current velocity
     * vectors.
     * @param movement Component configuration storing runtime grounding flags and memory indicators
     * for platformer motion.
     * @param follower Runtime tracking module containing path sequences and active step states.
     * @param deltaTime The discrete frame simulation duration step slice measured in seconds.
     * @param climb Optional surface alignment data mapping active vertical or horizontal attachment
     * parameters.
     * @return InputIntentions Computed inputs driving platformer kinematics.
     */
    InputIntentions followPlatformerPath(
        const Body& body,
        const PlatformerMovement& movement,
        PathFollower& follower,
        float deltaTime,
        const SurfaceClimb* climb = nullptr);
}
