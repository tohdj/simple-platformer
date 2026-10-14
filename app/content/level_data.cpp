#include "level_data.hpp"
#include "content_diagnostics.hpp"
#include "content_json.hpp"
#include "content_validation.hpp"

#include <cstddef>
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include <nlohmann/json.hpp>

#include "content/item_catalog.hpp"
#include "simple_platformer/math/coordinates.hpp"

namespace simple_platformer
{
    namespace
    {
        using Json = nlohmann::json;

        // Deserialises a 2-element integer array from JSON down into a discrete 2D grid Cell
        // coordinate structure
        Cell jsonCell(const Json& value, std::string_view sourceName, std::string_view path)
        {
            // Verify that the element contains exactly two numeric values representing an (X, Y)
            // layout pairing
            checkJsonPair(value, "[column, row]", sourceName, path);
            return {
                jsonInteger(
                    value[0],
                    sourceName,
                    indexPath(path, 0)), // Extract horizontal column grid mapping index
                jsonInteger(
                    value[1],
                    sourceName,
                    indexPath(path, 1)) // Extract vertical row grid mapping index
            };
        }

        // Extracts an object's spatial placement position, mandating an exclusive choice between a
        // grid cell or fine sub-pixel float coordinates
        LevelPosition readPosition(
            const Json& object,
            std::string_view cellKey,
            std::string_view feetKey,
            std::string_view sourceName,
            std::string_view path)
        {
            if (!object.is_object())
            {
                failJson(sourceName, path, "expected an object");
            }
            const auto cell = object.find(std::string(cellKey));
            const auto feet = object.find(std::string(feetKey));

            // XOR validation check pattern: force exactly one coordinate configuration format
            // parameter to prevent ambiguous definitions If both are missing or both are
            // simultaneously supplied, it triggers a validation layout crash.
            if ((cell == object.end()) == (feet == object.end()))
            {
                failJson(
                    sourceName,
                    path,
                    "supply exactly one of '" + std::string(cellKey) + "' or '" +
                        std::string(feetKey) + "'");
            }

            // Extract discrete grid tile matrix coordinates if the cell mapping property exists
            if (cell != object.end())
            {
                return jsonCell(*cell, sourceName, fieldPath(path, cellKey));
            }

            // Fall back onto parsing absolute continuous floating-point world position vectors
            return jsonVector(*feet, sourceName, fieldPath(path, feetKey));
        }

        // Parses two sequential spatial landmarks that outline an automated patrol node route for
        // mobile NPCs
        PatrolPlacement jsonPatrol(
            const Json& value,
            std::string_view sourceName,
            std::string_view path)
        {
            // Verify that the object contains only allowed fields before diving deeper into
            // coordinate translations
            checkJsonFields(
                value, {"firstCell", "firstFeet", "secondCell", "secondFeet"}, sourceName, path);

            // Extract the two path nodes safely, allowing either tile cells or absolute
            // floating-point vectors
            return {
                readPosition(value, "firstCell", "firstFeet", sourceName, path),
                readPosition(value, "secondCell", "secondFeet", sourceName, path)};
        }

        // Deserialises specific context properties defining an active AI agent unit spawn
        // configuration
        ActorPlacement jsonActorSettings(
            const Json& value,
            std::string_view sourceName,
            const std::string& path)
        {
            ActorPlacement result;
            // Parse out the required actor reference catalog name key token
            result.definitionName =
                readName(value, "definition", "actor definition name", sourceName, path);

            // Conditionally extract patrol itinerary loops configurations if the sub-node key
            // exists
            const auto found = value.find("patrol");
            if (found != value.end())
            {
                result.patrol = jsonPatrol(*found, sourceName, fieldPath(path, "patrol"));
            }
            return result;
        }

