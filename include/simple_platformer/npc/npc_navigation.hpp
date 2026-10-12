/**
 * @file npc_navigation.hpp
 * @brief Utility for calculating input steering macros and route changes to guide NPCs toward
 * target goals.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#pragma once

#include <glm/vec2.hpp>

#include "simple_platformer/input/input_state.hpp"

namespace simple_platformer
{
    struct Actor;
    struct NpcUpdate;
    struct PathFollower;

    /**
     * @brief Computes movement intentions toward a specified goal point, automatically evaluating
     * route calculations.
     *
     * This steering routine automatically triggers path regeneration sequences if a map tile
     * structure breaks, the target goal moves past tracking limits, or the actor's physical body
     * gets displaced far off the path.
     *
     * @note Systems that additionally handle separate weapon aiming routines should overwrite the
     * horizontal targeting variables on the returned structure after this function returns.
     *
     * @param update Aggregated environment state fields, containing lookahead caches and delta
     * frame clocks.
     * @param actor The underlying physical actor entity data structure to evaluate.
     * @param follower Path navigation controller tracking waypoints and replaying structural
     * inputs.
     * @param goalFeet Spatial world target coordinates tracking the destination baseline footprint.
     * @return InputIntentions A structured steering package containing directional bits and inputs
     * for this tick.
     */
    InputIntentions intentionsToReach(
        const NpcUpdate& update,
        Actor& actor,
        PathFollower& follower,
        glm::vec2 goalFeet);
}
