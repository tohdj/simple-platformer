/**
 * @file        debug_tools.cpp
 * @brief       Declares high-level diagnostic layout drawing functions for orchestrating
 *              world overlay layers and text status windows.
 *
 * @author      Mohd Fadzuli
 * @author      Adapted by: Toh Da Jun
 * @copyright   Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#include "debug_tools.hpp"

// User interface and rendering headers for the debug system
#include "debug_overlay.hpp"
#include "debug_overlay_ui.hpp"

#include <optional>

namespace simple_platformer
{
    /**
     * @brief Renders the debug tools and overlays onto the screen.
     *
     * @param overlay The data and state of the current debug overlay.
     * @param viewport The optional window viewport configuration to map the rendering coordinates.
     */
    void drawDebugTools(const DebugOverlay& overlay, const std::optional<WindowViewport>& viewport)
    {
        // Delegate the actual rendering process to the UI drawing subsystem
        drawDebugOverlay(overlay, viewport);
    }
}
