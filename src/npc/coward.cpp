/**
 * @file coward_state_machine.cpp
 * @brief State machine transitions and frame updates for the evasion-focused Coward NPC archetype.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#include "simple_platformer/npc/coward.hpp"

#include <optional>
#include <stdexcept>

#include <glm/geometric.hpp>
#include <glm/vec2.hpp>

#include "simple_platformer/actor/actor.hpp"
#include "simple_platformer/input/input_state.hpp"
#include "simple_platformer/math/aabb.hpp"
#include "simple_platformer/navigation/path_follower.hpp"
#include "simple_platformer/npc/npc.hpp"
#include "simple_platformer/npc/npc_facts.hpp"
#include "simple_platformer/npc/npc_navigation.hpp"
#include "simple_platformer/npc/npc_behaviour.hpp"
#include "simple_platformer/npc/npc_update.hpp"

namespace simple_platformer
{
    namespace
    {
        // Cool-down buffer duration tracking how long an entity remains in high-alert flight states
        constexpr float RecoverySeconds = 1.5F;

        // Fallback path router picking between an active route sequence loop or basic static
        // resting
        NpcState patrolOrIdle(const NpcFacts& facts)
        {
            return facts.hasPatrol ? NpcState::Patrol : NpcState::Idle;
        }

        // Steers the character layout towards the furthest edge of its route footprint layout to
        // escape threats
        void updateFleeState(
            const NpcUpdate& update,
            Actor& actor,
            const NpcBrain& brain,
            PathFollower& follower,
            const Actor* target)
        {
            // Abort flight track paths immediately if threat targets become invalid or route maps
            // drop out
            if (target == nullptr || !actor.patrol.has_value())
            {
                clearPath(follower);
                return;
            }

            const Patrol& patrol = *actor.patrol;

            // Derive absolute left and right extreme node bounds along the level patrol path runway
            const glm::vec2 left =
                patrol.firstFeet.x < patrol.secondFeet.x ? patrol.firstFeet : patrol.secondFeet;
            const glm::vec2 right =
                patrol.firstFeet.x < patrol.secondFeet.x ? patrol.secondFeet : patrol.firstFeet;
            const glm::vec2 feet = feetOf(actor.body.bounds);

            // Flee Direction Rule: pick the anchor checkpoint point located opposite to the target
            // threat vector
            const glm::vec2 refuge = feet.x < brain.lastKnownTargetFeet.x ? left : right;

            // Freeze movement and turn back to face tracking focus parameters once safety refuge is
            // reached
            if (glm::distance(feet, refuge) <= 1.0F)
            {
                clearPath(follower);
                aimToward(actor, brain.lastKnownTargetFeet);
                return;
            }

            // Dispatch navigation steering intentions towards the calculated escape destination
            // point
            actor.intentions = intentionsToReach(update, actor, follower, refuge);
        }

        // Pulls target layout coordinates matching the active destination waypoint node
        glm::vec2 patrolGoal(const Patrol& patrol)
        {
            return patrol.headingToSecond ? patrol.secondFeet : patrol.firstFeet;
        }

        // Standard movement routine advancing the character node along its patrol tracking nodes
        // list
        void updatePatrolState(const NpcUpdate& update, Actor& actor, PathFollower& follower)
        {
            if (!actor.patrol.has_value())
            {
                throw std::logic_error("A patrolling NPC is missing its patrol");
            }
            Patrol& patrol = *actor.patrol;
            actor.intentions = intentionsToReach(update, actor, follower, patrolGoal(patrol));

            // Loop navigation logic: toggle tracking parameters when goal cells are reached
            // successfully
            if (pathComplete(follower))
            {
                patrol.headingToSecond = !patrol.headingToSecond;
                clearPath(follower);
            }
        }
    }

    // Evaluates state transition tables for coward archetypes, prioritizing distance-keeping and
    // self-preservation
    std::optional<NpcState> nextCowardState(NpcState state, const NpcFacts& facts)
    {
        switch (state)
        {
        case NpcState::Flee:
            if (facts.targetInBiteRange)
            {
                return NpcState::Bite; // Desperation counter-strike: bite back if cornered inside
                                       // melee bounds
            }
            if (!facts.targetKnown && facts.targetLostElapsed >= RecoverySeconds)
            {
                return patrolOrIdle(
                    facts); // Calm down and stand down to defaults after escape window expires
            }
            return std::nullopt;

        case NpcState::Bite:
            if (!facts.biteReady || facts.stateElapsed <= 0.0F)
            {
                return std::nullopt; // Lock behavior in execution layer until the lunge frame cycle
                                     // completes
            }
            return facts.targetKnown
                       ? NpcState::Flee
                       : patrolOrIdle(facts); // Immediately resume flight paths after striking

        default:
            // Idle and Patrol share these decisions. Other tactics' states also come
            // here if supplied: flee from a nearby visible threat, otherwise return
            // to Patrol or Idle.
            if (facts.targetVisible && facts.targetWithinStandoffDistance)
            {
                return NpcState::Flee; // Priority alert trigger: initiate escape routes if threat
                                       // crosses distance buffers
            }
            if (state != patrolOrIdle(facts))
            {
                return patrolOrIdle(
                    facts); // Reframe state alignment routines to match ambient routes
            }
            return std::nullopt;
        }
    }

    // Handles tactical initialization logic structures when an entity completes a state update pass
    void enterCowardState(Actor& actor, NpcState state)
    {
        if (state == NpcState::Bite)
        {
            actor.intentions.primaryAttackPressed =
                true; // Signal an automatic fight tick response upon lunging
        }
    }

    // Framework loop router dispatching continuous frame update ticks to specialized coward
    // behavioral subroutines
    void updateCowardState(
        const NpcUpdate& update,
        Actor& actor,
        const NpcBrain& brain,
        PathFollower& follower,
        const Actor* target,
        NpcState state,
        float)
    {
        switch (state)
        {
        case NpcState::Patrol:
            updatePatrolState(update, actor, follower);
            break;
        case NpcState::Flee:
            updateFleeState(update, actor, brain, follower, target);
            break;
        case NpcState::Bite:
            aimToward(actor, brain.lastKnownTargetFeet); // Center visual rotation focuses onto the
                                                         // threat during strikes
            break;
        default:
            break;
        }
    }

}
