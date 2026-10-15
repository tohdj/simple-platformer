/**
 * @file npc_senses.hpp
 * @brief Sensory update systems, target life tracking, and ground run validation for NPC
 * characters.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#include "simple_platformer/npc/npc_senses.hpp"

#include <algorithm>
#include <optional>
#include <stdexcept>
#include <vector>

#include <glm/geometric.hpp>
#include <glm/vec2.hpp>

#include "simple_platformer/actor/actor.hpp"
#include "simple_platformer/actor/actor_id.hpp"
#include "simple_platformer/combat/combat.hpp"
#include "simple_platformer/math/aabb.hpp"
#include "simple_platformer/math/coordinates.hpp"
#include "simple_platformer/math/validation.hpp"
#include "simple_platformer/navigation/platformer_cells.hpp"
#include "simple_platformer/npc/npc.hpp"
#include "simple_platformer/world/sight.hpp"
#include "simple_platformer/world/tile_map.hpp"
#include "simple_platformer/world/world.hpp"

namespace simple_platformer
{
    namespace
    {
        // Aggregate helper struct grouping references to an agent's physical and sensory data
        // components
        struct SensingNpc
        {
            Actor& actor;
            NpcBrain& brain;
            NpcPerception& perception;
            const NpcSenses& senses;
        };

        // Evaluates whether a target's spatial midpoint falls within an NPC's configured detection
        // radius
        bool withinNoticeDistance(const Aabb& observer, const Aabb& target, const NpcSenses& senses)
        {
            if (!isFiniteNonNegative(senses.noticeDistance))
            {
                throw std::invalid_argument("NPC notice distance must be finite and non-negative");
            }

            // Perform a squared dot-product computation to completely avoid expensive square-root
            // calculations
            const glm::vec2 offset = centerOf(target) - centerOf(observer);
            return glm::dot(offset, offset) <= senses.noticeDistance * senses.noticeDistance;
        }

        // Checks if a target resides within notice limits and shares an unobstructed line-of-sight
        // vector path Within notice distance, with a line of sight between the two centres.
        bool canSeeTarget(
            const TileMap& map,
            const Aabb& observer,
            const Aabb& target,
            const NpcSenses& senses)
        {
            if (!withinNoticeDistance(observer, target, senses))
            {
                return false;
            }

            // Dispatch a formal parametric raycast to check for sight-blocking grid geometry
            return lineOfSight(map, centerOf(observer), centerOf(target));
        }

        // Commits target context information and spatial coordinates directly to cognitive state
        // variables
        void rememberTarget(NpcBrain& brain, const Actor& target, const NpcSenses& senses)
        {
            brain.target = target.id;
            brain.lastKnownTargetFeet = feetOf(target.body.bounds);
            brain.targetMemoryRemaining =
                senses.targetMemoryDuration; // Initialize retention countdown clock bounds
        }

        // Processes the frame sound event buffer queue, filtering acoustic signals that match the
        // active target
        bool hearNoises(
            const TileMap& map,
            SensingNpc& npc,
            const Actor& target,
            const std::vector<NoiseEvent>& noises)
        {
            bool heardNoise = false;
            for (const NoiseEvent& noise : noises)
            {
                if (noise.source != target.id)
                {
                    continue; // Skip environmental sound effects produced by irrelevant entities
                }
                const Aabb noiseBounds = boxStandingOn(noise.feet, target.body.bounds.size);
                if (!withinNoticeDistance(npc.actor.body.bounds, noiseBounds, npc.senses))
                {
                    continue; // Drop sound wave frequencies that dissipate prior to reaching the
                              // tracking threshold
                }

                // Shots travel through walls; landings are heard only on this run.
                if (noise.kind == NoiseKind::Landing)
                {
                    // Enforce structural contact checks so platform impacts only propagate along
                    // connected solid tile runways
                    if (!npc.actor.platformerMovement.has_value() ||
                        !npc.actor.platformerMovement->grounded ||
                        !onSameGroundRun(map, npc.actor.body.bounds, noiseBounds))
                    {
                        continue;
                    }
                    npc.perception.heardLanding =
                        true; // Set specialized tactile impact indicator flag
                }

                // Log acoustic event metadata and overwrite spatial tracking points with the sound
                // source coordinates
                heardNoise = true;
                rememberTarget(npc.brain, target, npc.senses);
                npc.brain.lastKnownTargetFeet = noise.feet;
            }
            return heardNoise;
        }

        // Visual tracking pass: checks raycast clarity and flags the target as explicitly visible
        // on success
        bool observeTarget(const TileMap& map, SensingNpc& npc, const Actor& target)
        {
            if (!canSeeTarget(map, npc.actor.body.bounds, target.body.bounds, npc.senses))
            {
                return false;
            }

            rememberTarget(npc.brain, target, npc.senses);
            npc.perception.targetVisible =
                true; // Engage active tracking and line-of-sight status indicators
            return true;
        }

        // Automatically steps down memory lifetimes, purging historical keys if target signatures
        // become invalid
        void decayTargetMemory(const World& world, NpcBrain& brain, float deltaTime)
        {
            // Wipe tracking entries instantly if target records drop out of the active data
            // container stack
            if (livingTarget(world, brain) == nullptr)
            {
                brain.target.reset();
                brain.targetMemoryRemaining = 0.0F;
                return;
            }

            // Countdown historical retention timers continuously until boundaries hit absolute zero
            // limits
            brain.targetMemoryRemaining = std::max(0.0F, brain.targetMemoryRemaining - deltaTime);
            if (brain.targetMemoryRemaining == 0.0F)
            {
                brain.target.reset(); // Erase expired focus target locks
            }
        }
    }

    // Evaluates whether two bounding regions share an unbroken, flat running surface on the same
    // row footprint
    bool onSameGroundRun(const TileMap& map, const Aabb& observer, const Aabb& target)
    {
        // Convert feet positions down to exact 2D tile cell grid indexes
        const Cell first = cellAtFeet(map.tileSize(), feetOf(observer));
        const Cell last = cellAtFeet(map.tileSize(), feetOf(target));

        // Break tracking instantly if their feet rest on completely separate vertical row heights
        if (first.y != last.y)
        {
            return false;
        }

        // Loop horizontally through all intermediate cells linking the two bodies together
        for (int x = std::min(first.x, last.x); x <= std::max(first.x, last.x); ++x)
        {
            if (!canStandAt(map, {x, first.y}, observer.size))
            {
                return false;
            }
        }
        return true;
    }

    // Locates the target actor tracked by the brain, returning nullptr if they are dead, missing,
    // or invalid.
    const Actor* livingTarget(const World& world, const NpcBrain& brain)
    {
        if (!brain.target.has_value())
        {
            return nullptr;
        }
        const Actor* target = world.findActor(*brain.target);
        return target != nullptr && target->life == LifeState::Alive ? target : nullptr;
    }

    // Main orchestration tick handling sensory snapshots, line-of-sight checks, and acoustic
    // listening for all active agents
    void updateNpcSenses(const TileMap& map, World& world, float deltaTime)
    {
        requireSeconds(deltaTime, "NPC senses time step");

        // Cache player references and gather the current frame's raw sound event buffer list
        const Actor* player = world.findActor(world.playerId());
        const std::vector<NoiseEvent> noises = world.takeNoises();

        // Iterate through the world database array to process sensory pipelines per automated agent
        for (Actor& actor : world.actors())
        {
            if (!actor.brain.has_value())
            {
                continue; // Skip mindless particles, non-agent projectiles, or player entities
            }
            if (!actor.senses.has_value() || !actor.perception.has_value())
            {
                throw std::logic_error("An NPC is missing its senses or perception");
            }

            NpcBrain& brain = *actor.brain;
            NpcPerception& perception = *actor.perception;

            // Wipe the transient tracking status metrics of the previous frame to compile clean
            // inputs
            perception = {};
            SensingNpc npc{actor, brain, perception, *actor.senses};

            // Evaluate status rules and relationship tables to confirm whether sensing metrics can
            // fire
            const bool livingPlayer = player != nullptr && player->life == LifeState::Alive;
            const bool sensesPlayer = actor.life == LifeState::Alive && livingPlayer &&
                                      areOpponents(actor.team, player->team);
            bool heardNoise = false;
            bool sawTarget = false;

            if (sensesPlayer)
            {
                // Hearing still records events when the target is visible; fresh sight
                // then takes priority over the last heard position.
                heardNoise = hearNoises(map, npc, *player, noises);
                sawTarget = observeTarget(map, npc, *player);
            }

            // Advance memory retention decay countdown clocks if no sensory feedback paths
            // refreshed this frame
            if (!heardNoise && !sawTarget)
            {
                decayTargetMemory(world, brain, deltaTime);
            }
        }
    }
}
