/**
 * @file npc_state_machine.cpp
 * @brief Global finite state machine execution and intent dispatchers for NPC AI characters.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#include "simple_platformer/npc/npc_behaviour.hpp"

#include <optional>

#include <glm/vec2.hpp>

#include "simple_platformer/actor/actor.hpp"
#include "simple_platformer/math/aabb.hpp"
#include "simple_platformer/navigation/path_follower.hpp"
#include "simple_platformer/npc/charger.hpp"
#include "simple_platformer/npc/coward.hpp"
#include "simple_platformer/npc/keep_distance.hpp"
#include "simple_platformer/npc/npc.hpp"
#include "simple_platformer/npc/npc_facts.hpp"
#include "simple_platformer/npc/npc_update.hpp"
#include "simple_platformer/npc/pursuer.hpp"

namespace simple_platformer
{
    namespace
    {
        // Continuous look-interval delay tracking how long an agent stares in one direction while
        // scanning
        constexpr float SearchTurnSeconds = 0.5F;
    }

    // Calculates a precise world-space targeting layout offset vector between the actor and its
    // objective
    void aimToward(Actor& actor, glm::vec2 targetFeet)
    {
        actor.intentions.aimDirection = targetFeet - feetOf(actor.body.bounds);
    }

    // Simulates an inquisitive left-and-right scanning cycle when searching for an elusive or
    // hidden player character
    void lookAbout(Actor& actor, const NpcBrain& brain, float stateElapsed)
    {
        // 1. Initialise the base orientation vector tracking toward the target's last recorded
        // footprint horizontal axis
        const float toward = brain.lastKnownTargetFeet.x - feetOf(actor.body.bounds).x;
        float side = toward < 0.0F ? -1.0F : 1.0F;

        // 2. Compute total elapsed interval counts to determine if the search cycle should
        // alternate directions
        const int turns = static_cast<int>(stateElapsed / SearchTurnSeconds);
        if (turns % 2 == 1)
        {
            side = -side; // Flip the horizontal visual direction mask on alternating steps
        }

        // 3. Bind the compiled scanning input direction directly into intent registries
        actor.intentions.aimDirection = {side, 0.0F};
    }

    // High-level decision matrix routing state updates down to specific behavioral tactics classes
    std::optional<NpcState> nextNpcState(NpcTactic tactic, NpcState state, const NpcFacts& facts)
    {
        switch (tactic)
        {
        case NpcTactic::Pursuer:
            return nextPursuerState(state, facts);
        case NpcTactic::KeepDistance:
            return nextKeepDistanceState(state, facts);
        case NpcTactic::Coward:
            return nextCowardState(state, facts);
        case NpcTactic::Charger:
            return nextChargerState(state, facts);
        }
        return std::nullopt;
    }

    // Lifecycle initialization hook handling boundary resets, clock overrides, and path clearing
    // upon a state change
    void enterNpcState(Actor& actor, NpcBrain& brain, PathFollower& follower, NpcState state)
    {
        // Reset chronological tracks and wipe old waypoint parameters to prevent path spillover
        // corruption
        brain.state = state;
        brain.stateElapsed = 0.0F;
        clearPath(follower);

        // Dispatch custom initialization routines matching the character's core tactical identity
        switch (brain.tactic)
        {
        case NpcTactic::Pursuer:
            enterPursuerState(actor, state);
            break;
        case NpcTactic::KeepDistance:
            enterKeepDistanceState(actor, state);
            break;
        case NpcTactic::Coward:
            enterCowardState(actor, state);
            break;
        case NpcTactic::Charger:
            enterChargerState(actor, brain, state);
            break;
        }
    }

    // Framework loop router directing consecutive frame commands to specialized tactical update
    // methods
    void updateNpcState(
        const NpcUpdate& update,
        Actor& actor,
        NpcBrain& brain,
        PathFollower& follower,
        const Actor* target,
        NpcState state,
        float stateElapsed)
    {
        switch (brain.tactic)
        {
        case NpcTactic::Pursuer:
            updatePursuerState(update, actor, brain, follower, target, state, stateElapsed);
            break;
        case NpcTactic::KeepDistance:
            updateKeepDistanceState(update, actor, brain, follower, target, state, stateElapsed);
            break;
        case NpcTactic::Coward:
            updateCowardState(update, actor, brain, follower, target, state, stateElapsed);
            break;
        case NpcTactic::Charger:
            updateChargerState(update, actor, brain, target, state);
            break;
        }
    }
}
