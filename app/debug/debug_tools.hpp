/**
 * @file        debug_tools.hpp
 * @brief       Declares high-level diagnostic layout drawing functions for orchestrating
 *              world overlay layers and text status windows.
 *
 * @author      Mohd Fadzuli
 * @author      Adapted by: Toh Da Jun
 * @copyright   Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#pragma once

#include <optional>

namespace simple_platformer
{
    struct DebugOverlay;
    struct WindowViewport;

    // Draws all world and text layers from the game's snapshot.
    // With no viewport only the text is drawn.
    void drawDebugTools(const DebugOverlay& overlay, const std::optional<WindowViewport>& viewport);
}
