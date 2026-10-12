/**
 * @file npc_behaviour.hpp
 * @brief Main engine subsystem coordinator for updating NPC decision routines and tactical
 * behaviors.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#pragma once

namespace simple_platformer
{
    class TileMap;
    class World;

    /**
     * @brief Orchestrates finite state transitions and input intention generation for all active
     * world NPCs.
     *
     * Iterates through simulated agents to process tactical cognitive loops, query internal
     * pathfinding graphs, and update command buffers based on frame context.
     *
     * @param map The static environment geometry grid layout context used to compute spatial path
     * nodes.
     * @param world The game world containing the mutable collection of simulated actors and path
     * tables.
     * @param deltaTime The discrete frame simulation duration step length in seconds.
     */
    void updateNpcBehaviour(const TileMap& map, World& world, float deltaTime);
}
