/**
 * @file keep_distance_state_machine.cpp
 * @brief State machine transitions and frame updates for the defensive, ranged-focused KeepDistance
 * NPC archetype.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#include "simple_platformer/npc/keep_distance.hpp"

#include <optional>
#include <stdexcept>

#include <glm/geometric.hpp>
#include <glm/vec2.hpp>

#include "simple_platformer/actor/actor.hpp"
#include "simple_platformer/input/input_state.hpp"
#include "simple_platformer/math/aabb.hpp"
#include "simple_platformer/math/coordinates.hpp"
#include "simple_platformer/navigation/path_follower.hpp"
#include "simple_platformer/navigation/platformer_cells.hpp"
#include "simple_platformer/npc/npc.hpp"
#include "simple_platformer/npc/npc_facts.hpp"
#include "simple_platformer/npc/npc_navigation.hpp"
#include "simple_platformer/npc/npc_behaviour.hpp"
#include "simple_platformer/npc/npc_update.hpp"
#include "simple_platformer/world/tile_map.hpp"

namespace simple_platformer
{
    namespace
    {
        // Fallback routine selecting between an active patrol loop or a basic idle resting state
        NpcState patrolOrIdle(const NpcFacts& facts)
        {
            return facts.hasPatrol ? NpcState::Patrol : NpcState::Idle;
        }

        // Handles loss of target contact, fallback routing to tactical surveillance instead of
        // active search tracking
        NpcState lostTarget(const NpcFacts& facts)
        {
            if (!facts.searches)
            {
                return patrolOrIdle(facts);
            }
            return NpcState::Watch; // Revert to watching instead of chasing the last-known position
        }

        // Core tactical decision pipeline prioritizing zone management and spacing over direct
        // assault
        std::optional<NpcState> pursuit(const NpcFacts& facts)
        {
            if (!facts.targetKnown)
            {
                return std::nullopt;
            }
            if (facts.targetWithinStandoffDistance)
            {
                return NpcState::Retreat; // Priority 1: Maintain safe operational buffer threshold
                                          // spacing
            }
            if (facts.targetInBiteRange)
            {
                return NpcState::Bite; // Priority 2: Melee counter-strike if cornered
            }
            if (facts.targetInSights)
            {
                return NpcState::Shoot; // Priority 3: Ranged weapon cycle engagement option
            }
            return NpcState::Chase; // Priority 4: Spacing open; advance horizontally to establish
                                    // weapon range
        }

        // Pulls target layout coordinates matching the active destination waypoint node
        glm::vec2 patrolGoal(const Patrol& patrol)
        {
            return patrol.headingToSecond ? patrol.secondFeet : patrol.firstFeet;
        }

        // Steers the character back and forth across its predefined level waypoint map tracks
        void updatePatrolState(const NpcUpdate& update, Actor& actor, PathFollower& follower)
        {
            if (!actor.patrol.has_value())
            {
                throw std::logic_error("A patrolling NPC is missing its patrol");
            }
            Patrol& patrol = *actor.patrol;
            actor.intentions = intentionsToReach(update, actor, follower, patrolGoal(patrol));

            // Loop path logic: switch to the alternative waypoint once the current track completes
            if (pathComplete(follower))
            {
                patrol.headingToSecond = !patrol.headingToSecond;
                clearPath(follower);
            }
        }

        // Calculates path navigation updates to hunt down the target's last confirmed footprint
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
            aimToward(actor, brain.lastKnownTargetFeet);
        }

        // Processes weapon aim orientation tracks and fires attack registry triggers
        // The tactic leaves Shoot when sight is lost.
        void updateShootState(Actor& actor, const Actor* target)
        {
            if (target == nullptr)
            {
                throw std::logic_error("A shooting NPC has no target");
            }
            actor.intentions.aimDirection =
                centerOf(target->body.bounds) - centerOf(actor.body.bounds);
            actor.intentions.primaryAttackPressed = true;
        }

        // Guides the entity towards a ghost coordinate point, launching scanning sweeps if target
        // is unreachable Walk to the remembered position, then look about if there or unreachable.
        void updateSearchState(
            const NpcUpdate& update,
            Actor& actor,
            const NpcBrain& brain,
            PathFollower& follower,
            float stateElapsed)
        {
            actor.intentions =
                intentionsToReach(update, actor, follower, brain.lastKnownTargetFeet);
            if (!follower.path.has_value() || pathComplete(follower))
            {
                lookAbout(actor, brain, stateElapsed);
            }
        }

        // Steers the entity backwards to expand spacing while firing suppressing weapon attacks
        // Back away while aiming and attacking; walkers avoid unsupported cells.
        void updateRetreatState(const NpcUpdate& update, Actor& actor, const NpcBrain& brain)
        {
            const glm::vec2 feet = feetOf(actor.body.bounds);
            glm::vec2 away = feet - brain.lastKnownTargetFeet;

            // Platformer Safety Check: Prevent the character from walking off cliffs or into holes
            // while backing away blind
            if (actor.platformerMovement.has_value())
            {
                const float side = away.x < 0.0F ? -1.0F : 1.0F;
                away = {side, 0.0F};
                const Cell ahead = cellAtFeet(
                    update.map.tileSize(), feet + glm::vec2{side * actor.body.bounds.size.x, 0.0F});

                // Halt background movement instantly if the next row position is ungrounded or
                // blocked
                if (!canStandAt(update.map, ahead, actor.body.bounds.size))
                {
                    away = {0.0F, 0.0F};
                }
            }

            // Bind steering vectors, keep look focuses locked onto target, and hold attack inputs
            actor.intentions.direction = away;
            aimToward(actor, brain.lastKnownTargetFeet);
            actor.intentions.primaryAttackPressed = true;
        }
    }

    // Evaluates state transition tables for range-keeping archetypes, interpreting spacing metrics
    // to update behaviors
    std::optional<NpcState> nextKeepDistanceState(NpcState state, const NpcFacts& facts)
    {
        const std::optional<NpcState> pursuing = pursuit(facts);
        // Supplied states from other tactics reuse these decisions: Flee/Sleep/Charge/
        // Stunned use Idle, and Search uses Watch.
        switch (state)
        {
        case NpcState::Flee:
        case NpcState::Sleep:
        case NpcState::Charge:
        case NpcState::Stunned:
        case NpcState::Idle:
            if (pursuing.has_value())
            {
                return pursuing; // Switch to target tracking or defensive retreats instantly if a
                                 // threat is near
            }
            if (facts.hasPatrol)
            {
                return NpcState::Patrol; // Switch to standard patrolling routes if idle
                                         // configurations are clear
            }
            return std::nullopt;

        case NpcState::Patrol:
            return pursuing; // Maintain walking routes unless direct spacing or threat checks block
                             // the current loop

        case NpcState::Chase:
        case NpcState::Shoot:
        case NpcState::Retreat:
            if (!pursuing.has_value())
            {
                return lostTarget(facts); // Target dropped out of memory fields: fall back to watch
                                          // zones or patrols
            }
            return *pursuing != state ? pursuing
                                      : std::nullopt; // Route updates only if target distance
                                                      // thresholds shift priorities

        case NpcState::Search:
        case NpcState::Watch:
            if (pursuing.has_value())
            {
                return pursuing; // Re-engage target tracking instantly if scanning actions catch
                                 // visual focus signals
            }
            return facts.searchTimeUp ? std::optional(patrolOrIdle(facts))
                                      : std::nullopt; // Stand down to route defaults after
                                                      // observation timers expire

        case NpcState::Bite:
            // Ready on entry means combat has not seen the request yet. Wait for a later
            // decision before treating Ready as a finished bite. Chase before biting again.
            if (!facts.biteReady || facts.stateElapsed <= 0.0F)
            {
                return std::nullopt; // Keep actions locked in progress until the melee frame cycle
                                     // completes fully
            }
            return facts.targetKnown
                       ? NpcState::Chase
                       : lostTarget(
                             facts); // Transition out of close combat into a chase or search step
        }
        return std::nullopt;
    }

    // Handles initialization sequences whenever the entity updates its current active state setting
    void enterKeepDistanceState(Actor& actor, NpcState state)
    {
        if (state == NpcState::Bite)
        {
            actor.intentions.primaryAttackPressed =
                true; // Forcibly engage standard attack registration hooks upon lunging
        }
    }

    // Framework loop dispatcher directing frame execution loops to matching behavioral subroutines
    void updateKeepDistanceState(
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
            aimToward(actor, brain.lastKnownTargetFeet); // Lock rotation vector alignments onto the
                                                         // threat center point during strikes
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
        case NpcState::Retreat:
            updateRetreatState(update, actor, brain);
            break;
        default:
            break;
        }
    }

}
