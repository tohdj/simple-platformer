/**
 * @file charger.hpp
 * @brief Manage the FSM for charger NPC
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

    // Determines the next logical state for a Charger NPC based on current telemetry.
    // Handles hyper-aggressive charging mechanics and cycles across states: Sleep, Charge, or
    // Stunned.
    std::optional<NpcState> nextChargerState(NpcState state, const NpcFacts& facts);

    // Handles the initialization logic required when a Charger transitions into a new state.
    // Resets tracking values, overrides velocities, and re-arms internal timers within the brain
    // layout.
    void enterChargerState(Actor& actor, NpcBrain& brain, NpcState state);

    // Processes the frame-by-frame update logic for the Charger's active state.
    // Executes direct line-of-sight acceleration runs during Charge, tracks Sleep triggers, or
    // updates Stun relaxation clocks.
    void updateChargerState(
        const NpcUpdate& update,
        Actor& actor,
        const NpcBrain& brain,
        const Actor* target,
        NpcState state);
}
