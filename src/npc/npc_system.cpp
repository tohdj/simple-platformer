/**
 * @file npc_system.cpp
 * @brief Main engine subsystem coordinator for updating NPC decision routines and tactical
 * behaviors.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#include "simple_platformer/npc/npc_system.hpp"

#include <optional>
#include <stdexcept>

#include <glm/geometric.hpp>
#include <glm/vec2.hpp>

#include "simple_platformer/actor/actor.hpp"
#include "simple_platformer/math/validation.hpp"
#include "simple_platformer/navigation/path_follower.hpp"
#include "simple_platformer/npc/npc.hpp"
#include "simple_platformer/npc/npc_behaviour.hpp"
#include "simple_platformer/npc/npc_facts.hpp"
#include "simple_platformer/npc/npc_senses.hpp"
#include "simple_platformer/npc/npc_update.hpp"
#include "simple_platformer/world/tile_map.hpp"
#include "simple_platformer/world/world.hpp"

namespace simple_platformer
{
    namespace
    {
        // Evaluates state transition changes and updates the active tactical loop subroutine
        void updateTacticState(
            const NpcUpdate& update,
            Actor& actor,
            NpcBrain& brain,
            PathFollower& follower,
            const Actor* target,
            const NpcFacts& facts)
        {
            // Check the finite state machine transition table to see if conditions warrant a swap
            if (const std::optional<NpcState> next = nextNpcState(brain.tactic, brain.state, facts))
            {
                enterNpcState(actor, brain, follower, *next); // Execute state initialization hooks
            }

            // Dispatch the standard continuous frame update matching the active behavior state
            updateNpcState(update, actor, brain, follower, target, brain.state, brain.stateElapsed);
        }

        // Main agent orchestration pass: gathers sensory facts, queries state changes, and advances
        // cognitive clocks Which state comes next is decided once, from the facts, before the state
        // acts.
        void updateNpcDecision(const NpcUpdate& update, Actor& actor)
        {
            // Verify all mandatory cognitive, sensory, and navigation modules are safely bound
            if (!actor.brain.has_value() || !actor.perception.has_value() ||
                !actor.pathFollower.has_value())
            {
                throw std::logic_error("An NPC is missing behaviour components");
            }

            NpcBrain& brain = *actor.brain;
            const NpcPerception& perception = *actor.perception;
            PathFollower& follower = *actor.pathFollower;

            // Locate the alive opponent actor node referenced inside internal tracking metrics
            const Actor* target = livingTarget(update.world, brain);
            if (target != nullptr)
            {
                brain.targetLostElapsed =
                    0.0F; // Reset the amnesia tracking clock while target remains visible
            }

            // Compile global level metrics, layout nodes, and sensory parameters into an immutable
            // facts block
            const NpcFacts facts =
                gatherNpcFacts(update.map, actor, brain, perception, target, brain.stateElapsed);

            // Evaluate state priorities and fire execution logic ticks
            updateTacticState(update, actor, brain, follower, target, facts);

            // Tick forward the target loss timer if direct visual connection dropped this frame
            if (target == nullptr)
            {
                brain.targetLostElapsed += update.deltaTime;
            }
        }
    }

    // Iterates through all active entities to evaluate AI decision pipelines and update internal
    // state timers
    void updateNpcBehaviour(const TileMap& map, World& world, float deltaTime)
    {
        requireSeconds(deltaTime, "NPC behaviour time step");
        const NpcUpdate update{map, world, deltaTime};

        // 1. Scan through all spawned actors to locate active computer-controlled agents
        for (Actor& actor : world.actors())
        {
            if (!actor.brain.has_value())
            {
                continue; // Skip static elements, player avatars, or mindless entities
            }
            if (!actor.pathFollower.has_value())
            {
                throw std::logic_error("An NPC is missing its path follower");
            }

            // 2. Clear out lingering control flag inputs to build fresh commands for this tick
            actor.intentions = {};
            NpcBrain& brain = *actor.brain;

            // 3. Only process cognitive thinking loops if the character is actively alive
            if (actor.life == LifeState::Alive)
            {
                updateNpcDecision(
                    update, actor); // Run the transition table checks and path decisions
                brain.stateElapsed +=
                    deltaTime; // Track continuous residence runtime spent in the active state
            }
        }
    }
}
