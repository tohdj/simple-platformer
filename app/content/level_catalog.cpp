/**
 * @file        level_catalog.cpp
 * @brief       Declares the LevelCatalog structure and indexing functions for mapping, 
 *              loading, and ordering sequential level files in the game.
 *
 * @author      Mohd Fadzuli
 * @author      Adapted by: Toh Da Jun
 * @copyright   Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#include "level_catalog.hpp"
#include "content_diagnostics.hpp"
#include "content_json.hpp"
#include <initializer_list>

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>
#include <glm/vec2.hpp>

#include "simple_platformer/math/coordinates.hpp"
#include "simple_platformer/math/validation.hpp"

namespace simple_platformer
{
    namespace
    {
        // Alias naming short-token reference mapping to simplify nlohmann json object usage
        using Json = nlohmann::json;

        // Extracts an integer key parameter token and verifies that its value is strictly greater
        // than zero
        int jsonPositiveInteger(
            const Json& value,
            std::string_view sourceName,
            std::string_view path)
        {
            const int result = jsonInteger(value, sourceName, path);
            if (result <= 0)
            {
                failJson(sourceName, path, "level number must be positive");
            }
            return result;
        }

        // Sanitizes input file paths strings to mandate relative tracking vectors and prevent
        // illegal directory traversal attacks
        std::filesystem::path jsonRelativeFile(
            const Json& value,
            std::string_view sourceName,
            std::string_view path)
        {
            const std::filesystem::path file = jsonText(value, sourceName, path);

            // Enforce basic relative framework security rules bounds early
            if (file.empty() || file.is_absolute())
            {
                failJson(sourceName, path, "file must be a non-empty relative path");
            }

            // Sandbox protection: Parse directory segments explicitly to block parent directory
            // escape tokens ("..")
            for (const std::filesystem::path& part : file)
            {
                if (part == "..")
                {
                    failJson(sourceName, path, "file must stay inside the level directory");
                }
            }
            return file;
        }

        // Parses, structure-validates, and indexes complete level catalog arrays hierarchies out of
        // the schema root
        LevelCatalog jsonLevelCatalog(
            const Json& root,
            std::string_view sourceName,
            const std::filesystem::path& levelDirectory)
        {
            // Enforce explicit top-level block key rules mappings
            checkJsonFields(root, {"startLevel", "cameraDeadZone", "levels"}, sourceName, "root");

            LevelCatalog result;
            result.startLevel = jsonPositiveInteger(
                requiredJsonMember(root, "startLevel", sourceName, "root"),
                sourceName,
                "startLevel");
            result.cameraDeadZone = readVector(root, "cameraDeadZone", sourceName, "root");

            // Validate that camera dead-zone boundary configurations box fits securely inside
            // active rendering frame sizes
            if (!isFinitePositive(result.cameraDeadZone) ||
                result.cameraDeadZone.x > InternalViewportSize.x ||
                result.cameraDeadZone.y > InternalViewportSize.y)
            {
                failJson(
                    sourceName,
                    "cameraDeadZone",
                    "expected a positive size that fits in the " + std::to_string(InternalWidth) +
                        " by " + std::to_string(InternalHeight) + " view");
            }
            result.levelDirectory = levelDirectory;

            const Json& levels = requiredJsonMember(root, "levels", sourceName, "root");
            if (!levels.is_array() || levels.empty())
            {
                failJson(sourceName, "levels", "expected at least one level");
            }

            result.levels.reserve(
                levels
                    .size()); // Pre-allocate vector nodes storage to optimize array assembly passes

            // --- LOOP 1: PARSE CHRONOLOGICAL SCENE LEVEL ENTRIES AND CHECK SYSTEM UNIQUE INTEGRITY
            // ---
            for (std::size_t index = 0; index < levels.size(); ++index)
            {
                const Json& value = levels[index];
                const std::string path = indexPath("levels", index);
                checkJsonFields(value, {"number", "file"}, sourceName, path);

                LevelCatalogEntry entry;
                entry.number = jsonPositiveInteger(
                    requiredJsonMember(value, "number", sourceName, path),
                    sourceName,
                    fieldPath(path, "number"));
                entry.relativeFile = jsonRelativeFile(
                    requiredJsonMember(value, "file", sourceName, path),
                    sourceName,
                    fieldPath(path, "file"));

                // Prevent key identifier collisions by validating the level index entry number
                // hasn't been listed previously
                const auto duplicateNumber = std::find_if(
                    result.levels.begin(),
                    result.levels.end(),
                    [&entry](const LevelCatalogEntry& existing)
                    { return existing.number == entry.number; });

                if (duplicateNumber != result.levels.end())
                {
                    failJson(
                        sourceName, fieldPath(path, "number"), "level number is already listed");
                }
                result.levels.push_back(std::move(entry));
            }

            // --- STAGE 2: VALIDATE THE GAME LOOP INITIAL STARTING ENTRY HOOKS ---
            const auto start = std::find_if(
                result.levels.begin(),
                result.levels.end(),
                [&result](const LevelCatalogEntry& entry)
                { return entry.number == result.startLevel; });

            if (start == result.levels.end())
            {
                failJson(sourceName, "startLevel", "level is not listed in the catalog");
            }
            return result;
        }
    }

    // Parses raw text buffer streams into an engine-ready LevelCatalog blueprint structure
    LevelCatalog parseLevelCatalog(
        std::string_view text,
        std::string_view sourceName,
        const std::filesystem::path& levelDirectory)
    {
        // Initialize the low-level content root tokenizer layer using our abstract parsing engine
        const auto root = parseContentRoot(text, sourceName);
        try
        {
            // Hand off the root structural node to the core catalog generation loop compiler
            return jsonLevelCatalog(root, sourceName, levelDirectory);
        }
        // Intercept native structural framework errors to augment them with local filename metadata
        catch (const Json::exception& exception)
        {
            // Format a complete path diagnostic error log pass to simplify target script bug fixes
            failJson(sourceName, {}, std::string("invalid JSON: ") + exception.what());
        }
    }

    // Handles drive reading IO interactions and extracts parent directories to anchor relative
    // files paths
    LevelCatalog loadLevelCatalog(const std::filesystem::path& path)
    {
        // Pass file content strings down into parsing handlers, mapping the file's folder root as
        // the level directory base
        return parseLevelCatalog(loadContentText(path), path.string(), path.parent_path());
    }

    // Resolves an absolute operating system file path for a requested numerical scene target index
    std::filesystem::path levelPath(const LevelCatalog& catalog, int levelNumber)
    {
        // 1. Traverse catalog listings to locate an existing entry matching the designated sequence
        // index key
        const auto found = std::find_if(
            catalog.levels.begin(),
            catalog.levels.end(),
            [levelNumber](const LevelCatalogEntry& entry) { return entry.number == levelNumber; });

        // 2. Safeguard logic paths against runtime scene requests index boundary overflows
        if (found == catalog.levels.end())
        {
            throw std::invalid_argument(
                "Level " + std::to_string(levelNumber) + " is not in the catalog");
        }

        // Combine the stored base directory parent root with the matching relative level script
        // file name location Example Output Result: "assets/levels/" / "level_01.json" ->
        // "assets/levels/level_01.json"
        return catalog.levelDirectory / found->relativeFile;
    }
}
