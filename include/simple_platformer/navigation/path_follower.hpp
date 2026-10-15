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

    // Identifies the verification phase inside an active waypoint transition sequence
    enum class PathStepPhase
    {
        // Aligning the physical body with the starting point of the current waypoint edge
        ApproachStart,
        // Executing a pre-recorded frame-by-frame macro program (e.g., precise jump input)
        ReplayInputs,
        // Monitoring kinematic conditions until physics updates settle the body at the target
        // bounds
        AwaitArrival
    };

    // Runtime state tracking an actor's progress along a planned waypoint trajectory
    // Note: Advancing the follower generates steering commands; external physics updates resolve
    // actual movement.
    struct PathFollower
    {
        // The active waypoint collection sequence currently being tracked
        std::optional<NavigationPath> path;
        // Index identifier matching the target step node (equals array size when complete)
        std::size_t nextStep = 0;
        // Active sub-step tracking condition guiding steering macros
        PathStepPhase phase = PathStepPhase::ApproachStart;
        // Accumulated duration (seconds) spent processing the active step's macro program
        float programElapsed = 0.0F;
        // The original target position; used to throttle redundant search updates for stationary
        // goals
        std::optional<glm::vec2> goal;
        // Map's tile break registry version at path initialization; mismatches trigger automatic
        // recalculations
        std::size_t breaksWhenPlanned = 0;
    };

    // Initializes the follower state tracking fields and transitions alignment to the first
    // waypoint node
    void setPath(PathFollower& follower, NavigationPath path);

    // Purges all loaded waypoint sequences and resets internal tracking phase indicators
    void clearPath(PathFollower& follower);

    // Verifies if all registered waypoint nodes have been successfully cleared (false if no path is
    // bound)
    bool pathComplete(const PathFollower& follower);

    // Calculates steering vectors and input intentions to guide an airborne agent along its path
    // layout Computes line-of-sight tracking vectors, applying damping parameters on the terminal
    // frame for precise alignment
    InputIntentions followFlyingPath(
        const Aabb& bounds,
        const FlyingMovement& movement,
        PathFollower& follower,
        float deltaTime);

    // Calculates steering input macros to guide a platforming agent along its trajectory
    // Walk: Navigates toward target coordinates and initiates braking sequences at destination
    // limits Jump/Fall: Aligns with takeoff edges, plays macro inputs, and verifies arrival once
    // grounding stabilizes Climb: Commands alignment to surface entry planes, then loops input
    // recordings (requires valid climb data) Note: Unexpected deviations from waypoint perimeters
    // terminate tracking; grip commands persist between transitions
    InputIntentions followPlatformerPath(
        const Body& body,
        const PlatformerMovement& movement,
        PathFollower& follower,
        float deltaTime,
        const SurfaceClimb* climb = nullptr);
}
