/**
 * @file        actor_lifecycle.cpp
 * @brief       Declares functions and constants responsible for handling damage processing, death
 * timers, and lifecycle updates.
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright   Copyright (c) 2026 Simple Platformer. All rights reserved.
 */

#include "simple_platformer/actor/actor_lifecycle.hpp"

#include <algorithm>
#include <vector>

#include "simple_platformer/actor/actor.hpp"
#include "simple_platformer/actor/actor_id.hpp"
#include "simple_platformer/math/validation.hpp"
#include "simple_platformer/world/world.hpp"
#include "simple_platformer/world/world_requests.hpp"

namespace simple_platformer
{
    // Resolves pending damage, updates death timers, handles player respawns, and schedules dead
    // NPC cleanups
    void updateActorLifecycle(World& world, WorldRequests& requests, float deltaTime)
    {
        // Enforce simulation time step validity parameters
        requireSeconds(deltaTime, "Actor lifecycle time step");

        // Snapshot before applying damage so a newly dying actor keeps its full timer.
        std::vector<ActorId> actorsAlreadyDying;
        for (const Actor& actor : world.actors())
        {
            if (actor.life == LifeState::Dying)
            {
                actorsAlreadyDying.push_back(actor.id);
            }
        }

        // Step 1: Process the queue of inbound character hit damage requests
        for (const auto& request : requests.damageRequests)
        {
            Actor* actor = world.findActor(request.target);

            // Ignore invalid targets, already dead units, or objects without active health pools
            if (actor == nullptr || actor->life != LifeState::Alive || !actor->health.has_value())
            {
                continue;
            }

            // Deduct health points down to a floor bound of zero
            actor->health->current = std::max(0, actor->health->current - request.amount);
            actor->lastDamageTimeSeconds = world.simulationTimeSeconds();

            // Check for death thresholds and trigger the transitional dying phase
            if (actor->health->current == 0)
            {
                actor->life = LifeState::Dying;
                actor->deathTimeRemaining =
                    ActorDeathSeconds;  // Lock down unit for the animation delay window
                actor->intentions = {}; // Wipe current steering inputs to freeze locomotion
            }
        }
        requests.damageRequests.clear(); // Clear the processed requests

        // Step 2: Tick decay timers for entities that entered the frame already dead
        for (const ActorId id : actorsAlreadyDying)
        {
            Actor* actor = world.findActor(id);
            if (actor == nullptr)
            {
                continue;
            }

            // Advance the countdown timer tracking removal phases
            actor->deathTimeRemaining = std::max(0.0F, actor->deathTimeRemaining - deltaTime);
            if (actor->deathTimeRemaining > 0.0F)
            {
                continue; // Wait out the remaining animation block duration
            }

            // Step 3: Handle structural cleanup or replacement scenarios for completed timelines
            if (id == world.playerId())
            {
                world.respawnPlayer(); // Instantly reconstruct player data fields at a spawn target
                                       // node
            }
            else
            {
                requests.remove(id); // Queue up an entity erasure request command for standard NPCs
            }
        }
    }
}
