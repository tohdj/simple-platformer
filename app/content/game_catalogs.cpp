#include "game_catalogs.hpp"
#include <filesystem>
#include <glm/vec2.hpp>
#include "actor_catalog.hpp"
#include "animation_catalog.hpp"
#include "tile_catalog.hpp"
#include "item_catalog.hpp"
#include "pickup_catalog.hpp"
#include "exit_catalog.hpp"
#include "hud_catalog.hpp"

namespace simple_platformer
{
    // Central resource orchestrator that sequentially loads all game metadata JSON definition
    // tables from disk
    GameCatalogs loadGameCatalogs(
        const std::filesystem::path& catalogDirectory,
        glm::ivec2 atlasSize)
    {
        GameCatalogs catalogs;

        // --- STAGE 1: LOAD INDEPENDENT BASELINE DATA TABLES ---
        // Load layout block characteristics and standard physics rules mappings
        catalogs.tiles = loadTileCatalog(catalogDirectory / "tiles.json");
        // Load raw skeletal clip keyframes and timing sequences variables
        catalogs.animations = loadAnimationCatalog(catalogDirectory / "animations.json");

        // --- STAGE 2: LOAD CROSS-REFERENCED DEPENDENT DATA TABLES ---
        // Actor definitions depend entirely on the animation sets compiled right above
        catalogs.actors = loadActorCatalog(catalogDirectory / "actors.json", catalogs.animations);
        // Load item identifiers, pricing properties, and visual icons descriptors
        catalogs.items = loadItemCatalog(catalogDirectory / "items.json");
        // Loose item stack drops verify their structures against the item definitions table loaded
        // right above
        catalogs.pickups = loadPickupCatalog(catalogDirectory / "pickups.json", catalogs.items);

        // --- STAGE 3: LOAD SYSTEM INTERFACE AND LEVEL METADATA PROFILES ---
        // Load doorway constraints and locking level validation criteria records
        catalogs.exits = loadExitCatalog(catalogDirectory / "exits.json");
        // Load hotbar indicators, health hearts, and menu canvas decoration sprites parameters
        catalogs.hudIcons = loadHudIcons(catalogDirectory / "hud.json");

        // --- STAGE 4: EXECUTE GLOBAL SPRITE SHEET VALIDATION RUNTIMES ---
        // Confirm every loaded asset coordinate maps securely inside the physical dimensions of the
        // texture atlas sheet
        validateAtlasRegions(catalogs, atlasSize, catalogDirectory);

        return catalogs;
    }

    // Directs catalog records to run sub-rect bounds checking passes against physical hardware
    // asset dimension constraints
    void validateAtlasRegions(
        const GameCatalogs& catalogs,
        glm::ivec2 atlasSize,
        const std::filesystem::path& catalogDirectory)
    {
        // Traverses every data matrix block to trap texture clipping coordinates overflow errors
        // early
        validateTileAtlasRegions(
            catalogs.tiles, atlasSize, (catalogDirectory / "tiles.json").string());
        validateAnimationAtlasRegions(
            catalogs.animations, atlasSize, (catalogDirectory / "animations.json").string());
        validateActorAtlasRegions(
            catalogs.actors, atlasSize, (catalogDirectory / "actors.json").string());
        validateItemAtlasRegions(
            catalogs.items, atlasSize, (catalogDirectory / "items.json").string());
        validatePickupAtlasRegions(
            catalogs.pickups, atlasSize, (catalogDirectory / "pickups.json").string());
        validateExitAtlasRegions(
            catalogs.exits, atlasSize, (catalogDirectory / "exits.json").string());
        validateHudAtlasRegions(
            catalogs.hudIcons, atlasSize, (catalogDirectory / "hud.json").string());
    }
}
