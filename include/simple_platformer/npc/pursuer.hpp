/**
 * @file pursuer.hpp
 * @brief State machine transitions and frame updates for the hunting-focused Pursuer NPC archetype.
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

    // Determines the next state transition for a Pursuer NPC based on current telemetry.
    // Handles hunting behaviors and switches across states: Idle, Patrol, Chase, Shoot, Bite, or
    // Search.
    std::optional<NpcState> nextPursuerState(NpcState state, const NpcFacts& facts);

    // Handles setup logic and initialization steps when a Pursuer enters a new state.
    // Allocates pathing demands, initializes timers, or changes movement speeds depending on state
    // entry rules.
    void enterPursuerState(Actor& actor, NpcState state);

    // Executes the frame-by-frame behavioral updates for the Pursuer's active state.
    // Processes tracking vectors during Chase, weapons during Shoot, or last-known positions during
    // Search.
    void updatePursuerState(
        const NpcUpdate& update,
        Actor& actor,
        const NpcBrain& brain,
        PathFollower& follower,
        const Actor* target,
        NpcState state,
        float stateElapsed);
}
