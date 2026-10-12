/**
 * @file npc_update.hpp
 * @brief Context packaging structure aggregating environment and time data for NPC execution loops.
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
     * @brief Aggregated context data package shared across internal NPC state evaluation routines.
     */
    struct NpcUpdate
    {
        const TileMap& map; /**< Reference to the active static world tile mapping and geometry layout database. */
        World& world;       /**< Mutable reference to the parent simulation world tracking global entity arrays. */
        float deltaTime;    /**< The discrete frame simulation duration step slice measured in seconds. */
    };
}
