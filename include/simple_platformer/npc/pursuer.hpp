/**
 * @file pursuer_state_machine.hpp
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

    /**
     * @brief Determines the next state transition for a Pursuer NPC based on current telemetry.
     *
     * The Pursuer archetype handles complex hunting behaviors and transitions across six primary
     * states: `Idle`, `Patrol`, `Chase`, `Shoot`, `Bite`, or `Search`.
     *
     * @param state The active AI state of the Pursuer during this evaluation frame.
     * @param facts Environmental and sensory metrics gathered about the NPC and its target.
     * @return std::optional<NpcState> The target state if a transition triggers, or std::nullopt to
     * maintain the current state.
     */
    std::optional<NpcState> nextPursuerState(NpcState state, const NpcFacts& facts);

    /**
     * @brief Handles setup logic and initialization steps when a Pursuer enters a new state.
     *
     * Allocates pathing demands, initializes combat wind-up timers, or locks/unlocks movement
     * speeds depending on the state entry requirements.
     *
     * @param actor The underlying physical actor entity data structure.
     * @param state The newly selected state configuration to initialize.
     */
    void enterPursuerState(Actor& actor, NpcState state);

    /**
     * @brief Executes the frame-by-frame behavioral updates for the Pursuer's active state.
     *
     * Processes context-driven routines such as direct vector tracking during `Chase`, weapon
     * cycling during `Shoot`, or navigating last-known target cell locations during `Search`.
     *
     * @param update Frame delta time and global world updates required for tracking logic.
     * @param actor The underlying physical actor entity data structure.
     * @param brain The cognitive state structure tracking internal timers and behavioral flags.
     * @param follower Component handling path node calculation and waypoint updates.
     * @param target A pointer to the targeted opponent (e.g., the Player) being hunted, or nullptr
     * if lost.
     * @param state The active runtime state context executing this loop tick.
     * @param stateElapsed Continuous elapsed time in seconds spent within this specific state.
     */
    void updatePursuerState(
        const NpcUpdate& update,
        Actor& actor,
        const NpcBrain& brain,
        PathFollower& follower,
        const Actor* target,
        NpcState state,
        float stateElapsed);
}
