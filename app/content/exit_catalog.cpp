/**
 * @file        exit_catalog.cpp
 * @brief       Declares the ExitCatalog structures and functions for loading, 
 *              parsing, validating, and composing level exit definitions.
 *
 * @author      Mohd Fadzuli
 * @author      Adapted by: Toh Da Jun
 * @copyright   Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#include "exit_catalog.hpp"
#include "content_diagnostics.hpp"
#include "content_json.hpp"
#include "content_validation.hpp"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <string_view>
#include "simple_platformer/math/aabb.hpp"
#include "simple_platformer/render/sprite.hpp"
#include "simple_platformer/world/level_exit.hpp"

namespace simple_platformer
{
    /**
     * @brief Validates the logical properties of an individual exit configuration.
     *
     * @param definition The structural configuration of the level exit.
     */
    void validateExitDefinition(const ExitDefinition& definition)
    {
        LevelExit exit;
        exit.bounds.size = definition.bodySize;

        // Pass to deep engines for bounding box verification and sprite dimensions bounds check
        validateLevelExit(exit);
        validateContentSprite(definition.sprite);
    }

    /**
     * @brief Iterates over and validates every entry inside an parsed exit lookup catalog.
     *
     * @param catalog Map layout tracking exit keys alongside their core physical values.
     */
    void validateExitCatalog(const ExitCatalog& catalog)
    {
        for (const auto& entry : catalog)
        {
            try
            {
                if (entry.first.empty())
                {
                    throw std::invalid_argument("exit definition name cannot be empty");
                }
                validateExitDefinition(entry.second);
            }
            catch (const std::invalid_argument& error)
            {
                // Attaches the precise JSON tracking field path to the encountered logic failure
                failJson({}, fieldPath("exits", entry.first), error.what());
            }
        }
    }

    /**
     * @brief Parses a raw JSON data string to populate an in-memory game ExitCatalog lookup.
     *
     * @param text String slice displaying the target raw JSON content.
     * @param sourceName String reference pointing to the source file for debug contexts.
     * @return ExitCatalog The compiled catalog structure containing valid asset definitions.
     */
    ExitCatalog parseExitCatalog(std::string_view text, std::string_view sourceName)
    {
        // Construct and extract the root JSON block structure
        const auto root = parseContentRoot(text, sourceName);
        checkJsonFields(root, {"exits"}, sourceName, "root");
        const auto& definitions = requiredJsonMember(root, "exits", sourceName, "root");
        checkJsonObject(definitions, sourceName, "exits");

        ExitCatalog catalog;
        for (const auto& entry : definitions.items())
        {
            const std::string path = fieldPath("exits", entry.key());
            const auto& value = entry.value();

            // Strictly enforce the schema components for each level exit
            checkJsonFields(value, {"bodySize", "sprite"}, sourceName, path);

            ExitDefinition definition;
            definition.bodySize = readVector(value, "bodySize", sourceName, path);
            definition.sprite = jsonSprite(
                requiredJsonMember(value, "sprite", sourceName, path),
                sourceName,
                fieldPath(path, "sprite"));

            catalog.emplace(entry.key(), definition);
        }

        // Enforce comprehensive validation rules tied back to the associated input source tracking
        validateInFile(sourceName, [&] { validateExitCatalog(catalog); });
        return catalog;
    }

    /**
     * @brief Helper wrapper that reads a manifest text file path directly down into the catalog
     * parser.
     */
    ExitCatalog loadExitCatalog(const std::filesystem::path& path)
    {
        return parseExitCatalog(loadContentText(path), path.string());
    }

    /**
     * @brief Safe lookup utility that retrieves a named definition out of the catalog.
     *
     * @throws std::invalid_argument if the specified label mapping does not exist inside the
     * container.
     */
    const ExitDefinition& exitDefinition(const ExitCatalog& catalog, const std::string& name)
    {
        const auto found = catalog.find(name);
        if (found == catalog.end())
        {
            throw std::invalid_argument("unknown exit definition '" + name + "'");
        }
        return found->second;
    }

    /**
     * @brief Composes an instantiated runtime LevelExit entity deployed directly into a game world
     * layer.
     *
     * @param definition Blueprint archetype definitions tracking sprite dimensions and size
     * offsets.
     * @param textureId Active texture slot targeting the compiled texture atlas index.
     * @param spawnFeet Target 2D coordinates specifying the base floor midpoint anchor of the
     * object.
     * @return LevelExit Fully formed live runtime game instance.
     */
    LevelExit composeExit(const ExitDefinition& definition, int textureId, glm::vec2 spawnFeet)
    {
        validateExitDefinition(definition);

        LevelExit exit;
        // Construct an Axis-Aligned Bounding Box (AABB) using the object's bottom-center floor
        // anchor
        exit.bounds = boxStandingOn(spawnFeet, definition.bodySize);

        Sprite sprite = definition.sprite;
        sprite.textureId = textureId;
        exit.sprite = sprite;

        validateLevelExit(exit);
        return exit;
    }

    /**
     * @brief Verifies that all texture coordinate regions mapped inside the catalog sit strictly
     * inside bounds.
     *
     * @param catalog Current populated game configuration asset database context.
     * @param atlasSize Horizontal and vertical size parameters of the target packed texture image.
     * @param sourceName Source name reference mapping out diagnostics messages.
     */
    void validateExitAtlasRegions(
        const ExitCatalog& catalog,
        glm::ivec2 atlasSize,
        std::string_view sourceName)
    {
        for (const auto& [name, definition] : catalog)
        {
            requireInAtlas(
                definition.sprite.region,
                atlasSize,
                sourceName,
                fieldPath(fieldPath("exits", name), "sprite"));
        }
    }
}
