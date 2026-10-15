/**
 * @file        pickup_catalog.cpp
 * @brief       Declares the PickupCatalog structures and functions for loading, 
 *              parsing, validating, and composing in-world collectible item definitions.
 *
 * @author      Mohd Fadzuli
 * @author      Adapted by: Toh Da Jun
 * @copyright   Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#include "pickup_catalog.hpp"
#include "content_diagnostics.hpp"
#include "content_json.hpp"
#include "content_validation.hpp"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <stdexcept>
#include "content/item_catalog.hpp"
#include <string_view>
#include <string>
#include "simple_platformer/math/aabb.hpp"
#include "simple_platformer/world/pickup.hpp"

namespace simple_platformer
{
    // Validates a pickup blueprint definition against core engine physics and active item database
    // limits
    void validatePickupDefinition(const PickupDefinition& definition, const ItemCatalog& items)
    {
        Pickup pickup;
        // Apply blueprint dimensions and convert string item keys into structured memory stacks
        pickup.body.bounds.size = definition.bodySize;
        pickup.stack = composeItemStack(items, definition.stack);
        pickup.sprite = definition.sprite;

        // Pass to the low-level engine validator to check for non-infinite bounds or negative
        // counts
        validatePickup(pickup);
        if (definition.sprite)
        {
            validateContentSprite(*definition.sprite);
        }
    }

    // Iterates through an entire catalog hash map to validate every loaded pickup asset profile
    // entry
    void validatePickupCatalog(const PickupCatalog& catalog, const ItemCatalog& items)
    {
        for (const auto& entry : catalog)
        {
            try
            {
                // Ensure the map dictionary key string identifier isn't blank
                if (entry.first.empty())
                {
                    throw std::invalid_argument("pickup name cannot be empty");
                }
                validatePickupDefinition(entry.second, items);
            }
            catch (const std::invalid_argument& error)
            {
                // Route error logs to a formatted JSON tracker path specifying the broken
                // dictionary key node
                failJson({}, fieldPath("pickups", entry.first), error.what());
            }
        }
    }

    // Parses a raw string text configuration structure down into an operational binary catalog
    // collection matrix
    PickupCatalog parsePickupCatalog(
        std::string_view text,
        std::string_view sourceName,
        const ItemCatalog& items)
    {
        // 1. Initialize structural JSON token parses out of the stream
        const auto root = parseContentRoot(text, sourceName);
        checkJsonFields(root, {"pickups"}, sourceName, "root");
        const auto& definitions = requiredJsonMember(root, "pickups", sourceName, "root");
        checkJsonObject(definitions, sourceName, "pickups");

        PickupCatalog catalog;

        // 2. Loop through all sub-node keys inside the root object tree configurations
        for (const auto& entry : definitions.items())
        {
            const std::string path = fieldPath("pickups", entry.key());
            const auto& value = entry.value();

            // Enforce explicit mandatory schema requirements parameters for every index entry token
            checkJsonFields(value, {"item", "quantity", "bodySize", "sprite"}, sourceName, path);

            PickupDefinition definition;
            // Map the parsed variables into flat memory storage properties containers
            definition.stack = {
                readText(value, "item", sourceName, path),
                readInteger(value, "quantity", sourceName, path)};
            definition.bodySize = readVector(value, "bodySize", sourceName, path);
            readOptionalSprite(value, "sprite", definition.sprite, sourceName, path);

            catalog.emplace(entry.key(), definition);
        }

        // 3. Fire complete content verification validation routines before returning output
        // definitions
        validateInFile(sourceName, [&] { validatePickupCatalog(catalog, items); });
        return catalog;
    }

    // Handles drive reading IO operations and directs content text directly downward to text
    // parsing units
    PickupCatalog loadPickupCatalog(const std::filesystem::path& path, const ItemCatalog& items)
    {
        return parsePickupCatalog(loadContentText(path), path.string(), items);
    }

    // Safe bounds-checked map search lookup interface to extract blueprint references matching a
    // key query name
    const PickupDefinition& pickupDefinition(const PickupCatalog& catalog, const std::string& name)
    {
        const auto found = catalog.find(name);
        if (found == catalog.end())
        {
            throw std::invalid_argument("unknown pickup definition '" + name + "'");
        }
        return found->second;
    }

    // Instantiation factory that transforms a flat catalog profile map definition into an active
    // physics world entity item
    Pickup composePickup(
        const PickupDefinition& definition,
        const ItemCatalog& items,
        int textureId,
        glm::vec2 spawnFeet)
    {
        validatePickupDefinition(definition, items);

        Pickup result;
        // Construct an Axis-Aligned Bounding Box (AABB) centered directly over the targeted spawn
        // anchor base floor placement
        result.body.bounds = boxStandingOn(spawnFeet, definition.bodySize);
        result.stack = composeItemStack(items, definition.stack);
        result.sprite = definition.sprite;

        if (result.sprite)
        {
            // Link the specific active global texture sheet layer index handle onto the rendering
            // profile instance
            result.sprite->textureId = textureId;
        }

        // Conclude setup steps by applying post-construction validation data verification loops
        validatePickup(result);
        return result;
    }

    // Validates sprite boundary dimensions criteria to ensure bounding coordinates sit securely
    // inside texture sheet limits
    void validatePickupAtlasRegions(
        const PickupCatalog& catalog,
        glm::ivec2 atlasSize,
        std::string_view sourceName)
    {
        for (const auto& [name, definition] : catalog)
        {
            // Check texture coordinate values against physical texture limits if a graphic asset is
            // bound
            if (definition.sprite.has_value())
            {
                requireInAtlas(
                    definition.sprite->region,
                    atlasSize,
                    sourceName,
                    fieldPath(fieldPath("pickups", name), "sprite"));
            }
        }
    }
}
