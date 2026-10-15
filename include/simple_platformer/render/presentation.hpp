/**
 * @file world_presentation.hpp
 * @brief Handles rendering adjustments, particles, and purely cosmetic frame updates.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#pragma once

namespace simple_platformer
{
    class TileMap;
    class World;

    // Evaluates and updates presentation-only elements without affecting the core simulation state.
    // This function executes strictly after the physics and gameplay simulation loops have
    // finalized. It handles non-gameplay tasks such as:
    // 1. Advancing rendering components (e.g., visual particles, cosmetic ambient animations).
    // 2. Updating audio tracking hooks, screen shakes, or camera smoothing values.
    // 3. Syncing visual node attachments to match their underlying physics states.
    void updateWorldPresentation(const TileMap& map, World& world, float deltaTime);
}
