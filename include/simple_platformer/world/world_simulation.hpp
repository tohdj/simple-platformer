/**
 * @file world_simulation.hpp
 * @brief Core simulation step executor for updating game world state and mapping constraints.
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
     * @brief Runs a single gameplay step and resolves queued structural modifications.
     *
     * If the world state is completed, this function performs no actions. While the world's
     * exit sequence is open, only the simulation clock and exit verification routines are advanced.
     *
     * @param map The active tilemap defining the world boundaries and geometry.
     * @param world The persistent state container for the current world simulation.
     * @param deltaTime The elapsed time slice in seconds for the current frame step.
     */
    void updateWorldSimulation(TileMap& map, World& world, float deltaTime);
}
