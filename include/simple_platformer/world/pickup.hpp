/**
 * @file pickup.hpp
 * @brief Logic, structure validation, and physics updates for collectible item pickups in the
 * world.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#pragma once

#include <optional>

#include "simple_platformer/inventory/item.hpp"
#include "simple_platformer/physics/body.hpp"
#include "simple_platformer/render/sprite.hpp"

namespace simple_platformer
{
    /**
     * @brief Represents a collectible item bundle dropping or resting inside the game world.
     */
    struct Pickup
    {
        Body body; /**< Physical translation box tracking gravity calculations and environmental
                      cell collisions. */
        ItemStack stack; /**< Specific item definition bundle and count quantity payload carried by
                            this pickup. */
        std::optional<Sprite> sprite =
            std::nullopt; /**< Optional visual model variant overriding standard default inventory
                             texture sheets. */
        std::optional<float> screenVisibility =
            std::nullopt; /**< Interpolated blending value (0.0 to 1.0) defining fog or reveal
                             presentation layers. Unset prior to initial draw updates. */
    };

    class TileMap;
    class World;
    class WorldRequests;

    /**
     * @brief Asserts structural integrity metrics for isolated pickup data models.
     * @note Companion validation passes inside World boundaries further cross-check global item
     * configuration tables.
     * @param pickup The targeted container footprint structure to verify.
     */
    void validatePickup(const Pickup& pickup);

    /**
     * @brief Integrates velocity constants and gravity curves across world-allocated collectibles.
     * @param map The static environment geometry grid layout context used to resolve resting
     * points.
     * @param world Target simulation container holding active collectible instances.
     * @param deltaTime The discrete frame simulation duration step length in seconds.
     */
    void updatePickupMovement(const TileMap& map, World& world, float deltaTime);

    /**
     * @brief Scans active world state layers and processes collision overlap tests against
     * character proxies.
     * @param world Immutable context provider carrying tracking collections of interactive
     * entities.
     * @param requests Safe event broker queue to dispatch collection completion and structural
     * asset cleanup requests.
     */
    void updatePickups(const World& world, WorldRequests& requests);
}
