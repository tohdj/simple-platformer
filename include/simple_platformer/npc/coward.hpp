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

    /**
     * @brief Determines the next state transition for a Coward NPC based on current metrics.
     *
     * The Coward archetype typically switches between four primary states: `Idle`, `Patrol`,
     * `Flee`, or `Bite` depending on proximity to threats or immediate combat advantages.
     *
     * @param state The active AI state of the Coward during this frame loop.
     * @param facts Environmental and sensory metrics gathered about the NPC.
     * @return std::optional<NpcState> The new state if a transition triggers, or std::nullopt to
     * maintain current state.
     */
    std::optional<NpcState> nextCowardState(NpcState state, const NpcFacts& facts);

    /**
     * @brief Handles setup logic and initialization steps when a Coward enters a new state.
     *
     * Resets transient variables, adjusts movement speeds, or flags animation states matching the
     * entry requirements.
     *
     * @param actor The underlying physical actor entity data structure.
     * @param state The newly selected state configuration to initialize.
     */
    void enterCowardState(Actor& actor, NpcState state);

    /**
     * @brief Executes the frame-by-frame behavioral logic for the Coward's active state.
     *
     * Processes ongoing actions such as updating path nodes during patrol, turning away from a
     * threat during a flee sequence, or winding up a close-quarters desperate attack.
     *
     * @param update Frame delta time and global updates required for tick adjustments.
     * @param actor The underlying physical actor entity data structure.
     * @param brain The cognitive state structure tracking AI parameters.
     * @param follower Component handling path node calculation and route progression.
     * @param target A pointer to the threat target (e.g., the Player) triggering reactions, or
     * nullptr if none exists.
     * @param state The active runtime state context executing this update loop.
     * @param stateElapsed Continuous time in seconds spent in the current state.
     */
    void updateCowardState(
        const NpcUpdate& update,
        Actor& actor,
        const NpcBrain& brain,
        PathFollower& follower,
        const Actor* target,
        NpcState state,
        float stateElapsed);
}
