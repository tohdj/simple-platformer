#include "tile_catalog.hpp"
#include "content_diagnostics.hpp"
#include "content_json.hpp"
#include "content_validation.hpp"

#include <cstddef>
#include <filesystem>
#include <map>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>
#include "simple_platformer/world/tile_map.hpp"

namespace simple_platformer
{
    // Parses a raw string text configuration structure down into an operational binary tile catalog
    // collection matrix
    TileCatalog parseTileCatalog(std::string_view text, std::string_view sourceName)
    {
        // 1. Initialize structural JSON token parses out of the stream
        const auto root = parseContentRoot(text, sourceName);
        checkJsonFields(root, {"tileSize", "tiles"}, sourceName, "root");
        const auto& tiles = requiredJsonMember(root, "tiles", sourceName, "root");
        checkJsonObject(tiles, sourceName, "tiles");

        // Ensure a fundamental 'empty' fallback tile layout profile exists within the
        // configurations
        if (!tiles.contains("empty"))
        {
            failJson(sourceName, "tiles", "missing 'empty'");
        }

        TileCatalog result;
        result.tileSize = jsonInteger(
            requiredJsonMember(root, "tileSize", sourceName, "root"), sourceName, "tileSize");

        // Resolve breaksInto definitions after every tile has an assigned internal ID,
        // allowing forward references inside the JSON file to evaluate cleanly.
        std::map<std::string, std::string> breaksIntoNames;

        // Internal helper lambda function to construct and validate individual TileDefinition
        // schemas
        const auto add = [&result, &breaksIntoNames, sourceName](
                             const std::string& name, const nlohmann::json& value)
        {
            const std::string path = fieldPath("tiles", name);
            // Non-empty tiles carry strict schema requirements mapping texture sheet properties
            if (name == "empty")
            {
                checkJsonFields(
                    value, {"blocksMovement", "blocksSight", "climbable"}, sourceName, path);
            }
            else
            {
                checkJsonFields(
                    value,
                    {"blocksMovement", "blocksSight", "climbable", "sprite", "breaksInto"},
                    sourceName,
                    path);
            }

            TileDefinition definition;
            definition.blocksMovement = readBoolean(value, "blocksMovement", sourceName, path);
            definition.blocksSight = readBoolean(value, "blocksSight", sourceName, path);
            readOptionalBoolean(value, "climbable", definition.climbable, sourceName, path);

            if (name != "empty")
            {
                const auto& sprite = requiredJsonMember(value, "sprite", sourceName, path);
                const std::string spritePath = fieldPath(path, "sprite");
                checkJsonFields(sprite, {"position"}, sourceName, spritePath);

                // Set the default sprite drawing bounds matching the globally defined tile size
                const auto side = static_cast<float>(result.tileSize);
                definition.sprite = {
                    jsonVector(
                        requiredJsonMember(sprite, "position", sourceName, spritePath),
                        sourceName,
                        fieldPath(spritePath, "position")),
                    {side, side}};

                std::string breaksInto;
                readOptionalText(value, "breaksInto", breaksInto, sourceName, path);
                if (!breaksInto.empty())
                {
                    // Cache the text reference configuration key for secondary link resolution
                    // passes
                    breaksIntoNames.emplace(name, breaksInto);
                }
            }

            // Map the identifier keys text tokens to sequential vector index array handles
            result.ids.emplace(name, static_cast<int>(result.definitions.size()));
            result.definitions.push_back(definition);
        };

        // 2. Enforce the empty index at position zero, then parse remaining layout components
        add("empty", tiles.at("empty"));
        for (const auto& entry : tiles.items())
        {
            if (entry.key() != "empty")
            {
                add(entry.key(), entry.value());
            }
        }

        // 3. SECONDARY PASS: Resolve string keys to lookup target indexes for breakable tile
        // reactions
        for (const auto& entry : breaksIntoNames)
        {
            const std::string path = fieldPath(fieldPath("tiles", entry.first), "breaksInto");
            const auto target = result.ids.find(entry.second);
            // Fail structural builds if dest break target doesn't exist anywhere inside definitions
            // arrays
            if (target == result.ids.end())
            {
                failJson(sourceName, path, "unknown tile name '" + entry.second + "'");
            }

            const auto broken = static_cast<std::size_t>(result.ids.at(entry.first));
            result.definitions[broken].breaksIntoTileId = target->second;
        }

        // Conclude setup steps by applying post-construction validation data verification loops
        validateInFile(sourceName, [&] { validateTileCatalog(result); });
        return result;
    }

    // Handles drive reading IO operations and directs content text directly downward to text
    // parsing units
    TileCatalog loadTileCatalog(const std::filesystem::path& path)
    {
        return parseTileCatalog(loadContentText(path), path.string());
    }

    // Instantiation factory that assembles text-based grid diagrams into full tile map objects
    // using legend catalogs
    TileMap composeTileMap(
        const std::vector<std::string>& rows,
        const std::map<char, std::string>& legend,
        const TileCatalog& catalog)
    {
        // Callers can supply catalogs built directly in C++, without using the JSON loader.
        validateTileCatalog(catalog);
        validateTileLegend(legend, catalog);

        std::map<char, int> ids;
        // Map ascii char symbols directly to underlying validated definition tracker id handles
        for (const auto& entry : legend)
        {
            ids.emplace(entry.first, catalog.ids.at(entry.second));
        }

        // Invoke direct factory matrix deserializers to transform text blocks lines into world tile
        // grids
        return TileMap::fromAscii(catalog.tileSize, rows, catalog.definitions, ids);
    }

    // Validates sprite boundaries texture allocations to confirm coordinate regions sit safely
    // within physical atlas sizes
    void validateTileAtlasRegions(
        const TileCatalog& catalog,
        glm::ivec2 atlasSize,
        std::string_view sourceName)
    {
        for (const auto& [name, id] : catalog.ids)
        {
            // ID index 0 represents empty/air gap space configurations which skip graphics pass
            // loops entirely
            if (id != 0)
            {
                requireInAtlas(
                    catalog.definitions[static_cast<std::size_t>(id)].sprite,
                    atlasSize,
                    sourceName,
                    fieldPath(fieldPath("tiles", name), "sprite"));
            }
        }
    }
}
