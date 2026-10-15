/**
 * @file npc_state_machine.hpp
 * @brief Global finite state machine execution and intent dispatchers for NPC AI characters.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#pragma once

#include <optional>

#include <glm/vec2.hpp>

#include "simple_platformer/npc/npc.hpp"

namespace simple_platformer
{
    struct Actor;
    struct NpcFacts;
    struct NpcUpdate;
    struct PathFollower;

    // Formulates an intentional tracking look vector heading towards a target location.
    // Writing to an actor's intentional aiming registry allows physics systems to safely orient its
    // horizontal facing later.
    void aimToward(Actor& actor, glm::vec2 targetFeet);

    // Commands the actor to cycle its focal direction periodically across idle intervals.
    void lookAbout(Actor& actor, const NpcBrain& brain, float stateElapsed);

    // Directs conditional transition rules to map out tactical state modifications.
    // This function operates deterministically based on facts and does not mutate actors or
    // dispatch commands directly.
    std::optional<NpcState> nextNpcState(NpcTactic tactic, NpcState state, const NpcFacts& facts);

    // Triggers lifecycle hooks and memory resets whenever an NPC enters a newly selected state.
    void enterNpcState(Actor& actor, NpcBrain& brain, PathFollower& follower, NpcState state);

    // Computes mechanical input behaviors for an actor based on its active state loop context.
    // Generates specific target inputs, weapon signals, or lookaround updates based on the
    // evaluated state node.
    void updateNpcState(
        const NpcUpdate& update,
        Actor& actor,
        NpcBrain& brain,
        PathFollower& follower,
        const Actor* target,
        NpcState state,
        float stateElapsed);
}
