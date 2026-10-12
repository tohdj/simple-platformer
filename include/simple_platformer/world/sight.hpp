/**
 * @file line_of_sight.hpp
 * @brief Raycasting verification routines for establishing visibility and lines of sight.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#pragma once

#include <glm/vec2.hpp>

namespace simple_platformer
{
    class TileMap;

    /**
     * @brief Performs a grid-based raycast to check for an unobstructed line of sight between two
     * points.
     *
     * This function evaluates whether a direct line between coordinates intercepts any
     * visibility-blocking tiles.
     *
     * @note The raycast deliberately ignores any visibility-blocking tiles wrapping the starting
     * point (`from`). This allows entities positioned inside a cover element (e.g., hidden inside
     * high grass) to look outward cleanly without blocking their own line of sight.
     *
     * @param map The current level layout and tile behavior definition matrix.
     * @param from The starting coordinates of the observation query in world pixels.
     * @param to The target coordinates of the observation query in world pixels.
     * @return true if the path between both positions is completely clear, false if obstructed.
     */
    bool lineOfSight(const TileMap& map, glm::vec2 from, glm::vec2 to);
}