        // Parses ground loot collectibles configuration, enforcing an exclusive split between
        // catalog references and inline custom templates
        PickupPlacement jsonPickupSettings(
            const Json& value,
            std::string_view sourceName,
            const std::string& path)
        {
            PickupPlacement result;

            // Pattern 1: Target element uses a pre-defined structural catalog reference name
            if (value.contains("definition"))
            {
                // Invariant validation: Block parsing if duplicate inline custom parameters try to
                // overlay on top of templates
                if (value.contains("item") || value.contains("quantity") ||
                    value.contains("bodySize"))
                {
                    failJson(
                        sourceName,
                        path,
                        "use either a pickup definition or an inline item, quantity and bodySize");
                }
                result.definitionName =
                    readName(value, "definition", "pickup definition name", sourceName, path);
                return result;
            }

            // Pattern 2: Target element explicitly configures a standalone unique custom item stack
            // setup
            result.stack = {
                readName(value, "item", "item name", sourceName, path),
                readInteger(value, "quantity", sourceName, path)};
            result.bodySize = readVector(value, "bodySize", sourceName, path);

            // Invoke low-level sanity validation checking parameters to verify dimensions are valid
            validatePickupSettings(result, path, sourceName);
            return result;
        }

        // Unpacks portal gate settings, handling entry checklist key item criteria alongside target
        // scene routing indexing constants
        ExitPlacement jsonExitSettings(
            const Json& value,
            std::string_view sourceName,
            const std::string& path)
        {
            ExitPlacement result;
            // Extract the core exit definition structural catalog blueprint name
            result.definitionName = jsonText(
                requiredJsonMember(value, "definition", sourceName, path),
                sourceName,
                fieldPath(path, "definition"));

            // Parse any mandatory inventory validation tracking metrics needed to open up this
            // gateway frame
            if (const auto found = value.find("requirement"); found != value.end())
            {
                const Json& requirement = *found;
                const std::string requirementPath = fieldPath(path, "requirement");
                checkJsonFields(requirement, {"item", "quantity"}, sourceName, requirementPath);

                const int quantity = jsonInteger(
                    requiredJsonMember(requirement, "quantity", sourceName, requirementPath),
                    sourceName,
                    fieldPath(requirementPath, "quantity"));

                result.requirement = NamedItemStack{
                    readName(requirement, "item", "item name", sourceName, requirementPath),
                    quantity};
            }

            // Extract logic flag determining if inventory item counts are reduced upon transition
            // pass interactions
            if (const auto found = value.find("consumeItem"); found != value.end())
            {
                result.consumeItem =
                    jsonBoolean(*found, sourceName, fieldPath(path, "consumeItem"));
            }

            // Parse optional sequence forward redirection indices targeting downstream scenes
            // numbers
            if (const auto found = value.find("nextLevel"); found != value.end())
            {
                const int nextLevel = jsonInteger(*found, sourceName, fieldPath(path, "nextLevel"));
                result.nextLevel = nextLevel;
            }

            // Run a comprehensive post-parse validation sweep to confirm integrity across the
            // entire exit structure
            validateExitSettings(result, path, sourceName);
            return result;
        }

        // Extracts the textual map layout lines array from JSON and validates its complete
        // rectangular shape consistency
        std::vector<std::string> jsonMapRows(
            const Json& value,
            std::string_view sourceName,
            const std::map<char, std::string>& legend)
        {
            // Block processing if the map element array node is missing or completely unassigned
            if (!value.is_array() || value.empty())
            {
                failJson(sourceName, "map", "expected at least one row");
            }

            std::vector<std::string> rows;
            rows.reserve(value.size()); // Pre-allocate vector nodes to optimize loop assembly
            for (std::size_t rowIndex = 0; rowIndex < value.size(); ++rowIndex)
            {
                const std::string path = indexPath("map", rowIndex);
                rows.push_back(jsonText(value[rowIndex], sourceName, path));
            }

            // Execute cell-by-cell validation loops checking for uniform dimensions and unknown
            // tracking symbols
            validateMapRows(rows, legend, sourceName);
            return rows;
        }

