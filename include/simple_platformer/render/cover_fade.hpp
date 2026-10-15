/**
 * @file cover_fade.hpp
 * @brief Handles ambient visual fading mechanics, stealth reveals, and opacity scaling for objects
 * in cover.
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

    // Interpolation speed in seconds for a visual sprite to transition fully between exposed and
    // concealed states.
    constexpr float CoverFadeSeconds = 0.2F;

    // Total continuous duration in seconds the player remains fully visible to enemies after
    // discharging a weapon.
    constexpr float ShotRevealSeconds = 1.0F;

    // Blends the rendering visibility factors for actors and drops based on environmental
    // concealment rules. This function runs purely presentation-side calculations without mutating
    // active AI sensing routines. Non-player characters and items remain fully exposed up to a 50%
    // cover intersection, dropping to complete invisibility beyond 75%. Player entities ignore this
    // occlusion when tracing external elements, but their personal presentation shadow maps are
    // forced into a fully exposed state if actively spotted or within the shot-reveal window.
    void updateCoverFades(const TileMap& map, World& world, float deltaTime);
}
