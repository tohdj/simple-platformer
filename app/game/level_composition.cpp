#include "level_composition.hpp"
#include "content/game_catalogs.hpp"
#include "content/actor_catalog.hpp"
#include "content/actor_definition.hpp"
#include "content/item_catalog.hpp"
#include "content/pickup_catalog.hpp"
#include "content/exit_catalog.hpp"
#include "content/level_catalog.hpp"
#include "content/level_data.hpp"
#include "content/tile_catalog.hpp"
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>

#include <glm/vec2.hpp>
#include "simple_platformer/actor/actor.hpp"
#include "simple_platformer/actor/actor_id.hpp"
#include "simple_platformer/math/coordinates.hpp"
#include "simple_platformer/npc/npc.hpp"
#include "simple_platformer/world/level_exit.hpp"
#include "simple_platformer/world/pickup.hpp"
#include "simple_platformer/world/tile_map.hpp"

namespace simple_platformer
{
    namespace
    {
        // Extracts absolute 2D world floor coordinates from a generic structural level position
        // variant type
        glm::vec2 feetOf(const TileMap& map, const LevelPosition& position)
        {
            // If the placement is defined via a discrete cell grid coordinate index structure
            if (const auto* cell = std::get_if<Cell>(&position))
            {
                // Translate grid indices (Column, Row) into pixel spatial vectors using the map's
                // tile sizing constants
                return feetInCell(map.tileSize(), *cell);
            }
            // Otherwise, extract the embedded floating-point world vector values directly out of
            // the position variant
            return std::get<glm::vec2>(position);
        }

        // Factory wrapper that handles creating a physical collectible item container on the game
        // map layout
        Pickup makePickup(
            const TileMap& map,
            const PickupPlacement& placement,
            const PickupCatalog& pickups,
            const ItemCatalog& items,
            int textureId)
        {
            // Verify if the configuration refers to a pre-defined reference template name inside
            // catalogs
            if (!placement.definitionName.empty())
            {
                return composePickup(
                    pickupDefinition(pickups, placement.definitionName),
                    items,
                    textureId,
                    feetOf(
                        map,
                        placement.spawn)); // Translate the destination coordinate structures safely
            }
            // Fall back to a custom configuration setup override if an explicit definition name is
            // absent
            PickupDefinition definition;
            definition.stack = placement.stack;
            definition.bodySize = placement.bodySize;
            return composePickup(definition, items, textureId, feetOf(map, placement.spawn));
        }

        // Generates an automated walking itinerary movement frame loop configuration for mobile
        // guard units
        std::optional<Patrol> makePatrol(
            const TileMap& map,
            const std::optional<PatrolPlacement>& placement)
        {
            // Drop out cleanly if no specific guard path configuration settings are requested
            if (!placement.has_value())
            {
                return std::nullopt;
            }
            // Package absolute target points and set the looping direction activity flag to active
            return Patrol{feetOf(map, placement->first), feetOf(map, placement->second), true};
        }

        // Assembles gate or gateway exit doors and loads mandatory inventory dependencies (such as
        // key cards)
        LevelExit makeExit(
            const TileMap& map,
            int textureId,
            const ExitPlacement& placement,
            const ItemCatalog& items,
            const ExitCatalog& exits)
        {
            LevelExit exit = composeExit(
                exitDefinition(exits, placement.definitionName),
                textureId,
                feetOf(map, placement.spawn));

            // If the gate requires explicit items to open, link structural item quantities into
            // requirements
            if (placement.requirement)
            {
                exit.requirement = composeItemStack(items, *placement.requirement);
            }
            exit.consumeItem =
                placement
                    .consumeItem; // Track whether key items are removed from inventory upon usage
            exit.nextLevel = placement.nextLevel; // Target layer number destination configuration
            return exit;
        }
    }

    // Top-level assembly factory that loads structural scene data off disk and validates catalog
    // constraints
    GameLevel composeGameLevel(
        const LevelCatalog& catalog,
        int levelNumber,
        int textureId,
        const GameCatalogs& catalogs)
    {
        // 1. Resolve level paths on drive and pull file serialization data states downstream
        const auto path = levelPath(catalog, levelNumber);
        const LevelData data = loadLevelData(path);

        // Cache catalog collection reference targets locally to reduce indirection lookups
        const auto& tiles = catalogs.tiles;
        const auto& actors = catalogs.actors;
        const auto& exits = catalogs.exits;
        const auto& items = catalogs.items;
        const auto& pickups = catalogs.pickups;

        // --- DATA VALIDATION LOOPS: Verify all string keys resolve to active database objects ---
        for (const auto& reference : data.exitReferences)
        {
            try
            {
                exitDefinition(exits, reference.second);
            }
            catch (const std::invalid_argument& error)
            {
                throw std::invalid_argument(
                    path.string() + ": " + reference.first + ": " + error.what());
            }
        }
        for (const auto& reference : data.itemReferences)
        {
            try
            {
                itemDefinition(items, reference.second);
            }
            catch (const std::invalid_argument& error)
            {
                throw std::invalid_argument(
                    path.string() + ": " + reference.first + ": " + error.what());
            }
        }
        for (const auto& reference : data.pickupReferences)
        {
            try
            {
                pickupDefinition(pickups, reference.second);
            }
            catch (const std::invalid_argument& error)
            {
                throw std::invalid_argument(
                    path.string() + ": " + reference.first + ": " + error.what());
            }
        }
        for (const auto& reference : data.actorReferences)
        {
            try
            {
                actorDefinition(actors, reference.second);
            }
            catch (const std::invalid_argument& error)
            {
                throw std::invalid_argument(
                    path.string() + ": " + reference.first + ": " + error.what());
            }
        }

        // 2. Parse text legends maps down into hardware matrix arrays layout tile blocks
        TileMap map = composeTileMap(data.mapRows, data.tileLegend, tiles);
        World world(composeItems(items, textureId));
        std::unordered_map<std::uint32_t, std::string> actorDefinitionNames;

        // 3. Populate dynamic scene characters and assign active tracking ID mappings
        for (const auto& placement : data.actors)
        {
            try
            {
                const ActorId id = world.addActor(composeActor(
                    actorDefinition(actors, placement.definitionName),
                    catalogs.animations,
                    textureId,
                    feetOf(map, placement.spawn),
                    makePatrol(map, placement.patrol)));

                // Track dynamic ID pairings to simplify structural diagnostics lookups in debug
                // panels
                actorDefinitionNames.emplace(id.value, placement.definitionName);
            }
            catch (const std::invalid_argument& error)
            {
                throw std::invalid_argument(
                    path.string() + ": actor '" + placement.definitionName + "': " + error.what());
            }
        }

        // 4. Instantiation steps for loose collectables and portal gate frames
        for (const auto& placement : data.pickups)
        {
            world.addPickup(makePickup(map, placement, pickups, items, textureId));
        }
        world.setExit(makeExit(map, textureId, data.exit, items, exits));

        // 5. Track spawn foundations for the player container instance and assemble properties
        const glm::vec2 playerSpawnFeet = feetOf(map, data.playerSpawn);
        return {
            levelNumber,
            std::move(map),
            std::move(world),
            playerSpawnFeet,
            std::move(actorDefinitionNames)};
    }

    // Direct instantiation factory to assemble the core player model properties structure cleanly
    Actor composePlayer(const GameCatalogs& catalogs, int textureId)
    {
        const auto& actors = catalogs.actors;
        // Query catalog definitions by key string and pass parameters over to structural composers
        return composeActor(actorDefinition(actors, actors.player), catalogs.animations, textureId);
    }
}