        // Gathers dictionary char keys to prevent symbol registration collisions between background
        // tiles and game objects
        void checkLegendSymbols(const Json& tiles, const Json& objects, std::string_view sourceName)
        {
            std::vector<std::string> tileSymbols;
            std::vector<std::string> objectSymbols;
            for (const auto& entry : tiles.items())
            {
                tileSymbols.push_back(entry.key());
            }
            for (const auto& entry : objects.items())
            {
                objectSymbols.push_back(entry.key());
            }

            // Guard against ambiguity: a single glyph marker character cannot represent both a tile
            // block and an interactive actor
            validateLegendSymbols(tileSymbols, objectSymbols, sourceName);
        }

        // Identifies and traces file positions for explicit user character initial spawn field
        // nodes
        std::vector<PlacementOrigin> jsonPlayerOrigins(const Json& root)
        {
            std::vector<PlacementOrigin> result;
            for (const auto* key : {"playerSpawnCell", "playerSpawnFeet"})
            {
                if (root.contains(key))
                {
                    result.push_back({key, std::nullopt});
                }
            }
            return result;
        }

        // Safely logs parsed actor identifiers into internal lists to verify global catalog key
        // integrity checks
        void collectActorReference(
            LevelData& level,
            const ActorPlacement& actor,
            const std::string& path)
        {
            level.actorReferences.emplace(fieldPath(path, "definition"), actor.definitionName);
        }

        // Resolves whether a collectible item is an inline custom item stack or an existing catalog
        // template definition link
        void collectPickupReference(
            LevelData& level,
            const PickupPlacement& pickup,
            const std::string& path)
        {
            if (pickup.definitionName.empty())
            {
                // Pattern 1: Inline custom item name tracking reference
                level.itemReferences.emplace(fieldPath(path, "item"), pickup.stack.item);
            }
            else
            {
                // Pattern 2: Global structural pickup catalog blueprint reference
                level.pickupReferences.emplace(
                    fieldPath(path, "definition"), pickup.definitionName);
            }
        }

        // Logs level transition gateway definition keys and item requirement keys to catch broken
        // links before a level initializes
        void collectExitReferences(
            LevelData& level,
            const ExitPlacement& exit,
            const std::string& path)
        {
            level.exitReferences.emplace(fieldPath(path, "definition"), exit.definitionName);
            if (exit.requirement)
            {
                // Map out dependency links tracking the specific inventory item ID needed to open
                // this exit path portal
                level.itemReferences.emplace(
                    fieldPath(path, "requirement.item"), exit.requirement->item);
            }
        }

        // Tagging struct container used as a structural placeholder for locating the player inside
        // character text charts
        struct PlayerMarker
        {
        };

        // Templates contain parsed settings. Their spawn is assigned only when a map
        // cell copies the template into a placement; it is never read from template JSON.
        using ObjectTemplate =
            std::variant<PlayerMarker, ActorPlacement, PickupPlacement, ExitPlacement>;
        using ObjectLegend = std::map<char, ObjectTemplate>;

