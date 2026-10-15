/**
 * @file        navigation_debug_ui.hpp
 * @brief       Declares diagnostic overlay rendering functions for visualizing pathfinding
 *              cell footprints, connection nodes, and profiling metrics via Dear ImGui.
 *
 * @author      Mohd Fadzuli
 * @author      Adapted by: Toh Da Jun
 * @copyright   Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#pragma once

#include <imgui.h>

namespace simple_platformer
{
    struct Aabb;
    struct NavigationConnectionsDebugInfo;
    struct WindowViewport;

    // Draws visible cells and the cursor cell's footprint, outgoing connections,
    // and connection count.
    void drawNavigationConnections(
        ImDrawList& drawList,
        const NavigationConnectionsDebugInfo& navigation,
        const Aabb& cameraBounds,
        const WindowViewport& viewport);

    // Draws the selected profile, then advances the text position.
    void drawNavigationProfile(
        ImDrawList& drawList,
        const NavigationConnectionsDebugInfo& navigation,
        ImVec2& position);
}
