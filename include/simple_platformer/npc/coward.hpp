/**
 * @file coward_state_machine.hpp
 * @brief State machine transitions and frame updates for the evasion-focused Coward NPC archetype.
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

    // Determines the next state transition for a Coward NPC based on current metrics.
    // Handles safety-first behaviors and switches across states: Idle, Patrol, Flee, or Bite.
    std::optional<NpcState> nextCowardState(NpcState state, const NpcFacts& facts);

    // Handles setup logic and initialization steps when a Coward enters a new state.
    // Resets tracking variables, changes speeds, or sets animation states matching the entry rules.
    void enterCowardState(Actor& actor, NpcState state);

    // Executes the frame-by-frame behavioral logic for the Coward's active state.
    // Processes path nodes during patrol, calculates escape vectors during Flee, or triggers
    // close-quarters attacks.
    void updateCowardState(
        const NpcUpdate& update,
        Actor& actor,
        const NpcBrain& brain,
        PathFollower& follower,
        const Actor* target,
        NpcState state,
        float stateElapsed);
}
