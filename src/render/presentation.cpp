/**
 * @file world_presentation.cpp
 * @brief Handles rendering adjustments, particles, and purely cosmetic frame updates.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#include "simple_platformer/render/presentation.hpp"

#include "simple_platformer/render/animation_system.hpp"
#include "simple_platformer/render/cover_fade.hpp"

namespace simple_platformer
{
    void updateWorldPresentation(const TileMap& map, World& world, float deltaTime)
    {
        updateWorldAnimations(world, deltaTime);
        updateCoverFades(map, world, deltaTime);
    }
}
