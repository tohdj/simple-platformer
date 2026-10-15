/**
 * @file        debug_overlay_ui.hpp
 * @brief       Declares high-level rendering functions for drawing real-time game state
 *              overlays and world-space diagnostic layers.
 *
 * @author      Mohd Fadzuli
 * @author      Adapted by: Toh Da Jun
 * @copyright   Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#pragma once

#include <optional>

#include "graphics/display_viewport.hpp"

namespace simple_platformer
{
    struct DebugOverlay;

    void drawDebugOverlay(const DebugOverlay& scene, const std::optional<WindowViewport>& viewport);
}
