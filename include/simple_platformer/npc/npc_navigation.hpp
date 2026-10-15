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

    // Computes movement intentions toward a specified goal point, automatically evaluating
    // pathfinding routes. Automatically triggers path regeneration if a map tile breaks, the target
    // moves past tracking limits, or the actor gets displaced far off the path. Overwrite the
    // horizontal aim direction variables on the returned package if separate weapon aiming routines
    // are needed.
    InputIntentions intentionsToReach(
        const NpcUpdate& update,
        Actor& actor,
        PathFollower& follower,
        glm::vec2 goalFeet);
}
