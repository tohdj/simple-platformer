/**
 * @file animation_system.hpp
 * @brief Advances active skeletal or sprite sheet frame updates independently of structural physics
 * steps.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#pragma once

namespace simple_platformer
{
    class World;

    // Advances actor animation playback. This is intentionally separate from the
    // gameplay-only world simulation.
    void updateWorldAnimations(World& world, float deltaTime);
}
