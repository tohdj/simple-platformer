/**
 * @file pursuer.cpp
 * @brief State machine transitions and frame updates for the hunting-focused Pursuer NPC archetype.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#include "simple_platformer/npc/pursuer.hpp"

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
        // Fallback picker routing an entity to active route looping or basic static resting
        NpcState patrolOrIdle(const NpcFacts& facts)
        {
            return facts.hasPatrol ? NpcState::Patrol : NpcState::Idle;
        }

        // Handles target evasion parameters, choosing whether to search last known locations or
        // stand down
        NpcState lostTarget(const NpcFacts& facts)
        {
            if (!facts.searches)
            {
                return patrolOrIdle(
                    facts); // Stand down immediately if search traits are not attached
            }
            return NpcState::Search; // Enter investigation loop patterns
        }

        // High-level target engagement pipeline evaluating distances to switch combat tactics
        std::optional<NpcState> pursuit(const NpcFacts& facts)
        {
            if (!facts.targetKnown)
            {
                return std::nullopt; // Maintain default states if target trajectory vectors are
                                     // completely blank
            }
            if (facts.targetInBiteRange)
            {
                return NpcState::Bite; // Melee proximity override triggered
            }
            if (facts.targetInSights)
            {
                return NpcState::Shoot; // Line-of-sight range option ready
            }
            return NpcState::Chase; // Target out of reach: maintain direct chase velocity tracking
        }

        // Pulls the targeted coordinate pair matching the current waypoint loop direction
        glm::vec2 patrolGoal(const Patrol& patrol)
        {
            return patrol.headingToSecond ? patrol.secondFeet : patrol.firstFeet;
        }

        // Drives the entity down a back-and-forth waypoint path grid loop layout
        void updatePatrolState(const NpcUpdate& update, Actor& actor, PathFollower& follower)
        {
            if (!actor.patrol.has_value())
            {
                throw std::logic_error("A patrolling NPC is missing its patrol");
            }
            Patrol& patrol = *actor.patrol;

            // Calculate intention path direction grids to chase the primary node goal
            actor.intentions = intentionsToReach(update, actor, follower, patrolGoal(patrol));

            // Ping-pong path logic: reverse route tracking once current destination is reached
            if (pathComplete(follower))
            {
                patrol.headingToSecond = !patrol.headingToSecond;
                clearPath(follower); // Drop stale paths to allow recalculations on next cycle
            }
        }

        // Resolves travel path calculations toward the last verified location footprint coordinates
        // of an opponent
        void updateChaseState(
            const NpcUpdate& update,
            Actor& actor,
            const NpcBrain& brain,
            PathFollower& follower,
            const Actor* target)
        {
            if (target == nullptr)
            {
                throw std::logic_error("A chasing NPC has no target");
            }

            actor.intentions =
                intentionsToReach(update, actor, follower, brain.lastKnownTargetFeet);
            aimToward(
                actor,
                brain.lastKnownTargetFeet); // Ensure face components look toward target center maps
        }

        // Processes alignment corrections and maps attack signals during target lock-on windows
        // The tactic leaves Shoot when sight is lost.
        void updateShootState(Actor& actor, const Actor* target)
        {
            if (target == nullptr)
            {
                throw std::logic_error("A shooting NPC has no target");
            }

            // Derive absolute orientation vectors between the source and targeted bounding centers
            actor.intentions.aimDirection =
                centerOf(target->body.bounds) - centerOf(actor.body.bounds);
            actor.intentions.primaryAttackPressed = true; // Engage weapon fire component loops
        }

        // Directs the actor toward a ghost tracking cell, firing scanning routines if the
        // destination is dead or blocked Walk to the remembered position, then look about if there
        // or unreachable.
        void updateSearchState(
            const NpcUpdate& update,
            Actor& actor,
            const NpcBrain& brain,
            PathFollower& follower,
            float stateElapsed)
        {
            actor.intentions =
                intentionsToReach(update, actor, follower, brain.lastKnownTargetFeet);

            // Initialize investigative rotations if path calculations drop or completion limits
            // fire
            if (!follower.path.has_value() || pathComplete(follower))
            {
                lookAbout(actor, brain, stateElapsed);
            }
        }
    }

    // Evaluates state transition tables for pursuing archetypes, interpreting incoming facts to
    // trigger action changes
    std::optional<NpcState> nextPursuerState(NpcState state, const NpcFacts& facts)
    {
        const std::optional<NpcState> pursuing = pursuit(facts);
        // Supplied states from other tactics reuse these decisions: Flee/Sleep/Charge/
        // Stunned use Idle, Retreat uses Chase/Shoot, and Watch uses Search.
        switch (state)
        {
        case NpcState::Flee:
        case NpcState::Sleep:
        case NpcState::Charge:
        case NpcState::Stunned:
        case NpcState::Idle:
            if (pursuing.has_value())
            {
                return pursuing; // Switch to target tracking instantly if a threat is located
            }
            if (facts.hasPatrol)
            {
                return NpcState::Patrol; // Switch to walking routes if idle space maps are clear
            }
            return std::nullopt;

        case NpcState::Patrol:
            return pursuing; // Keep patrolling unless direct target engagement blocks the current
                             // track loop

        case NpcState::Chase:
        case NpcState::Shoot:
        case NpcState::Retreat:
            if (!pursuing.has_value())
            {
                return lostTarget(
                    facts); // Target connection lost: drop back to investigation or baseline paths
            }
            return *pursuing != state
                       ? pursuing
                       : std::nullopt; // Route changes only if tactical options swap priorities

        case NpcState::Search:
        case NpcState::Watch:
            if (pursuing.has_value())
            {
                return pursuing; // Re-engage target instantly if search patterns find visual cues
            }
            return facts.searchTimeUp
                       ? std::optional(patrolOrIdle(facts))
                       : std::nullopt; // Return to route baseline after search clock ticks down

        case NpcState::Bite:
            // Ready on entry means combat has not seen the request yet. Wait for a later
            // decision before treating Ready as a finished bite. Chase before biting again.
            if (!facts.biteReady || facts.stateElapsed <= 0.0F)
            {
                return std::nullopt; // Lock behavior in progress until the attack animation
                                     // completes completely
            }
            return facts.targetKnown ? NpcState::Chase
                                     : lostTarget(facts); // Reset spacing variables by entering a
                                                          // chase loop after striking
        }
        return std::nullopt;
    }

    // Handles initialization logic sequences when an entity commits to a state switch boundaries
    // map
    void enterPursuerState(Actor& actor, NpcState state)
    {
        if (state == NpcState::Bite)
        {
            actor.intentions.primaryAttackPressed =
                true; // Involuntarily signal an immediate strike request upon lunging
        }
    }

    // Routes frame-by-frame updates to specialized logical subroutines matching the current state
    void updatePursuerState(
        const NpcUpdate& update,
        Actor& actor,
        const NpcBrain& brain,
        PathFollower& follower,
        const Actor* target,
        NpcState state,
        float stateElapsed)
    {
        switch (state)
        {
        case NpcState::Patrol:
            updatePatrolState(update, actor, follower);
            break;
        case NpcState::Chase:
            updateChaseState(update, actor, brain, follower, target);
            break;
        case NpcState::Bite:
            aimToward(
                actor,
                brain.lastKnownTargetFeet); // Continuous orientation adjustments during lunges
            break;
        case NpcState::Shoot:
            updateShootState(actor, target);
            break;
        case NpcState::Search:
            updateSearchState(update, actor, brain, follower, stateElapsed);
            break;
        case NpcState::Watch:
            lookAbout(actor, brain, stateElapsed);
            break;
        default:
            break;
        }
    }
}
