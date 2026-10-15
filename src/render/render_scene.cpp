/**
 * @file render_scene.hpp
 * @brief Transforms simulation metrics into unified screen-space sprite drawing command matrices.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#include "simple_platformer/render/render_scene.hpp"

#include <algorithm>
#include <cmath>
#include <optional>

#include <glm/vec2.hpp>

#include "simple_platformer/actor/actor.hpp"
#include "simple_platformer/combat/combat.hpp"
#include "simple_platformer/math/aabb.hpp"
#include "simple_platformer/math/coordinates.hpp"
#include "simple_platformer/world/level_exit.hpp"
#include "simple_platformer/world/pickup.hpp"
#include "simple_platformer/movement/platformer_movement.hpp"
#include "simple_platformer/render/actor_sprite.hpp"
#include "simple_platformer/render/camera.hpp"
#include "simple_platformer/render/sprite.hpp"
#include "simple_platformer/world/tile_map.hpp"
#include "simple_platformer/world/world.hpp"

namespace simple_platformer
{
    namespace
    {
        // Global visual timing scalars and animation configurations
        constexpr float DeathFadeSeconds = 0.2F;
        constexpr float HitFlashSeconds = 0.1F;
        constexpr float HitFlashAmount = 0.1F;
        // The door flashes white as it opens and the player fades into it; both follow
        // how far through ExitOpenSeconds the opening is.
        constexpr float ExitOpenFlashAmount = 0.5F;
        constexpr float PickupBobHeight = 2.0F;
        constexpr float PickupBobPeriodSeconds = 1.0F;
        constexpr int PickupBobPhaseCount = 4;
        constexpr float ProjectileBurstFinalScale = 2.0F;
        constexpr float RadiansPerCycle = 6.28318530717958647692F;

        // Calculates transparency curves based on an entity's remaining decay time after dying
        float actorOpacity(const Actor& actor)
        {
            if (actor.life == LifeState::Alive)
            {
                return 1.0F;
            }
            return std::clamp(actor.deathTimeRemaining / DeathFadeSeconds, 0.0F, 1.0F);
        }

        // Resolves white damage flash mix amounts if an entity sustained a hit within the flash
        // window threshold
        float actorWhiteFlashAmount(const World& world, const Actor& actor)
        {
            const std::optional<float> sinceDamage =
                world.secondsSince(actor.lastDamageTimeSeconds);
            const bool wasRecentlyDamaged =
                sinceDamage.has_value() && *sinceDamage < HitFlashSeconds;
            return wasRecentlyDamaged ? HitFlashAmount : 0.0F;
        }

        // Computes the normalized progression ratio (0.0 to 1.0) of the stage-exit portal
        // transition state 0 until the exit is entered, 1 once it has fully opened.
        float exitOpenProgress(const World& world)
        {
            const auto& exit = world.exit();
            if (!exit.has_value())
            {
                return 0.0F;
            }
            const std::optional<float> sinceOpened = world.secondsSince(exit->openedTimeSeconds);
            if (!sinceOpened.has_value())
            {
                return 0.0F;
            }
            return std::clamp(*sinceOpened / ExitOpenSeconds, 0.0F, 1.0F);
        }

        // Generates smooth vertical offset values using cosine wave transformations to make loot
        // items levitate
        float pickupVerticalOffset(float animationTime)
        {
            const float cycleRadians = animationTime * RadiansPerCycle / PickupBobPeriodSeconds;
            return -PickupBobHeight * 0.5F * (1.0F - std::cos(cycleRadians));
        }

        // Derives an initial phase shift using spatial coordinates to prevent level items from
        // bobbing in absolute unison
        float pickupPhaseOffset(int tileSize, const Aabb& bounds)
        {
            // Spread level-start pickups across four phases instead of bobbing in lockstep.
            const Cell cell = cellAt(tileSize, bounds.topLeft);
            int phaseIndex = (cell.x + cell.y) % PickupBobPhaseCount;
            if (phaseIndex < 0)
            {
                phaseIndex += PickupBobPhaseCount;
            }
            return static_cast<float>(phaseIndex) * PickupBobPeriodSeconds /
                   static_cast<float>(PickupBobPhaseCount);
        }

        // View-frustum culling system: pushes visible structural tile meshes into the render
        // pipeline scene buffer
        void appendTiles(
            RenderScene& scene,
            const TileMap& map,
            int tileTextureId,
            const Camera& camera)
        {
            const float tileSize = static_cast<float>(map.tileSize());
            // Map spatial viewport corners to pixel columns and rows to completely bypass
            // out-of-frame blocks
            const int firstColumn =
                std::max(0, static_cast<int>(std::floor(camera.position.x / tileSize)));
            const int lastColumn = std::min(
                map.width() - 1,
                static_cast<int>(
                    std::ceil((camera.position.x + camera.viewportSize.x) / tileSize)) -
                    1);
            const int firstRow =
                std::max(0, static_cast<int>(std::floor(camera.position.y / tileSize)));
            const int lastRow = std::min(
                map.height() - 1,
                static_cast<int>(
                    std::ceil((camera.position.y + camera.viewportSize.y) / tileSize)) -
                    1);

            // Scan through rows and columns within view parameters to append active nodes
            for (int row = firstRow; row <= lastRow; ++row)
            {
                for (int column = firstColumn; column <= lastColumn; ++column)
                {
                    const Cell cell{column, row};
                    if (map.tileAt(cell) == 0)
                    {
                        continue; // Passable airspace tile
                    }

                    const glm::vec2 worldPosition = {
                        static_cast<float>(column * map.tileSize()),
                        static_cast<float>(row * map.tileSize())};
                    SpriteDrawCommand command;
                    command.textureId = tileTextureId;
                    command.position = worldToScreen(camera, worldPosition);
                    command.size = {tileSize, tileSize};
                    command.source = map.definitionAt(cell).sprite;
                    command.flipHorizontal = false;
                    scene.sprites.push_back(command);
                }
            }
        }

        // Calculates hover translations and appends visible collectible items into the draw queue
        void appendPickups(
            RenderScene& scene,
            const TileMap& map,
            const World& world,
            const Camera& camera)
        {
            for (const Pickup& pickup : world.pickups())
            {
                const float pickupVisibility = pickup.screenVisibility.value_or(1.0F);
                if (pickupVisibility <= 0.0F)
                {
                    continue;
                }
                const Sprite& sprite =
                    pickup.sprite ? *pickup.sprite : world.itemDefinition(pickup.stack.item).icon;
                Aabb bounds = spriteBounds(pickup.body.bounds, sprite);
                bounds.topLeft.y += pickupVerticalOffset(
                    static_cast<float>(world.simulationTimeSeconds()) +
                    pickupPhaseOffset(map.tileSize(), pickup.body.bounds));
                SpriteDrawCommand command;
                command.textureId = sprite.textureId;
                command.position = worldToScreen(camera, bounds.topLeft);
                command.size = bounds.size;
                command.source = sprite.region;
                command.flipHorizontal = false;
                command.rotationRadians = 0.0F;
                command.opacity = pickupVisibility;
                scene.sprites.push_back(command);
            }
        }

        // Configures cutscene overlays and enqueues the exit gateway structure
        void appendExit(RenderScene& scene, const World& world, const Camera& camera)
        {
            const auto& exit = world.exit();
            if (!exit.has_value())
            {
                return;
            }

            const LevelExit& levelExit = exit.value();
            if (!levelExit.sprite.has_value())
            {
                return;
            }

            const Sprite& sprite = levelExit.sprite.value();
            const Aabb bounds = spriteBounds(levelExit.bounds, sprite);
            // Blend out flash metrics dynamically as completion percentages grow close to 1.0
            const float flash = levelExit.openedTimeSeconds.has_value()
                                    ? (1.0F - exitOpenProgress(world)) * ExitOpenFlashAmount
                                    : 0.0F;
            SpriteDrawCommand command;
            command.textureId = sprite.textureId;
            command.position = worldToScreen(camera, bounds.topLeft);
            command.size = bounds.size;
            command.source = sprite.region;
            command.flipHorizontal = false;
            command.rotationRadians = 0.0F;
            command.opacity = 1.0F;
            command.whiteFlashAmount = flash;
            scene.sprites.push_back(command);
        }

        // Resolves directional flipping flags, alpha decays, and damage color masks for character sprites
        void appendActors(RenderScene& scene, const World& world, const Camera& camera)
        {
            const float playerOpacity = 1.0F - exitOpenProgress(world);
            for (const Actor& actor : world.actors())
            {
                if (!actor.sprite.has_value())
                {
                    continue;
                }
                const float actorVisibility = actor.screenVisibility.value_or(1.0F);
                const bool isPlayer = actor.id == world.playerId();
                if (!isPlayer && actorVisibility <= 0.0F)
                {
                    continue;
                }

                const ActorSpritePlacement placement = placeActorSprite(actor);
                SpriteDrawCommand command;
                command.textureId = actor.sprite->textureId;
                command.position = worldToScreen(camera, placement.drawn.topLeft);
                command.size = placement.drawn.size;
                command.source = actor.sprite->region;
                command.flipHorizontal = placement.flipHorizontal;
                command.rotationRadians = placement.rotationRadians;
                command.opacity =
                    actorOpacity(actor) * (isPlayer ? playerOpacity : actorVisibility);
                command.whiteFlashAmount = actorWhiteFlashAmount(world, actor);
                command.shadeAmount =
                    isPlayer ? (1.0F - actorVisibility) * PlayerConcealedShade : 0.0F;
                scene.sprites.push_back(command);
            }
        }

        // Populates the rendering queue with active projectiles, orienting them along their
        // velocity trajectories
        void appendProjectiles(RenderScene& scene, const World& world, const Camera& camera)
        {
            for (const Projectile& projectile : world.projectiles())
            {
                // Align the center of the visual sprite perfectly with the midpoint of the physics
                // collider
                const glm::vec2 projectileCenter = centerOf(projectile.bounds);
                const glm::vec2 spritePosition =
                    projectileCenter - projectile.sprite.region.size * 0.5F;

                // Derive the tracking orientation angle using the horizontal and vertical velocity
                // offsets
                const float rotationRadians =
                    std::atan2(projectile.velocity.y, projectile.velocity.x);

                SpriteDrawCommand command;
                command.textureId = projectile.sprite.textureId;
                command.position = worldToScreen(camera, spritePosition);
                command.size = projectile.sprite.region.size;
                command.source = projectile.sprite.region;
                command.flipHorizontal = false;
                command.rotationRadians = rotationRadians;
                scene.sprites.push_back(command);
            }
        }

        // Processes dynamic scale updates and transparency fades for expanding explosion/impact
        // burst particle overlays
        void appendProjectileBursts(RenderScene& scene, const World& world, const Camera& camera)
        {
            for (const ProjectileBurst& burst : world.projectileBursts())
            {
                // Calculate progress metrics to handle life cycles accurately
                const float remainingFraction =
                    std::clamp(burst.lifetimeRemaining / burst.duration, 0.0F, 1.0F);
                const float progress = 1.0F - remainingFraction;

                // Interpolate texture dimensions outwards over time to simulate a swelling
                // shockwave effect
                const float scale = 1.0F + progress * (ProjectileBurstFinalScale - 1.0F);
                const glm::vec2 size = burst.sprite.region.size * scale;
                const glm::vec2 position = burst.center - size * 0.5F;

                // Align directional assets with the impact vector trajectory path
                const float rotationRadians = std::atan2(burst.direction.y, burst.direction.x);

                SpriteDrawCommand command;
                command.textureId = burst.sprite.textureId;
                command.position = worldToScreen(camera, position);
                command.size = size;
                command.source = burst.sprite.region;
                command.flipHorizontal = false;
                command.rotationRadians = rotationRadians;
                command.opacity =
                    remainingFraction; // Smoothly dissolve alpha levels to zero as expiration nears
                scene.sprites.push_back(command);
            }
        }
    }

    // Constructs the final comprehensive render frame by appending layers in a strict back-to-front
    // sorting order
    RenderScene buildRenderScene(
        const TileMap& map,
        int tileTextureId,
        const Camera& camera,
        const World& world)
    {
        RenderScene scene;

        // Layer 1: Background environment grid geometry
        appendTiles(scene, map, tileTextureId, camera);

        // Layer 2: Floating collectible loot entities and items
        appendPickups(scene, map, world, camera);

        // Layer 3: Level completion gateway/portal structures
        appendExit(scene, world, camera);

        // Layer 4: Living entities, characters, and monsters
        appendActors(scene, world, camera);

        // Layer 5: Active particle hazards and bullet instances
        appendProjectiles(scene, world, camera);

        // Layer 6: Foreground particle bursts and explosions
        appendProjectileBursts(scene, world, camera);

        return scene;
    }
}
