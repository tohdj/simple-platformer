/**
 * @file world_simulation.cpp
 * @brief Core simulation step executor for updating game world state and mapping constraints.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#include "simple_platformer/world/world_simulation.hpp"

#include "simple_platformer/actor/actor_lifecycle.hpp"
#include "simple_platformer/actor/actor_system.hpp"
#include "simple_platformer/combat/attack_system.hpp"
#include "simple_platformer/combat/projectile_system.hpp"
#include "simple_platformer/npc/npc_senses.hpp"
#include "simple_platformer/navigation/platformer_connection_table.hpp"
#include "simple_platformer/npc/npc_system.hpp"
#include "simple_platformer/world/level_exit.hpp"
#include "simple_platformer/world/pickup.hpp"
#include "simple_platformer/world/tile_map.hpp"
#include "simple_platformer/world/world.hpp"
#include "simple_platformer/world/world_requests.hpp"

namespace simple_platformer
{
    // The main execution engine loop that orchestrates global simulation updates,
    // physics handling, AI tracking, collision detection, and queued request execution.
    void updateWorldSimulation(TileMap& map, World& world, float deltaTime)
    {
        // 1. Freeze execution paths instantly if the stage objective metrics are achieved
        if (world.levelComplete())
        {
            return;
        }

        // 2. Clock update step: advance global timeline progression counters
        world.advanceSimulationTime(deltaTime);

        // While the exit opens the game is paused: only the clock runs, so the door can
        // finish opening.
        if (exitOpening(world))
        {
            updateLevelExit(world);
            return; // Skip gameplay updates while the transitional cutscene plays
        }

        // 3. Environment updates: A tile broken last step rebuilds pathfinding linkages
        // across the grid mesh node before any NPCs calculate movement paths.
        world.platformerConnections().applyRecordedTileBreaks(map);

        // 4. AI & Senses stage: Update sensory checks and process behavior state machines
        updateNpcSenses(map, world, deltaTime);
        updateNpcBehaviour(map, world, deltaTime);

        // 5. Kinematics processing: Simulate actor movement paths and floating pickup physics
        updateActorMovement(map, world, deltaTime);
        updatePickupMovement(map, world, deltaTime);

        // 6. Interaction processing: Open a request queue bucket for safe mutations
        WorldRequests requests;

        // 7. Combat updates: Process weapon strike fields, tracking nodes, and particle system
        // timings
        updateAttacks(world, requests, deltaTime);
        updateProjectiles(map, world, requests, deltaTime);
        updateProjectileBursts(world, requests, deltaTime);

        // 8. Actor cleanup checking: Manage respawns, health flags, and entity lifecycles
        updateActorLifecycle(world, requests, deltaTime);

        // 9. Loot detection cycle: Detect item collection overlaps after damage logic and
        // entity status updates occur, but safely before database structural lists change.
        updatePickups(world, requests);

        // 10. Mutate state parameters: Resolve all deferred requests collected this frame
        applyWorldRequests(world, requests);

        // 11. Exit condition check: Check if the player character entered the level exit bounds
        updateLevelExit(world);
    }
}
