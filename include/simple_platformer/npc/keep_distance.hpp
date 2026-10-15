/**
 * @file keep_distance_state_machine.hpp
 * @brief State machine transitions and frame updates for the defensive, ranged-focused KeepDistance
 * NPC archetype.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#pragma once

#include <optional>
#include "simple_platformer/npc/npc.hpp"

namespace simple_platformer
{
    struct Actor;
    struct NpcFacts;
    struct NpcUpdate;
    struct PathFollower;

    // Determines the next state transition for a KeepDistance NPC based on current telemetry.
    // Manages a safe operating threshold, transitioning across states: Idle, Patrol, Chase, Shoot,
    // Bite, Retreat, or Watch.
    std::optional<NpcState> nextKeepDistanceState(NpcState state, const NpcFacts& facts);

    // Handles setup logic and initialization steps when a KeepDistance NPC enters a new state.
    // Allocates pathing demands, initializes timers, or shifts movement speeds depending on state
    // entry rules.
    void enterKeepDistanceState(Actor& actor, NpcState state);

    // Executes the frame-by-frame behavioral updates for the KeepDistance NPC's active state.
    // Processes backing away when a threat crosses proximity thresholds during Retreat, weapon
    // cycling during Shoot, or holding positioning during Watch.
    void updateKeepDistanceState(
        const NpcUpdate& update,
        Actor& actor,
        const NpcBrain& brain,
        PathFollower& follower,
        const Actor* target,
        NpcState state,
        float stateElapsed);
}
