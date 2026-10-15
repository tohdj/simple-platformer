/**
 * @file sight.cpp
 * @brief Raycasting verification routines for establishing visibility and lines of sight.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#include "simple_platformer/world/sight.hpp"

#include <glm/vec2.hpp>

#include "simple_platformer/physics/segment_cast.hpp"

namespace simple_platformer
{
    bool lineOfSight(const TileMap& map, glm::vec2 from, glm::vec2 to)
    {
        return !segmentCastSightBlockingTiles(map, from, to).has_value();
    }
}
