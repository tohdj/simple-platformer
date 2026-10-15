/**
 * @file        level_composition.hpp
 * @brief       Declares the GameLevel structure and engine factory functions for translating
 *              authored layout definitions into runtime map environments and active world entities.
 *
 * @author      Mohd Fadzuli
 * @author      Adapted by: Toh Da Jun
 * @copyright   Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

#include <glm/vec2.hpp>

#include "simple_platformer/world/tile_map.hpp"
#include "simple_platformer/world/world.hpp"

namespace simple_platformer
{
    struct Actor;
    struct LevelCatalog;
    struct GameCatalogs;

    // Aggregates active map structures, physics worlds, spawn locations, and entity
    // registry lookups allocated for a live game session level.
    struct GameLevel
    {
        int number = 0; // Sequential game level index code.
        TileMap map;    // Structural grid colliders and graphics terrain data.
        World world;    // Simulation space managing active physics component pipelines.
        glm::vec2 playerSpawnFeet = {
            0.0F,
            0.0F}; // Initial entry footing position coordinate for the player.
        std::unordered_map<std::uint32_t, std::string>
            actorDefinitionNames; // Registry maps runtime actor entity IDs back to authoring
                                  // template names.
    };

    // Deserializes data fields from a targeted level file index, matches tokens against master
    // catalog templates, and instantiates the physics world and map layer bounds.
    GameLevel composeGameLevel(
        const LevelCatalog& catalog,
        int levelNumber,
        int textureId,
        const GameCatalogs& catalogs);

    // Extracts player configuration data from master definitions and assembles a fresh
    // control-bound runtime Actor profile instance.
    Actor composePlayer(const GameCatalogs& catalogs, int textureId);
}
