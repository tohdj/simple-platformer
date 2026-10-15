/**
 * @file        game.cpp
 * @brief       Declares the primary Game management class that coordinates simulation frames,
 *              level loading transitions, input intent application, and visual compilation.
 *
 * @author      Mohd Fadzuli
 * @author      Adapted by: Toh Da Jun
 * @copyright   Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#include "game.hpp"

#include "debug/debug_overlay.hpp"
#include "content/actor_catalog.hpp"
#include "debug/navigation_debug.hpp"
#include "level_composition.hpp"
#include "content/level_catalog.hpp"
#include "content/game_catalogs.hpp"
#include "content/hud_catalog.hpp"

#include <cstddef>
#include <optional>
#include <stdexcept>
#include <utility>

#include "simple_platformer/actor/actor.hpp"
#include "simple_platformer/actor/actor_id.hpp"
#include "simple_platformer/input/input_state.hpp"
#include "simple_platformer/inventory/inventory.hpp"
#include "simple_platformer/inventory/item.hpp"
#include "simple_platformer/math/aabb.hpp"
#include "simple_platformer/math/coordinates.hpp"
#include "simple_platformer/movement/platformer_movement.hpp"
#include "simple_platformer/math/validation.hpp"
#include "simple_platformer/navigation/actor_navigation.hpp"
#include "simple_platformer/render/camera.hpp"
#include "simple_platformer/render/presentation.hpp"
#include "simple_platformer/render/render_scene.hpp"
#include "simple_platformer/render/sprite.hpp"
#include "simple_platformer/world/level_exit.hpp"
#include "simple_platformer/world/level_validation.hpp"
#include "simple_platformer/world/world_requests.hpp"
#include "simple_platformer/world/world_simulation.hpp"

namespace simple_platformer
{
    // Constructor: Orchestrates the foundational data layout, structures assets, and spins up the
    // initial level
    Game::Game(
        int textureId,
        LevelCatalog levelCatalog,
        GameCatalogs gameCatalogs,
        float stepSeconds)
        : levelCatalog(std::move(levelCatalog)),
          gameCatalogs(std::move(gameCatalogs)),
          level(composeGameLevel(
              this->levelCatalog,
              this->levelCatalog.startLevel,
              textureId,
              this->gameCatalogs)),
          atlasTextureId(textureId),
          simulationStepSeconds(stepSeconds)
    {
        // Guard configuration invariants against division-by-zero or backward time increments
        if (!isFinitePositive(simulationStepSeconds))
        {
            throw std::invalid_argument("The game's simulation step must be finite and positive");
        }
        // Spawn and map the player into the initial starting layout
        startLevel(composePlayer(this->gameCatalogs, atlasTextureId));
    }

    // Transfers vital player states across standard game level transitions
    void Game::loadLevel(int levelNumber)
    {
        Actor nextPlayer = composePlayer(gameCatalogs, atlasTextureId);

        // Find existing player entity in the current simulation pool
        if (const Actor* previousPlayer = level.world.findActor(level.world.playerId()))
        {
            // Carry over persistent character traits across scene segments
            nextPlayer.health = previousPlayer->health;
            nextPlayer.inventory = previousPlayer->inventory;
        }
        // Only player health and inventory carry over; the new world has fresh runtime state.
        replaceLevel(levelNumber, std::move(nextPlayer));
    }

    // Reconstructs level configurations and builds corresponding actor containers
    void Game::replaceLevel(int levelNumber, Actor player)
    {
        level = composeGameLevel(levelCatalog, levelNumber, atlasTextureId, gameCatalogs);
        startLevel(std::move(player));
    }

    // Instantiates physics bounds, links player identifiers, and registers cameras for a newly
    // active scene
    void Game::startLevel(Actor player)
    {
        // Align character box feet layout directly onto designated map spawn points
        moveFeetTo(player.body.bounds, level.playerSpawnFeet);

        // Insert character into world simulation engine
        const ActorId playerId = level.world.addActor(std::move(player));
        level.actorDefinitionNames.emplace(playerId.value, gameCatalogs.actors.player);
        level.world.setPlayer(playerId, level.playerSpawnFeet);

        // Audit structural entities inside the map bounds for consistency
        validateLevelActors(level.map, level.world, level.number);

        const Actor* playerActor = level.world.findActor(playerId);
        if (playerActor == nullptr)
        {
            throw std::logic_error("The game could not initialise its camera");
        }

        // Setup view framing window bounds focused directly on the player
        cameraController =
            makeCameraController(level.map, playerActor->body.bounds, levelCatalog.cameraDeadZone);

        // Pre-compute internal mesh paths and nodes for NPC traversal patterns
        prepareNavigation(level.map, level.world, simulationStepSeconds);
    }

    // Main Update Step: Advances the logical simulation tick and frame-by-frame presentation clocks
    void Game::update(const InputIntentions& intentions, float deltaTime)
    {
        if (gameComplete)
        {
            return;
        }

        Actor* player = level.world.findActor(level.world.playerId());
        if (player == nullptr)
        {
            throw std::logic_error("The game has no player");
        }

        // Pipe current hardware state or virtual control intents down to player parameters
        player->intentions = intentions;

        // Advance physics solvers, collision dynamics, and entity movement updates
        updateWorldSimulation(level.map, level.world, deltaTime);

        // Process conditional level completion flags
        if (level.world.levelComplete())
        {
            const auto& completedExit = level.world.exit();
            if (!completedExit.has_value())
            {
                throw std::logic_error("A completed game level must have an exit");
            }

            const auto nextLevel = completedExit->nextLevel;
            if (nextLevel.has_value())
            {
                loadLevel(*nextLevel); // Progress down the stack to the subsequent room
            }
            else
            {
                gameComplete = true; // No levels remaining means final campaign completion
            }
            return;
        }

        // Fetch fresh object address after structural array modification or expansion
        player = level.world.findActor(level.world.playerId());
        if (player == nullptr)
        {
            throw std::logic_error("The game has no player after lifecycle update");
        }

        // Center the scrolling view bounding box smoothly around player coordinates
        followTarget(cameraControllerValue(), level.map, player->body.bounds);

        // Animations and fades pause with the simulation while the exit opens. The door's
        // flash and the player's fade into it follow the world clock instead.
        if (!exitOpening(level.world))
        {
            updateWorldPresentation(level.map, level.world, deltaTime);
        }
    }

    // Transforms screen interface spaces into spatial target offsets relative to the player
    glm::vec2 Game::playerAimDirection(glm::vec2 screenPosition) const
    {
        const Actor* player = level.world.findActor(level.world.playerId());
        if (player == nullptr)
        {
            throw std::logic_error("The game has no player");
        }

        // Compute relative vector offset from target center mass pointing toward cursor space
        return screenToWorld(currentCamera(), screenPosition) - centerOf(player->body.bounds);
    }

    // Assembles structural texture layers and visible sprites for pipeline processing
    RenderScene Game::buildScene() const
    {
        const Actor* player = level.world.findActor(level.world.playerId());
        if (player == nullptr || !player->sprite.has_value())
        {
            throw std::logic_error("The player is missing its sprite");
        }

        // Collect tile coordinates, background spaces, and current viewport transforms
        return buildRenderScene(
            level.map, player->sprite.value().textureId, currentCamera(), level.world);
    }

    // Generates telemetry data structures to fuel developer diagnostic graphs and map previews
    DebugOverlay Game::debugOverlay(
        float atlasWidth,
        std::optional<glm::vec2> internalCursor,
        std::size_t navigationProfileIndex,
        std::optional<float> framerate) const
    {
        NavigationDebugView navigation;
        if (internalCursor.has_value())
        {
            // Map the internal pointer system to world coordinates
            navigation.cursorWorld =
                screenToWorld(currentCamera(), internalCursor.value_or(glm::vec2{0.0F, 0.0F}));
        }
        navigation.profileIndex = navigationProfileIndex;

        // Label navigation profiles from definitions that can use ground NPC navigation.
        for (const auto& [name, definition] : gameCatalogs.actors.definitions)
        {
            if (definition.platformer.has_value() && definition.senses.has_value())
            {
                navigation.namedProfiles.push_back(
                    {name,
                     {definition.bodySize,
                      definition.platformer.value_or(PlatformerMovementConfig{}),
                      simulationStepSeconds}});
            }
        }

        // Construct the analytical structural overlay
        DebugOverlay overlay = makeDebugOverlay(
            level.world,
            level.map,
            cameraControllerValue(),
            atlasWidth,
            simulationStepSeconds,
            navigation,
            framerate);

        // Map string metadata signatures to entity definitions for inspection panels
        for (ActorDebugInfo& actor : overlay.actors)
        {
            const auto definition = level.actorDefinitionNames.find(actor.id.value);
            if (definition != level.actorDefinitionNames.end())
            {
                actor.definitionName = definition->second;
            }
        }
        return overlay;
    }

    // Retrieves the current health state of the player actor, throwing if the player doesn't exist
    // or has no health
    Health Game::playerHealth() const
    {
        const Actor* player = level.world.findActor(level.world.playerId());
        if (player == nullptr || !player->health.has_value())
        {
            throw std::logic_error("The player is missing its health");
        }
        return *player->health;
    }

    // Helper wrapper to extract the underlying viewport Camera data configuration out of the active
    // controller
    Camera Game::currentCamera() const
    {
        return cameraControllerValue().camera;
    }

    // Returns a reference to the global HUD icon textures and styling layout configurations
    const HudIcons& Game::hudIcons() const
    {
        return gameCatalogs.hudIcons;
    }

    // Safely reads the player's underlying carrying capacity and item stacks wrapper from the
    // physics world
    const Inventory& Game::playerInventory() const
    {
        const Actor* player = level.world.findActor(level.world.playerId());
        if (player == nullptr || !player->inventory.has_value())
        {
            throw std::logic_error("The player is missing its inventory");
        }
        return *player->inventory;
    }

    // Look up static item characteristics metadata (e.g., icons, naming, behaviors) by item
    // configuration ID
    const ItemDefinition& Game::itemDefinition(int id) const
    {
        return level.world.itemDefinition(id);
    }

    // Processes slot usage actions requested directly from screen UI triggers (e.g., consumption,
    // equipping)
    void Game::useInventoryItem(std::size_t slot)
    {
        if (gameComplete)
        {
            return;
        }
        WorldRequests requests;
        requests.useItem(level.world.playerId(), slot);
        // UI requests are applied while paused without advancing movement, combat or timers.
        applyWorldRequests(level.world, requests);
    }

    // Converts a screen position to world space coordinates and attempts to destroy a tile at that
    // grid cell location
    bool Game::breakTileAt(glm::vec2 internalPosition)
    {
        const glm::vec2 world = screenToWorld(currentCamera(), internalPosition);
        // Validate that the targeted coordinate resides completely within the structural map
        // borders
        if (world.x < 0.0F || world.y < 0.0F || world.x >= level.map.pixelWidth() ||
            world.y >= level.map.pixelHeight())
        {
            return false;
        }
        return level.map.breakTile(cellAt(level.map.tileSize(), world));
    }

    // Resets the win/loss runtime flags and re-creates the first catalog sequence level cleanly
    void Game::restart()
    {
        gameComplete = false;
        replaceLevel(levelCatalog.startLevel, composePlayer(gameCatalogs, atlasTextureId));
    }

    // Gets the numerical layout counter value matching the currently active level instance
    int Game::levelNumber() const
    {
        return level.number;
    }

    // Checks whether the final victory conditions have been verified across the application
    // environment
    bool Game::complete() const
    {
        return gameComplete;
    }

    // Computes the two-dimensional screen space coordinate projection matching the topmost midpoint
    // of the map exit door
    std::optional<glm::vec2> Game::levelExitScreenPosition() const
    {
        const auto& levelExit = level.world.exit();
        if (!levelExit.has_value())
        {
            return std::nullopt;
        }
        return worldToScreen(currentCamera(), topCenterOf(levelExit.value().bounds));
    }

    // Evaluates flash duration thresholds to return a hint sprite (e.g., key item) if a locked door
    // was bumped recently
    std::optional<Sprite> Game::lockedExitHintIcon() const
    {
        const auto& levelExit = level.world.exit();
        if (!levelExit.has_value() || !levelExit->requirement.has_value())
        {
            return std::nullopt;
        }
        const std::optional<float> sinceTouch =
            level.world.secondsSince(levelExit->lastLockedTouchTimeSeconds);
        // Discard rendering properties if the flash notification display cooldown has run out
        if (!sinceTouch.has_value() || *sinceTouch > LockedExitHintSeconds)
        {
            return std::nullopt;
        }
        return level.world.itemDefinition(levelExit->requirement->item).icon;
    }

    // Non-const accessor to extract the CameraController object wrapper instance safely
    CameraController& Game::cameraControllerValue()
    {
        if (!cameraController.has_value())
        {
            throw std::logic_error("The game camera is not initialised");
        }
        return *cameraController;
    }

    // Const-qualified accessor to extract the CameraController object wrapper safely for read-only
    // lookups
    const CameraController& Game::cameraControllerValue() const
    {
        if (!cameraController.has_value())
        {
            throw std::logic_error("The game camera is not initialised");
        }
        return *cameraController;
    }
}
