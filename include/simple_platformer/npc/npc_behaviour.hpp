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

    /**
     * @brief Formulates an intentional tracking look vector heading towards a target location.
     * @note Writing to an actor's intentional aiming registry allows physics systems to safely
     * orient its horizontal facing later.
     * @param actor The underlying physical actor entity data structure.
     * @param targetFeet World pixel coordinates tracking the target's baseline footprint anchor
     * point.
     */
    void aimToward(Actor& actor, glm::vec2 targetFeet);

    /**
     * @brief Commands the actor to cycle its focal direction periodically across idle intervals.
     * @param actor The underlying physical actor entity data structure.
     * @param brain Cognitive state tracking internal timers, chosen tactics, and historical target
     * footprints.
     * @param stateElapsed Total continuous time in seconds spent executing the current AI state.
     */
    void lookAbout(Actor& actor, const NpcBrain& brain, float stateElapsed);

    /**
     * @brief Directs conditional transition rules to map out tactical state modifications.
     * @note This function operates deterministically based on facts and does not mutate actors or
     * dispatch commands directly.
     * @param tactic The governing strategy archetype tracking operational capabilities.
     * @param state The active AI state node monitored prior to validation checking.
     * @param facts Analyzed environmental parameters and context flags gathered this loop.
     * @return std::optional<NpcState> Target state modification node if triggered, or std::nullopt
     * to maintain active loops.
     */
    std::optional<NpcState> nextNpcState(NpcTactic tactic, NpcState state, const NpcFacts& facts);

    /**
     * @brief Triggers lifecycle hooks and memory resets whenever an NPC enters a newly selected
     * state.
     * @param actor The underlying physical actor entity data structure.
     * @param brain Cognitive state tracking internal timers, chosen tactics, and historical target
     * footprints.
     * @param follower Path navigation controller tracing waypoints and replaying inputs.
     * @param state The target state node context being initialized.
     */
    void enterNpcState(Actor& actor, NpcBrain& brain, PathFollower& follower, NpcState state);

    /**
     * @brief Computes mechanical input behaviors for an actor based on its active state loop
     * context.
     * @param update Aggregated world state properties, including delta clocks and pathfinders.
     * @param actor The underlying physical actor entity data structure to manipulate.
     * @param brain Cognitive state tracking internal timers, chosen tactics, and historical target
     * footprints.
     * @param follower Path navigation controller tracing waypoints and replaying inputs.
     * @param target Pointer targeting the live opponent being tracked, or nullptr if out of sensory
     * ranges.
     * @param state The active execution state being processed this frame tick.
     * @param stateElapsed Total continuous time in seconds spent executing the current AI state.
     */
    void updateNpcState(
        const NpcUpdate& update,
        Actor& actor,
        NpcBrain& brain,
        PathFollower& follower,
        const Actor* target,
        NpcState state,
        float stateElapsed);
}