        // Maps structural JSON nodes down into specific type-checked variant objects based on their
        // blueprint descriptor fields
        ObjectTemplate jsonObjectTemplate(
            const Json& object,
            std::string_view sourceName,
            const std::string& path,
            LevelData& level)
        {
            const std::string type = readText(object, "type", sourceName, path);

            // Invariant validation safety check: legend templates cannot declare absolute vector
            // locations internally
            if (object.contains("spawnCell") || object.contains("spawnFeet"))
            {
                failJson(sourceName, path, "position comes from the map symbol");
            }

            // --- BRANCH 1: EVALUATE PLAYER CHARACTER SPAWN TEMPLATES ---
            if (type == "player")
            {
                checkJsonFields(object, {"type"}, sourceName, path);
                return PlayerMarker{};
            }

            // --- BRANCH 2: EVALUATE MOBILE AGENT / ENEMY NPC BLUEPRINTS ---
            if (type == "actor")
            {
                checkJsonFields(object, {"type", "definition", "patrol"}, sourceName, path);
                ActorPlacement actor = jsonActorSettings(object, sourceName, path);
                collectActorReference(
                    level,
                    actor,
                    path); // Log template definition names trackers for catalog integrity checks
                return actor;
            }

            // --- BRANCH 3: EVALUATE LOOSE COLLECTIBLE WORLD LOOT DROPS ---
            if (type == "pickup")
            {
                checkJsonFields(
                    object,
                    {"type", "definition", "item", "quantity", "bodySize"},
                    sourceName,
                    path);
                PickupPlacement pickup = jsonPickupSettings(object, sourceName, path);
                collectPickupReference(
                    level, pickup, path); // Log structural item catalog tokens links
                return pickup;
            }

            // --- BRANCH 4: EVALUATE SCENE REDIRECTION PORTAL PORTALS ---
            if (type == "exit")
            {
                checkJsonFields(
                    object,
                    {"type", "definition", "requirement", "consumeItem", "nextLevel"},
                    sourceName,
                    path);
                ExitPlacement exit = jsonExitSettings(object, sourceName, path);
                collectExitReferences(level, exit, path);
                return exit;
            }

            // Fallback catch block handles unregistered classification strings configurations
            failJson(
                sourceName,
                fieldPath(path, "type"),
                "unknown object type '" + type + "'; expected player, actor, pickup, or exit");
        }

        // Parses mapping character graphs dictionary charts (tile legends vs entity object legends)
        // from the configuration root
        ObjectLegend jsonLegends(const Json& root, std::string_view sourceName, LevelData& level)
        {
            // 1. Fetch text chart translation definitions mapping static background blocks
            const Json& tiles = requiredJsonMember(root, "tileLegend", sourceName, "root");
            if (!tiles.is_object() || tiles.empty())
            {
                failJson(sourceName, "tileLegend", "expected a nonempty object");
            }

            const Json emptyLegend = Json::object();
            // Fetch dictionary sets tracking interactive layout items if declared inside the scene
            // file
            const Json& objects =
                root.contains("objectLegend") ? root.at("objectLegend") : emptyLegend;

            checkJsonObject(objects, sourceName, "objectLegend");
            checkLegendSymbols(
                tiles,
                objects,
                sourceName); // Shield against cross-category duplicate symbol map keys errors

            // 2. Deserialize terrain mapping characters blocks keys
            for (const auto& entry : tiles.items())
            {
                level.tileLegend.emplace(
                    entry.key().front(), jsonText(entry.value(), sourceName, "tileLegend"));
            }

            // 3. Deserialize entity instantiation templates keys matching structural layout symbols
            ObjectLegend result;
            for (const auto& entry : objects.items())
            {
                const char symbol = entry.key().front();
                result.emplace(
                    symbol,
                    jsonObjectTemplate(
                        entry.value(), sourceName, fieldPath("objectLegend", entry.key()), level));

                // Architectural design rule constraint: interactive markers implicitly instantiate
                // an air/empty terrain block behind themselves so that entities don't glitch or
                // overlap inside solid wall block textures.
                level.tileLegend.emplace(symbol, "empty");
            }
            return result;
        }

        // Structural diagnostic summary layout tracking the physical map source origins of singular
        // objects
        struct PlacementOrigins
        {
            std::vector<PlacementOrigin> players;
            std::vector<PlacementOrigin> exits;
        };

        // Extracts a node member pointer address from an array structure while checking array
        // constraint boundaries
        const Json& jsonPlacementArray(
            const Json& root,
            const char* key,
            std::string_view sourceName,
            const Json& emptyArray)
        {
            // Fall back onto pre-allocated empty structures array matrices if data maps miss
            // specified keys targets
            const Json& result = root.contains(key) ? root.at(key) : emptyArray;
            if (!result.is_array())
            {
                failJson(sourceName, key, "expected an array");
            }
            return result;
        }

