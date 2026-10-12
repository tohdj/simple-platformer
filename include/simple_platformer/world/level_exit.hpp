/**
 * @file level_exit.hpp
 * @brief Logic, structure validation, and unlock mechanics for level transitions and stage exit
 * volumes.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#pragma once

#include <optional>

#include "simple_platformer/inventory/item.hpp"
#include "simple_platformer/math/aabb.hpp"
#include "simple_platformer/render/sprite.hpp"

namespace simple_platformer
{
    /** @brief Intermission delay duration in seconds for the exit sequence animation before
     * transitioning scenes. */
    constexpr float ExitOpenSeconds = 1.0F;

    /**
     * @brief Spatial trigger zone defining the end-of-stage gateway and its key verification
     * criteria.
     */
    struct LevelExit
    {
        Aabb bounds; /**< Spatial collision bounding boundaries defining the trigger zone. */
        std::optional<ItemStack> requirement; /**< Optional structural key item type and count
                                                 required to activate the gateway. */
        bool consumeItem = false; /**< If true, removes the qualifying requirement stack from user
                                     inventory upon activation. */
        std::optional<int> nextLevel; /**< Target layout identifier block to load next; std::nullopt
                                         denotes absolute game completion. */
        std::optional<Sprite>
            sprite; /**< Visual model asset overlay defining the door or portal gate geometry. */
        std::optional<double>
            lastLockedTouchTimeSeconds; /**< Absolute simulation clock timestamp tracking the latest
                                           interaction attempt without a valid key. */
        std::optional<double>
            openedTimeSeconds; /**< Absolute simulation clock timestamp capturing the exact frame
                                  the exit sequence successfully unlocked. */
    };

    class World;
    struct Actor;

    /**
     * @brief Asserts state boundaries and metric requirements for isolated level exit models.
     * @param exit The target terminal structure node to verify.
     */
    void validateLevelExit(const LevelExit& exit);

    /** @brief Verifies whether the provided actor satisfies all inventory key validation parameters
     * required by the exit. */
    bool exitUnlocked(const LevelExit& exit, const Actor& actor);

    /** @brief Confirms whether the simulation is currently running an active exit cutscene block.
     */
    bool exitOpening(const World& world);

    /** @brief Processes intersection sweeps between the user node and exit volume bounds, running
     * unlocking routines. */
    void updateLevelExit(World& world);
}
