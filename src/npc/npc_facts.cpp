/**
 * @file npc_facts.cpp
 * @brief Telemetry aggregation and conditional fact parsing for NPC AI decision-making.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#include "simple_platformer/npc/npc_facts.hpp"

#include <glm/geometric.hpp>

#include "simple_platformer/actor/actor.hpp"
#include "simple_platformer/combat/attack_system.hpp"
#include "simple_platformer/combat/combat.hpp"
#include "simple_platformer/math/aabb.hpp"
#include "simple_platformer/npc/npc.hpp"
#include "simple_platformer/npc/npc_senses.hpp"
#include "simple_platformer/world/tile_map.hpp"

namespace simple_platformer
{
    namespace
    {
        // Performs a spatial intersection check to determine if the target character box overlaps
        // the active melee attack field
        bool targetIsInBiteRange(const Actor& actor, const Actor& target)
        {
            if (!actor.bite.has_value())
            {
                return false; // Actor lacks close-range combat data configurations
            }

            // Generate the current attack frame box boundary depending on direction, and trace
            // against target boundaries
            return overlaps(
                biteHitbox(actor.body.bounds, *actor.bite, actor.facing), target.body.bounds);
        }

        // Measures linear distances to the target's last remembered coordinate footprint to verify
        // standoff buffer boundaries
        bool targetIsWithinStandoffDistance(const Actor& actor, const NpcBrain& brain)
        {
            const float standoffDistance =
                actor.senses.has_value() ? actor.senses->standoffDistance : 0.0F;

            return glm::distance(feetOf(actor.body.bounds), brain.lastKnownTargetFeet) <
                   standoffDistance;
        }

        // Verifies if both combatant profiles occupy the same physical runway ledge, allowing
        // horizontal ground behaviors
        bool targetIsOnSameRun(const TileMap& map, const Actor& actor, const Actor& target)
        {
            // Ensure both actors are currently mapped to grounded platformer dynamics layers
            return actor.platformerMovement.has_value() && actor.platformerMovement->grounded &&
                   target.platformerMovement.has_value() && target.platformerMovement->grounded &&
                   onSameGroundRun(
                       map,
                       actor.body.bounds,
                       target.body.bounds); // Audit connected cell structures
        }

        // Compares basic raw position offsets against core sensing variables to verify threshold
        // presence parameters
        bool targetIsWithinNoticeDistance(const Actor& actor, const Actor& target)
        {
            return actor.senses.has_value() &&
                   glm::distance(feetOf(actor.body.bounds), feetOf(target.body.bounds)) <=
                       actor.senses->noticeDistance;
        }
    }

    // Aggregates component registers and telemetry data into a unified, flat boolean matrix to
    // guide AI transitions
    NpcFacts gatherNpcFacts(
        const TileMap& map,
        const Actor& actor,
        const NpcBrain& brain,
        const NpcPerception& perception,
        const Actor* target,
        float stateElapsed)
    {
        NpcFacts facts;

        // 1. Core Target Visibility and Memory Status Mapping
        facts.targetKnown = target != nullptr;
        facts.targetVisible = perception.targetVisible;

        // 2. Combat Proximity and Hitbox Validation Passes
        facts.targetInBiteRange =
            target != nullptr && perception.targetVisible && targetIsInBiteRange(actor, *target);
        facts.biteReady = actor.bite.has_value() && actor.bite->phase == BitePhase::Ready;
        facts.targetInSights =
            target != nullptr && perception.targetVisible && actor.rangedWeapon.has_value();

        // 3. Spacing, Acoustics, and Ledge Navigation Metrics Evaluation
        facts.targetWithinStandoffDistance =
            target != nullptr && targetIsWithinStandoffDistance(actor, brain);
        facts.heardLanding = perception.heardLanding;
        facts.targetOnSameRun = target != nullptr && targetIsOnSameRun(map, actor, *target);
        facts.targetWithinNoticeDistance =
            target != nullptr && targetIsWithinNoticeDistance(actor, *target);

        // 4. Locomotion Barriers and Path Context Mapping
        facts.movementBlocked =
            actor.platformerMovement.has_value() && actor.platformerMovement->blocked;
        facts.hasPatrol = actor.patrol.has_value();

        // 5. Investigative Search Timing and Temporal Counters Processing
        const float searchDuration = actor.senses.has_value() ? actor.senses->searchDuration : 0.0F;
        facts.searches = searchDuration > 0.0F;
        facts.searchTimeUp =
            stateElapsed >= searchDuration; // Check if the search window timer has expired
        facts.stateElapsed = stateElapsed;
        facts.targetLostElapsed = brain.targetLostElapsed;

        return facts;
    }
}
