/**
 * @file        debug_overlay.cpp
 * @brief       Declares data structures and capture functions for gathering comprehensive, 
 *              view-bounded simulation snapshots used by diagnostic tools.
 *
 * @author      Mohd Fadzuli
 * @author      Adapted by: Toh Da Jun
 * @copyright   Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#include "debug_overlay.hpp"

#include "navigation_debug.hpp"

#include <cstddef>
#include <optional>
#include <stdexcept>
#include <vector>

#include <glm/vec2.hpp>

#include "simple_platformer/actor/actor.hpp"
#include "simple_platformer/actor/actor_id.hpp"
#include "simple_platformer/combat/attack_system.hpp"
#include "simple_platformer/combat/combat.hpp"
#include "simple_platformer/math/aabb.hpp"
#include "simple_platformer/math/coordinates.hpp"
#include "simple_platformer/math/validation.hpp"
#include "simple_platformer/movement/platformer_movement.hpp"
#include "simple_platformer/navigation/navigation_path.hpp"
#include "simple_platformer/navigation/path_follower.hpp"
#include "simple_platformer/npc/npc.hpp"
#include "simple_platformer/render/actor_sprite.hpp"
#include "simple_platformer/render/animation.hpp"
#include "simple_platformer/render/camera.hpp"
#include "simple_platformer/render/sprite.hpp"
#include "simple_platformer/world/tile_map.hpp"
#include "simple_platformer/world/world.hpp"
#include "simple_platformer/world/pickup.hpp"

namespace simple_platformer
{
    namespace
    {
        // Categorises the actor object wrapper instance to apply distinct debug tracking visual
        // modes
        ActorDebugKind kindOf(const Actor& actor, ActorId playerId)
        {
            // Explicit lookup match to flag the human-controlled player entity
            if (actor.id == playerId)
            {
                return ActorDebugKind::Player;
            }
            // If the structure possesses a valid state machine brain component, classify it as an
            // NPC
            if (actor.brain.has_value())
            {
                return ActorDebugKind::Npc;
            }
            // Fallback categorisation for static, interactive, or passive entities
            return ActorDebugKind::Actor;
        }

        // Extracts structural frame bounds and derives 1-based tile index locations from the
        // texture sheet
        // Calculates explicit atlas index coordinates and layout bounds for rendering debug text
        // markers
        ActorSpriteDebugInfo spriteDebugInfo(
            const Actor& actor,
            const Sprite& sprite,
            float atlasWidth)
        {
            // Sanity verification constraints to intercept corrupted or missing texture dimension
            // values early
            if (!isFiniteNonNegative(sprite.region.position) ||
                !isFinitePositive(sprite.region.size) || atlasWidth < sprite.region.size.x)
            {
                throw std::logic_error("Debug overlay requires a valid sprite region");
            }

            // Extract the actual rendering boundary box calculated from the actor's current
            // orientation and position parameters
            const Aabb bounds = placeActorSprite(actor).visible;

            // Calculate how many distinct sprite column slots exist horizontally across the main
            // texture atlas sheet
            const std::size_t atlasColumns =
                static_cast<std::size_t>(atlasWidth / sprite.region.size.x);

            // Convert the absolute pixel horizontal offset down into a discrete 0-indexed column
            // grid coordinate
            const std::size_t atlasColumn =
                static_cast<std::size_t>(sprite.region.position.x / sprite.region.size.x);

            // Convert the absolute pixel vertical offset down into a discrete 0-indexed row grid
            // coordinate
            const std::size_t atlasRow =
                static_cast<std::size_t>(sprite.region.position.y / sprite.region.size.y);

            // Returns the 1-based structural tile grid array index alongside visual bounding
            // parameters (Calculated via: (Row * TotalColumns) + Column + 1 offset for standard
            // human-readable indexing)
            return {bounds, atlasRow * atlasColumns + atlasColumn + 1, sprite.region.position};
        }

        // Simulates cinematic airborne trajectories to build node paths preview lines for jumping
        // AI entities
        std::vector<glm::vec2> sampleAirborneTraversal(
            const Actor& actor,
            const TileMap& map,
            glm::vec2 startFeet,
            const Waypoint& waypoint,
            float stepSeconds)
        {
            // Cease path generation if the character profile lacks jump curves or platforming
            // parameters
            if (!actor.platformerMovement.has_value())
            {
                return {};
            }
            // Run kinematic equations loops across discrete time deltas to extrapolate the velocity
            // arcs
            return sampleAirborneProgram(
                map,
                startFeet,
                actor.body.bounds.size,
                actor.platformerMovement.value().config,
                waypoint.traversal,
                waypoint.inputs,
                stepSeconds);
        }

        // Unpacks pathfinding nodes chains and constructs linear visual connection segments for
        // rendering overlays
        PathFollowerDebugInfo pathFollowerDebugInfo(
            const Actor& actor,
            const TileMap& map,
            const PathFollower& follower,
            float stepSeconds)
        {
            PathFollowerDebugInfo info;
            info.goalFeet =
                follower.goal; // Track the ultimate absolute destination target node coordinate
            if (!follower.path.has_value())
            {
                return info; // Exit early with target details if no active operational route is
                             // calculated
            }

            info.hasPath = true;
            info.nextStep = follower.nextStep;
            info.stepCount = follower.path->waypoints.size();
            info.connections.reserve(
                info.stepCount); // Pre-allocate vector sizes to avoid memory fragmentation triggers

            glm::vec2 from = follower.path->startFeet;
            // Iterate down the structural itinerary nodes to populate tracking properties
            for (std::size_t index = 0; index < follower.path->waypoints.size(); ++index)
            {
                const Waypoint& waypoint = follower.path->waypoints[index];

                // Pack node characteristics (grouping historical lines, active vectors, and
                // airborne arcs)
                info.connections.push_back(
                    {from,
                     waypoint.feet,
                     waypoint.traversal,
                     index <
                         follower.nextStep, // Node tracking has already been successfully traversed
                     index == follower.nextStep, // Node represents the current immediate targeted
                                                 // destination vector
                     sampleAirborneTraversal(actor, map, from, waypoint, stepSeconds)});

                from = waypoint.feet; // Shift baseline tracking coordinates forward to evaluate the
                                      // consecutive step
            }
            return info;
        }

        // Compiles sensory awareness attributes, line-of-sight tracking data, and AI memory
        // timeouts
        SensorDebugInfo sensorDebugInfo(
            const Actor& actor,
            const NpcBrain& brain,
            const NpcSenses& senses,
            const Actor* player)
        {
            SensorDebugInfo info;
            info.observerCenter =
                centerOf(actor.body.bounds); // The raycasting vantage origin point of the NPC
            info.noticeDistance = senses.noticeDistance; // Max operational vision radius boundary
            info.memoryRemaining =
                brain.targetMemoryRemaining; // Cooldown timer tracking alert state decay

            // Check if line-of-sight raycasts confirm a target is visible, alive, and hostile
            if (actor.perception.has_value() && actor.perception->targetVisible &&
                player != nullptr && player->life == LifeState::Alive &&
                areOpponents(actor.team, player->team))
            {
                // Active detection trace: lock targeted center line vectors onto live target
                // coordinate points
                info.visibleTargetCenter = centerOf(player->body.bounds);
            }
            // Fallback to displaying a memory trace marker if the target broke line-of-sight but
            // memory persists
            else if (brain.target.has_value() && brain.targetMemoryRemaining > 0.0F)
            {
                info.rememberedTargetFeet =
                    brain.lastKnownTargetFeet; // Last known tracking position coordinates
            }
            return info;
        }

        // Culls actors present in the physics loop to isolate only elements intersecting the active
        // camera frame bounds
        std::vector<const Actor*> actorsInView(const World& world, const Aabb& view)
        {
            std::vector<const Actor*> shown;
            for (const Actor& actor : world.actors())
            {
                // Spatial AABB intersection test ensures off-screen calculations overhead drops
                // significantly
                if (overlaps(actor.body.bounds, view))
                {
                    shown.push_back(&actor);
                }
            }
            return shown;
        }

        // Bounds for the tile-break hint, or nothing without a cursor over a breakable tile.
        std::optional<Aabb> breakableCellUnderCursor(
            const TileMap& map,
            std::optional<glm::vec2> cursorWorld)
        {
            // Drop out cleanly if the user pointer target parameter holds no coordinate value data
            if (!cursorWorld.has_value())
            {
                return std::nullopt;
            }

            const glm::vec2 cursor = cursorWorld.value_or(glm::vec2{0.0F, 0.0F});
            // Bounds-check the vector positions against physical tile boundary grid dimensions
            if (cursor.x < 0.0F || cursor.y < 0.0F || cursor.x >= map.pixelWidth() ||
                cursor.y >= map.pixelHeight())
            {
                return std::nullopt;
            }

            // Convert world space pixel coordinates back into standard layout grid map cells
            // (Column, Row)
            const Cell cell = cellAt(map.tileSize(), cursor);
            // Verify structural composition to determine if the tile definition properties contain
            // break options
            if (!map.definitionAt(cell).breaksIntoTileId.has_value())
            {
                return std::nullopt; // Non-destructible solid block asset or air empty gap cell;
                                     // exit cleanly
            }

            // Generate absolute bounding box configurations outlining the target cell structure
            // boundaries
            const auto tileSize = static_cast<float>(map.tileSize());
            return Aabb{
                {static_cast<float>(cell.x) * tileSize, static_cast<float>(cell.y) * tileSize},
                {tileSize, tileSize}};
        }
    }

    // Constructs the complete unified diagnostics state model snapshot parsed directly out of
    // active systems
    DebugOverlay makeDebugOverlay(
        const World& world,
        const TileMap& map,
        const CameraController& cameraController,
        float atlasWidth,
        float simulationStepSeconds,
        const NavigationDebugView& navigation,
        std::optional<float> framerate)
    {
        // Intercept invalid structural math definitions early to guarantee predictable display
        // layouts
        if (!isFinitePositive(simulationStepSeconds))
        {
            throw std::invalid_argument(
                "Debug overlay simulation step must be finite and positive");
        }
        if (!isFinitePositive(atlasWidth))
        {
            throw std::invalid_argument("Debug overlay atlas width must be positive and finite");
        }

        DebugOverlay scene;

        // 1. Capture primary viewing metrics and camera controller constraints
        scene.cameraBounds = {
            cameraController.camera.position, cameraController.camera.viewportSize};

        // Calculate the central dead-zone bounding frames relative to current viewport sizes
        scene.cameraDeadZone = {
            cameraController.camera.position +
                (cameraController.camera.viewportSize - cameraController.deadZoneSize) * 0.5F,
            cameraController.deadZoneSize};

        // --- PHASE 2: CALCULATE VISIBILITY FRUSTUM WITH A 1-TILE OVERSIZE PADDING REGION ---
        const auto margin = static_cast<float>(map.tileSize());
        const Aabb view{
            scene.cameraBounds.topLeft - glm::vec2{margin, margin},
            scene.cameraBounds.size + glm::vec2{margin, margin} * 2.0F};

        // Perform space culling tests to discard offscreen tracking checks early
        const std::vector<const Actor*> shown = actorsInView(world, view);
        scene.actors.reserve(
            shown.size()); // Pre-allocate storage blocks to prevent runtime resizing splits
        const Actor* player = world.findActor(world.playerId());

        // --- PHASE 3: EXTRACT AND STRUCTURE PROPERTY OVERLAYS FOR EVERY ON-SCREEN ACTOR ---
        for (const Actor* shownActor : shown)
        {
            const Actor& actor = *shownActor;
            ActorDebugInfo info;
            info.id = actor.id;
            info.kind =
                kindOf(actor, world.playerId()); // Categorize (Player vs NPC vs generic entity)
            info.collider = actor.body.bounds;

            // Extract visual texture mappings if valid regions are bound
            if (actor.sprite.has_value())
            {
                info.sprite = spriteDebugInfo(actor, actor.sprite.value(), atlasWidth);
            }
            // Track character structural state tokens configurations
            if (actor.animator.has_value())
            {
                info.animation = actor.animator->current;
            }
            // Populate finite state behavior details if evaluating an automated agent unit
            if (actor.brain.has_value())
            {
                info.npcState = actor.brain->state;
                info.npcTactic = actor.brain->tactic;
            }
            // Extract AI routing node meshes and path planning vectors configurations
            if (actor.pathFollower.has_value())
            {
                info.pathFollower = pathFollowerDebugInfo(
                    actor, map, actor.pathFollower.value(), simulationStepSeconds);
            }
            // Map sight traces and detection parameters relative to player targets locations
            if (actor.brain.has_value() && actor.senses.has_value())
            {
                info.sensor =
                    sensorDebugInfo(actor, actor.brain.value(), actor.senses.value(), player);
            }
            // Compile linear destination landmarks tracking patrol itineraries
            if (actor.patrol.has_value())
            {
                const Patrol& patrol = actor.patrol.value();
                info.patrol =
                    PatrolDebugInfo{patrol.firstFeet, patrol.secondFeet, patrol.headingToSecond};
            }
            // Extract immediate physical melee combat hit-zones while aggressive action phases are
            // executing
            if (actor.bite.has_value() && actor.bite->phase == BitePhase::Active)
            {
                info.biteHitbox = biteHitbox(actor.body.bounds, actor.bite.value(), actor.facing);
            }
            scene.actors.push_back(info);
        }

        // --- PHASE 4: EXTRACT PASSIVE WORLD OBJECT METRICS ---
        // Capture active combat tracking parameters that intersect the visible grid camera spaces
        for (const Projectile& projectile : world.projectiles())
        {
            if (overlaps(projectile.bounds, view))
            {
                scene.projectiles.push_back(
                    {projectile.bounds, projectile.lifetimeRemaining, projectile.owner});
            }
        }

        // Gather dropped items data attributes and map matching asset names text identifiers
        for (const Pickup& pickup : world.pickups())
        {
            if (overlaps(pickup.body.bounds, view))
            {
                scene.pickups.push_back(
                    {pickup.body.bounds, world.itemDefinition(pickup.stack.item).name});
            }
        }

        // --- PHASE 5: SYSTEM DIAGNOSTICS LOG INTERFACES MAPS ASSEMBLY ---
        // Project spatial nodes layout connections details to visualize pathfinding logic layers
        scene.navigationConnections =
            makeNavigationConnectionsDebugInfo(world, map, simulationStepSeconds, navigation, view);

        // Map contextual highlighting boxes targeting destructible cells right under user
        // crosshairs
        scene.breakableCellUnderCursor = breakableCellUnderCursor(map, navigation.cursorWorld);

        scene.framerate = framerate.value();
        return scene;
    }
}
