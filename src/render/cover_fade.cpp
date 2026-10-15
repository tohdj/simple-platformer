/**
 * @file cover_fade.cpp
 * @brief Handles ambient visual fading mechanics, stealth reveals, and opacity scaling for objects
 * in cover.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#include "simple_platformer/render/cover_fade.hpp"

#include <algorithm>
#include <optional>

#include <glm/common.hpp>
#include <glm/vec2.hpp>

#include "simple_platformer/actor/actor.hpp"
#include "simple_platformer/combat/combat.hpp"
#include "simple_platformer/math/aabb.hpp"
#include "simple_platformer/math/coordinates.hpp"
#include "simple_platformer/math/validation.hpp"
#include "simple_platformer/npc/npc.hpp"
#include "simple_platformer/world/pickup.hpp"
#include "simple_platformer/world/sight.hpp"
#include "simple_platformer/world/tile_map.hpp"
#include "simple_platformer/world/world.hpp"

namespace simple_platformer
{
    namespace
    {
        // Cover is measured as a fraction of body area. Show fully up to the first limit
        // and hide from the second. A 12 by 20 body in 16-pixel-tall grass is 80% covered,
        // so these limits hide it without requiring the whole body to fit in grass.
        constexpr float ShownUpToCover = 0.5F;
        constexpr float HiddenFromCover = 0.75F;

        // Computes the geometric intersection ratio between an entity's AABB bounding box and
        // line-of-sight blocking tiles
        float fractionInCover(const TileMap& map, const Aabb& bounds)
        {
            const float area = bounds.size.x * bounds.size.y;
            if (area <= 0.0F)
            {
                return 0.0F;
            }

            const glm::vec2 boundsEnd = bounds.topLeft + bounds.size;
            const Cell first = cellAt(map.tileSize(), bounds.topLeft);
            const Cell last = cellAt(map.tileSize(), boundsEnd);
            float coveredArea = 0.0F;
            for (int row = first.y; row <= last.y; ++row)
            {
                for (int column = first.x; column <= last.x; ++column)
                {
                    if (!map.blocksSight({column, row}))
                    {
                        continue;
                    }
                    const glm::vec2 cellStart = cellCorner(map.tileSize(), {column, row});
                    const glm::vec2 cellEnd = cellStart + static_cast<float>(map.tileSize());
                    const glm::vec2 overlap = glm::max(
                        glm::min(boundsEnd, cellEnd) - glm::max(bounds.topLeft, cellStart),
                        glm::vec2{0.0F, 0.0F});
                    coveredArea += overlap.x * overlap.y;
                }
            }
            return std::min(coveredArea / area, 1.0F);
        }

        // Determines a target's final scalar visibility factor based on raycasts and tile layout
        // cover metrics How visible the target is to the viewer, from 1 for fully visible to 0 for
        // hidden. The viewer sees fully whatever it has a line of sight to; otherwise cover
        // decides. Walls alone never hide a target in the open. Without a viewer, cover alone
        // decides.
        float visibility(const TileMap& map, std::optional<glm::vec2> viewer, const Aabb& target)
        {
            const float covered = fractionInCover(map, target);
            if (covered <= ShownUpToCover)
            {
                return 1.0F; // Target is out in the open or barely covered
            }
            if (viewer.has_value() && lineOfSight(map, *viewer, centerOf(target)))
            {
                return 1.0F; // Direct line-of-sight raycast bypasses partial camouflage cover
                             // blockages
            }

            // Linearly interpolate the stealth blending value between the visible and fully
            // concealed limits
            return std::clamp(
                (HiddenFromCover - covered) / (HiddenFromCover - ShownUpToCover), 0.0F, 1.0F);
        }

        // Evaluates whether a player character recently fired a weapon, exposing their position
        // temporarily
        bool firedWithinRevealWindow(const World& world, const Actor& player)
        {
            if (!player.rangedWeapon.has_value())
            {
                return false;
            }
            const std::optional<float> sinceShot =
                world.secondsSince(player.rangedWeapon->lastFiredTimeSeconds);
            return sinceShot.has_value() && *sinceShot < ShotRevealSeconds;
        }

        // Checks if any active computer-controlled brain component is successfully tracking the
        // player What the latest senses update decided: whether any NPC sees the player now.
        bool playerSeenByAnyNpc(const World& world)
        {
            for (const Actor& actor : world.actors())
            {
                if (actor.brain.has_value() && actor.perception.has_value() &&
                    actor.perception->targetVisible && actor.brain->target == world.playerId())
                {
                    return true;
                }
            }
            return false;
        }

        // Resolves the ultimate target visibility destination state for the player character
        float playerTarget(const TileMap& map, const World& world, const Actor& player)
        {
            // Forcibly reveal the player sprite if they are actively spotted or shooting
            if (playerSeenByAnyNpc(world) || firedWithinRevealWindow(world, player))
            {
                return 1.0F;
            }
            return visibility(map, std::nullopt, player.body.bounds);
        }

        // Smoothly interpolates an optional floating opacity track value towards a target
        // coefficient over time
        void fadeTowards(std::optional<float>& shown, float target, float deltaTime)
        {
            if (!shown.has_value())
            {
                shown = target; // Initialize tracker if no entry history exists
                return;
            }
            const float mostChange = deltaTime / CoverFadeSeconds;
            *shown += std::clamp(target - *shown, -mostChange, mostChange);
        }
    }

    // Updates alpha tracking multipliers sequentially across all actors and pickups based on
    // current cover metrics
    void updateCoverFades(const TileMap& map, World& world, float deltaTime)
    {
        requireSeconds(deltaTime, "Cover fades time step");

        // Locate the primary player character to act as the central perspective point for
        // line-of-sight checks
        const Actor* player = world.findActor(world.playerId());
        const std::optional<glm::vec2> viewer =
            player != nullptr ? std::optional(centerOf(player->body.bounds)) : std::nullopt;

        // 1. Process fade transitions for all active actor instances
        for (Actor& actor : world.actors())
        {
            fadeTowards(
                actor.screenVisibility,
                actor.id == world.playerId() ? playerTarget(map, world, actor)
                                             : visibility(map, viewer, actor.body.bounds),
                deltaTime);
        }

        // 2. Process fade transitions for all floating pickup items
        for (Pickup& pickup : world.pickups())
        {
            fadeTowards(
                pickup.screenVisibility, visibility(map, viewer, pickup.body.bounds), deltaTime);
        }
    }
}
