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

    /**
     * @brief Determines the next state transition for a KeepDistance NPC based on current
     * telemetry.
     *
     * The KeepDistance archetype maintains a safe operating threshold, transitioning across seven
     * primary states: `Idle`, `Patrol`, `Chase`, `Shoot`, `Bite`, `Retreat`, or `Watch`.
     *
     * @param state The active AI state of the NPC during this evaluation frame.
     * @param facts Environmental and sensory metrics gathered about the NPC and its target.
     * @return std::optional<NpcState> The target state if a transition triggers, or std::nullopt to
     * maintain the current state.
     */
    std::optional<NpcState> nextKeepDistanceState(NpcState state, const NpcFacts& facts);

    /**
     * @brief Handles setup logic and initialization steps when a KeepDistance NPC enters a new
     * state.
     *
     * Allocates pathing demands, initializes combat wind-up timers, or locks/unlocks movement
     * speeds depending on the state entry requirements.
     *
     * @param actor The underlying physical actor entity data structure.
     * @param state The newly selected state configuration to initialize.
     */
    void enterKeepDistanceState(Actor& actor, NpcState state);

    /**
     * @brief Executes the frame-by-frame behavioral updates for the KeepDistance NPC's active
     * state.
     *
     * Processes context-driven routines such as backing away when a threat crosses proximity
     * thresholds during `Retreat`, weapon cycling during `Shoot`, or holding positioning while
     * observing during `Watch`.
     *
     * @param update Frame delta time and global world updates required for tracking logic.
     * @param actor The underlying physical actor entity data structure.
     * @param brain The cognitive state structure tracking internal timers and behavioral flags.
     * @param follower Component handling path node calculation and waypoint updates.
     * @param target A pointer to the targeted opponent (e.g., the Player) being managed, or nullptr
     * if lost.
     * @param state The active runtime state context executing this loop tick.
     * @param stateElapsed Continuous elapsed time in seconds spent within this specific state.
     */
    void updateKeepDistanceState(
        const NpcUpdate& update,
        Actor& actor,
        const NpcBrain& brain,
        PathFollower& follower,
        const Actor* target,
        NpcState state,
        float stateElapsed);
}
