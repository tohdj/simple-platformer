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

    /**
     * @brief Evaluates and updates presentation-only elements without affecting the simulation.
     *
     * This function executes strictly after the physics and gameplay simulation loops have
     * finalized. It handles non-gameplay tasks such as:
     * 1. Advancing rendering components (e.g., visual particles, cosmetic ambient animations).
     * 2. Updating audio tracking hooks, screen shakes, or camera smoothing values.
     * 3. Syncing visual node attachments to match their underlying physics states.
     *
     * @param map The static tile map layout used for contextual visual elements (like edge
     * particles or tile dust).
     * @param world The game world containing the visual elements and presentation parameters.
     * @param deltaTime The elapsed time since the last frame in seconds.
     */
    void updateWorldPresentation(const TileMap& map, World& world, float deltaTime);
}
