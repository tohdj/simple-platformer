#pragma once

#include <optional>

#include "simple_platformer/npc/npc.hpp"

namespace simple_platformer
{
    struct Actor;
    struct NpcFacts;
    struct NpcUpdate;

    /**
     * @brief Determines the next logical state for a Charger NPC based on current telemetry.
     *
     * The Charger archetype typically cycles through three core states: `Sleep`, `Charge`,
     * or `Stunned`. This function serves as the transition evaluation matrix.
     *
     * @param state The active AI state of the Charger during this evaluation cycle.
     * @param facts Environmental and situational metrics gathered about the NPC and its
     * surroundings.
     * @return std::optional<NpcState> The target state if a transition is triggered, or
     * std::nullopt to maintain current state.
     */
    std::optional<NpcState> nextChargerState(NpcState state, const NpcFacts& facts);

    /**
     * @brief Handles the initialization logic required when a Charger transitions into a new state.
     *
     * Resets component values, modifies physics metrics (e.g., locking movement on stun),
     * and sets up state timers inside the brain components.
     *
     * @param actor The underlying physical game entity data structure.
     * @param brain The cognitive state structure tracking internal timers and states.
     * @param state The newly entered state configuration to apply.
     */
    void enterChargerState(Actor& actor, NpcBrain& brain, NpcState state);

    /**
     * @brief Processes the frame-by-frame update logic for the Charger's active state.
     *
     * Executes behavioral actions like pathing towards a target while charging, tracking sleep
     * timers, or decrementing stun durations.
     *
     * @param update Global Delta time and environment updates required for tick adjustments.
     * @param actor The underlying physical game entity data structure.
     * @param brain The cognitive state structure tracking internal timers and states.
     * @param target A pointer to the target actor (e.g., the Player) being pursued, or nullptr if
     * none exists.
     * @param state The current runtime state context executing this update loop.
     */
    void updateChargerState(
        const NpcUpdate& update,
        Actor& actor,
        const NpcBrain& brain,
        const Actor* target,
        NpcState state);
}
