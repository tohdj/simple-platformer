/**
 * @file        level_catalog.hpp
 * @brief       Declares the LevelCatalog structure and indexing functions for mapping,
 *              loading, and ordering sequential level files in the game.
 *
 * @author      Mohd Fadzuli
 * @author      Adapted by: Toh Da Jun
 * @copyright   Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#pragma once

#include <filesystem>
#include <string_view>
#include <vector>

#include <glm/vec2.hpp>

namespace simple_platformer
{
    struct LevelCatalogEntry
    {
        int number = 0;
        std::filesystem::path relativeFile;
    };

    struct LevelCatalog
    {
        int startLevel = 0;
        // The part of the view the player moves in before the camera follows, in internal
        // pixels. It fits inside the view.
        glm::vec2 cameraDeadZone = {0.0F, 0.0F};
        std::filesystem::path levelDirectory;
        std::vector<LevelCatalogEntry> levels;
    };

    LevelCatalog parseLevelCatalog(
        std::string_view text,
        std::string_view sourceName,
        const std::filesystem::path& levelDirectory = {});
    LevelCatalog loadLevelCatalog(const std::filesystem::path& path);
    std::filesystem::path levelPath(const LevelCatalog& catalog, int levelNumber);
}
