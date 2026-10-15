/**
 * @file charger.cpp
 * @brief Manage the FSM for charger NPC
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#include "simple_platformer/npc/charger.hpp"

#include <optional>

#include <glm/geometric.hpp>

#include "simple_platformer/actor/actor.hpp"
#include "simple_platformer/math/aabb.hpp"
#include "simple_platformer/movement/platformer_movement.hpp"
#include "simple_platformer/npc/npc.hpp"
#include "simple_platformer/npc/npc_facts.hpp"
#include "simple_platformer/npc/npc_senses.hpp"
#include "simple_platformer/npc/npc_behaviour.hpp"
#include "simple_platformer/npc/npc_update.hpp"

namespace simple_platformer
{
    namespace
    {
        // Cool-down duration tracking how many seconds an entity remains incapacitated after
        // striking walls
        constexpr float RecoverySeconds = 1.5F;

        // Evaluates whether a stunned entity can rotate its gaze towards its target based on
        // connection ranges
        bool canFaceTargetWhileStunned(const TileMap& map, const Actor& actor, const Actor* target)
        {
            if (target == nullptr)
            {
                return false;
            }

            // Allow tracking rotations only if both combatants occupy the exact same horizontal
            // platform runway
            return actor.platformerMovement.has_value() && actor.platformerMovement->grounded &&
                   target->platformerMovement.has_value() && target->platformerMovement->grounded &&
                   onSameGroundRun(map, actor.body.bounds, target->body.bounds) &&
                   actor.senses.has_value() &&
                   glm::distance(feetOf(actor.body.bounds), feetOf(target->body.bounds)) <=
                       actor.senses->noticeDistance;
        }
    }

    // Evaluates state transition tables for charger entities, converting obstacle contacts into
    // immediate stuns
    std::optional<NpcState> nextChargerState(NpcState state, const NpcFacts& facts)
    {
        const bool canCharge = facts.targetOnSameRun && facts.targetWithinNoticeDistance;
        switch (state)
        {
        case NpcState::Charge:
            // Intercepting a solid grid wall tile forces an automatic shift into the stunned
            // recovery cycle
            return facts.movementBlocked ? std::optional(NpcState::Stunned) : std::nullopt;

        case NpcState::Stunned:
            // Lock control registers completely until the recovery duration window has ticked down
            // fully
            if (facts.stateElapsed < RecoverySeconds)
            {
                return std::nullopt;
            }
            return canCharge ? NpcState::Charge
                             : NpcState::Sleep; // Instantly re-charge if target remains in range

        default:
            // Sleep and any supplied state from another tactic use the same wake
            // decision: Charge on an eligible landing, otherwise settle into Sleep.
            if (facts.heardLanding && canCharge)
            {
                return NpcState::Charge; // Wake up immediately and charge if an enemy landing sound
                                         // is registered nearby
            }
            return state == NpcState::Sleep ? std::nullopt : std::optional(NpcState::Sleep);
        }
    }

    // Handles initialization sequences whenever an entity completes a state transition pass
    void enterChargerState(Actor& actor, NpcBrain& brain, NpcState state)
    {
        if (state == NpcState::Charge)
        {
            // Establish an absolute horizontal locked trajectory towards the target's current
            // horizontal axis
            brain.chargeDirection =
                brain.lastKnownTargetFeet.x < feetOf(actor.body.bounds).x ? -1.0F : 1.0F;
        }
    }

    // Framework loop router dispatching frame updates directly to specialized charger behavioral
    // subroutines
    void updateChargerState(
        const NpcUpdate& update,
        Actor& actor,
        const NpcBrain& brain,
        const Actor* target,
        NpcState state)
    {
        switch (state)
        {
        case NpcState::Sleep:
            break; // Zero intentions output: remain perfectly motionless while resting

        case NpcState::Charge:
            // Inject a rigid, fast horizontal displacement vector and engage full collision contact
            // damage logic masks
            actor.intentions.direction = {brain.chargeDirection, 0.0F};
            actor.intentions.avoidLedges =
                true; // Clamp movements to prevent the entity from flying off ledges blindly
            actor.intentions.contactDamage =
                true; // Activate standard collision box spike/hurt properties
            break;

        case NpcState::Stunned:
            // Keep head orientation elements updated towards the target if visual connection paths
            // remain clear
            if (canFaceTargetWhileStunned(update.map, actor, target))
            {
                aimToward(actor, brain.lastKnownTargetFeet);
            }
            break;

        default:
            break;
        }
    }
}
