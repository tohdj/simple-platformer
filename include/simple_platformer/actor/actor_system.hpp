/**
 * @file        actor_system.hpp
 * @brief       Declares environmental update functions responsible for moving actors and resolving
 * geometric tile collisions.
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright   Copyright (c) 2026 Simple Platformer. All rights reserved.
 */

#pragma once

namespace simple_platformer
{
    class TileMap;
    class World;

    // Updates the positions and velocities of all active actors in the world.
    // Processes actor physics and environmental interactions by moving bodies, resolving solid tile
    // collisions, and updating internal grounding flags or velocity dampening components.
    void updateActorMovement(const TileMap& map, World& world, float deltaTime);
}