        // Deserialises explicit coordinates array sets for actors, pickups, and exits outside the
        // text map layout grid
        PlacementOrigins jsonExplicitPlacements(
            const Json& root,
            std::string_view sourceName,
            LevelData& level)
        {
            PlacementOrigins origins;
            // 1. Process explicit top-level configuration markers allocating player character
            // structures
            origins.players = jsonPlayerOrigins(root);
            if (!origins.players.empty())
            {
                // Verify that multiple conflicting initial spawn positions are not allocated across
                // parameters
                validateSinglePlacement(origins.players, "player", sourceName);
                level.playerSpawn =
                    readPosition(root, "playerSpawnCell", "playerSpawnFeet", sourceName, "root");
            }

            const Json emptyArray = Json::array();

            // 2. Loop and map explicit placement configurations arrays for mobile guard/NPC
            // entities
            const Json& actors = jsonPlacementArray(root, "actors", sourceName, emptyArray);
            for (std::size_t index = 0; index < actors.size(); ++index)
            {
                const Json& object = actors[index];
                const std::string path = indexPath("actors", index);
                checkJsonFields(
                    object, {"definition", "spawnCell", "spawnFeet", "patrol"}, sourceName, path);

                ActorPlacement actor = jsonActorSettings(object, sourceName, path);
                actor.spawn = readPosition(object, "spawnCell", "spawnFeet", sourceName, path);

                collectActorReference(
                    level, actor, path); // Track character identifier metrics for index validation
                level.actors.push_back(std::move(actor));
            }

            // 3. Loop and map explicit placement configurations arrays for collectible ground items
            const Json& pickups = jsonPlacementArray(root, "pickups", sourceName, emptyArray);
            for (std::size_t index = 0; index < pickups.size(); ++index)
            {
                const Json& object = pickups[index];
                const std::string path = indexPath("pickups", index);
                checkJsonFields(
                    object,
                    {"definition", "item", "quantity", "bodySize", "spawnCell", "spawnFeet"},
                    sourceName,
                    path);

                PickupPlacement pickup = jsonPickupSettings(object, sourceName, path);
                pickup.spawn = readPosition(object, "spawnCell", "spawnFeet", sourceName, path);

                collectPickupReference(
                    level, pickup, path); // Track inventory item types definitions keys
                level.pickups.push_back(std::move(pickup));
            }

            // 4. Extract parameters tracking scene portal gateway structures if present in current
            // scopes
            if (root.contains("exit"))
            {
                const Json& object = root.at("exit");
                checkJsonFields(
                    object,
                    {"definition",
                     "spawnCell",
                     "spawnFeet",
                     "requirement",
                     "consumeItem",
                     "nextLevel"},
                    sourceName,
                    "exit");

                level.exit = jsonExitSettings(object, sourceName, "exit");
                level.exit.spawn =
                    readPosition(object, "spawnCell", "spawnFeet", sourceName, "exit");

                collectExitReferences(level, level.exit, "exit");
                origins.exits.push_back({"exit", std::nullopt});
            }
            return origins;
        }

        // Resolves character glyph symbols found in the ASCII diagram to place instances directly
        // inside data vectors
        void placeMapObject(
            const ObjectTemplate& object,
            Cell cell,
            char symbol,
            std::string_view sourceName,
            LevelData& level,
            PlacementOrigins& origins)
        {
            // Map text matrix position tracking properties to build standard diagnostic path logs
            // traces
            const PlacementOrigin origin{
                indexPath(
                    indexPath("map", static_cast<std::size_t>(cell.y)),
                    static_cast<std::size_t>(cell.x)),
                symbol};

            // Check target types matching variant definitions states
            if (std::holds_alternative<PlayerMarker>(object))
            {
                origins.players.push_back(origin);
                validateSinglePlacement(origins.players, "player", sourceName);
                level.playerSpawn =
                    cell; // Assign cell structural coordinates as the player spawn base
            }
            else if (const auto* actor = std::get_if<ActorPlacement>(&object))
            {
                level.actors.push_back(*actor);
                level.actors.back().spawn =
                    cell; // Lock NPC placement onto current grid coordinates
            }
            else if (const auto* pickup = std::get_if<PickupPlacement>(&object))
            {
                level.pickups.push_back(*pickup);
                level.pickups.back().spawn =
                    cell; // Lock item asset placement onto current grid coordinates
            }
            else if (const auto* exit = std::get_if<ExitPlacement>(&object))
            {
                origins.exits.push_back(origin);
                validateSinglePlacement(origins.exits, "exit", sourceName);
                level.exit = *exit;
                level.exit.spawn = cell; // Assign portal frame coordinates
            }
        }

        // Intercepts character text rows and executes a full column scanning pass looking for
        // special object tokens
        void placeMapObjects(
            const ObjectLegend& legend,
            std::string_view sourceName,
            LevelData& level,
            PlacementOrigins& origins)
        {
            for (std::size_t row = 0; row < level.mapRows.size(); ++row)
            {
                const std::string& cells = level.mapRows[row];
                for (std::size_t column = 0; column < cells.size(); ++column)
                {
                    const char symbol = cells[column];
                    const auto found = legend.find(symbol);
                    // If the character glyph matches a signature key stored inside object templates
                    // legends
                    if (found != legend.end())
                    {
                        const Cell cell{static_cast<int>(column), static_cast<int>(row)};
                        // Handoff to map positioning managers to resolve object arrays adjustments
                        placeMapObject(found->second, cell, symbol, sourceName, level, origins);
                    }
                }
            }
        }

        // Orchestrator function called to fully parse raw layout schemas down to binary engine
        // properties models
        LevelData jsonLevelData(const Json& root, std::string_view sourceName)
        {
            // Verify all key root objects nodes keys exist within the source structure definitions
            checkJsonFields(
                root,
                {"tileLegend",
                 "objectLegend",
                 "map",
                 "playerSpawnCell",
                 "playerSpawnFeet",
                 "actors",
                 "pickups",
                 "exit"},
                sourceName,
                "root");

            LevelData result;

            // 1. Process character legends mapping criteria lists definitions
            const ObjectLegend objects = jsonLegends(root, sourceName, result);
            // 2. Build map block layers matrices rows out of string text fields array vectors
            result.mapRows = jsonMapRows(
                requiredJsonMember(root, "map", sourceName, "root"), sourceName, result.tileLegend);
            // 3. Process coordinate placement definitions declared outside textual charts
            PlacementOrigins origins = jsonExplicitPlacements(root, sourceName, result);
            // 4. Overlay chart items and symbols data states to finalize entity position listings
            placeMapObjects(objects, sourceName, result, origins);

            // 5. Final validation passes confirming singular mandatory world conditions are fully
            // satisfied
            validateSinglePlacement(origins.players, "player", sourceName);
            validateSinglePlacement(origins.exits, "exit", sourceName);

            return result;
        }
    }

    // Parses raw text data string buffers into an engine-ready structured LevelData object
    // blueprint
    LevelData parseLevelData(std::string_view text, std::string_view sourceName)
    {
        // Initialize structural JSON parsing from the raw stream context to build the root node
        // tree
        const Json root = parseContentRoot(text, sourceName);
        try
        {
            // Forward the root structural node handle to the main level metadata compilation layout
            // mapping loop
            return jsonLevelData(root, sourceName);
        }
        // Intercept native library parsing exceptions to augment the message with local file
        // diagnostic contexts
        catch (const Json::exception& exception)
        {
            // Re-package the diagnostic logs safely before throwing a runtime validation exception
            // path trigger
            failJson(sourceName, {}, std::string("invalid JSON: ") + exception.what());
        }
    }

    // Handles drive reading disk input-output interactions and passes text contents straight to
    // text compilation nodes
    LevelData loadLevelData(const std::filesystem::path& path)
    {
        // Convert the file contents directly down to strings while extracting absolute file paths
        // for error telemetry maps
        return parseLevelData(loadContentText(path), path.string());
    }
}
