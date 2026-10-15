/**
 * @file        navigation_debug_ui.cpp
 * @brief       Declares diagnostic overlay rendering functions for visualizing pathfinding 
 *              cell footprints, connection nodes, and profiling metrics via Dear ImGui.
 *
 * @author      Mohd Fadzuli
 * @author      Adapted by: Toh Da Jun
 * @copyright   Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#include "navigation_debug_ui.hpp"

#include <cstddef>
#include <cstdio>
#include <optional>
#include <vector>

#include <imgui.h>

#include <glm/vec2.hpp>

#include "debug_draw.hpp"
#include "graphics/display_viewport.hpp"
#include "navigation_debug.hpp"
#include "simple_platformer/math/aabb.hpp"
#include "simple_platformer/navigation/traversal.hpp"
#include "ui/hud_draw.hpp"

namespace simple_platformer
{
    namespace
    {
        // Color palettes (packed as 32-bit RGBA) representing different grid states for pathfinding
        // cells
        constexpr ImU32 NavigationConnectedColour =
            IM_COL32(64, 160, 255, 90); // Normal standable node links (Semi-transparent blue)
        // A cell with connections that a climber can only hold a wall or the ceiling in.
        constexpr ImU32 NavigationClimbConnectedColour =
            IM_COL32(176, 96, 255, 90); // Restricted wall/ceiling anchor node links (Purple)
        constexpr ImU32 NavigationEmptyColour =
            IM_COL32(128, 128, 128, 70); // Valid pathfinding cell with zero outgoing connections
        constexpr ImU32 NavigationMissingColour =
            IM_COL32(255, 96, 32, 220); // Unprocessed or corrupted graph cell warning (Orange)
        constexpr ImU32 NavigationFootprintColour =
            IM_COL32(255, 224, 64, 200); // Sizing footprint box under active inspection (Yellow)

        // Looks up the specific debug overlay color mapped to each unique AI navigation travel type
        ImU32 traversalColour(Traversal traversal)
        {
            switch (traversal)
            {
            case Traversal::Walk:
                return WalkingPathColour;
            case Traversal::Fall:
                return FallingPathColour;
            case Traversal::Jump:
                return JumpingPathColour;
            case Traversal::Climb:
                return WalkingPathColour;
            case Traversal::Fly:
                return FlyingPathColour;
            }
            return UnknownPathColour;
        }

        // Renders the background block representing an individual node cell inside the AI
        // navigation grid
        void drawNavigationCell(
            ImDrawList& drawList,
            const NavigationCellDebugInfo& cell,
            const Aabb& cameraBounds,
            const WindowViewport& viewport)
        {
            // Convert absolute world positions into scaled pixel screens coordinates
            const ImVec2 minimum = screenPosition(cell.bounds.topLeft, cameraBounds, viewport);
            const ImVec2 maximum = {
                minimum.x + cell.bounds.size.x * viewport.scale.x,
                minimum.y + cell.bounds.size.y * viewport.scale.y};

            // If the cell was never processed by the graph generator, draw an orange alert box and
            // exit early
            if (!cell.connections.has_value())
            {
                drawList.AddRect(minimum, maximum, NavigationMissingColour, 0.0F, 0, 2.0F);
                return;
            }

            // Choose the fill color based on whether an NPC can stand natively vs hanging on
            // walls/ceilings
            const ImU32 connectedColour =
                cell.standable ? NavigationConnectedColour : NavigationClimbConnectedColour;
            const ImU32 colour = *cell.connections == 0 ? NavigationEmptyColour : connectedColour;

            // Paint the filled rectangle backdrop onto the background layer
            drawList.AddRectFilled(minimum, maximum, colour);
        }

        // Overlays a small text counter inside a node cell showing its exact number of valid
        // outgoing edges
        void drawConnectionCount(
            ImDrawList& drawList,
            const NavigationCellDebugInfo& cell,
            const Aabb& cameraBounds,
            const WindowViewport& viewport)
        {
            if (!cell.connections.has_value())
            {
                return;
            }
            char count[8];
            std::snprintf(count, sizeof(count), "%zu", *cell.connections);
            const ImVec2 minimum = screenPosition(cell.bounds.topLeft, cameraBounds, viewport);

            // Render the text string near the top-left edge of the targeted node tile block
            drawShadowedText(drawList, {minimum.x + 1.0F, minimum.y}, WorldLabelColour, count);
        }

        // Draws detailed path trace splines and bounding outlines for the node explicitly under
        // crosshairs
        void drawCursorCell(
            ImDrawList& drawList,
            const CursorCellDebugInfo& cell,
            const Aabb& cameraBounds,
            const WindowViewport& viewport)
        {
            // Render the yellow sizing box representing the target NPC collision bounds footprint
            // if present
            if (cell.footprint.has_value())
            {
                drawWorldBounds(
                    drawList,
                    cell.footprint.value_or(Aabb{}),
                    cameraBounds,
                    viewport,
                    NavigationFootprintColour);
            }

            // Iterate through and trace all valid outgoing edge connections linked to this cell
            // context
            for (const ConnectionDebugInfo& connection : cell.connections)
            {
                const ImU32 colour = traversalColour(connection.traversal);

                // Fallback: draw a basic straight line if no complex airborne path coordinates are
                // sampled
                if (connection.sampledFeet.size() < 2)
                {
                    drawList.AddLine(
                        screenPosition(connection.fromFeet, cameraBounds, viewport),
                        screenPosition(connection.toFeet, cameraBounds, viewport),
                        colour,
                        1.0F);
                    continue;
                }

                // Tracing loop: connect continuous line segments together to display cinematic
                // jumping parabolas
                for (std::size_t index = 1; index < connection.sampledFeet.size(); ++index)
                {
                    drawList.AddLine(
                        screenPosition(connection.sampledFeet[index - 1], cameraBounds, viewport),
                        screenPosition(connection.sampledFeet[index], cameraBounds, viewport),
                        colour,
                        1.0F);
                }
            }
            // Highlight the cell container boundaries with a crisp wireframe frame outline
            drawWorldBounds(drawList, cell.bounds, cameraBounds, viewport, WorldLabelColour);
        }
    }

    // Orchestrates rendering all visible navigation node maps and detailed contextual cursor
    // highlights
    void drawNavigationConnections(
        ImDrawList& drawList,
        const NavigationConnectionsDebugInfo& navigation,
        const Aabb& cameraBounds,
        const WindowViewport& viewport)
    {
        // Fallback to an empty metadata block if the crosshair doesn't hover over any active grid
        // path nodes
        const CursorCellDebugInfo cursor = navigation.cursorCell.value_or(CursorCellDebugInfo{});
        const NavigationCellDebugInfo* cellUnderCursor = nullptr;

        // 1. Draw out every cell tile node that intersects our active viewport frustum matrix
        for (const NavigationCellDebugInfo& cell : navigation.cells)
        {
            drawNavigationCell(drawList, cell, cameraBounds, viewport);

            // Check if the current tile matches the exact coordinate spot right under our mouse
            // cursor
            if (navigation.cursorCell.has_value() && cell.bounds.topLeft == cursor.bounds.topLeft)
            {
                cellUnderCursor = &cell; // Cache target pointer for numerical text overlay passes
            }
        }

        // 2. Render details for the focused cell node (parabolic jump traces, footprint shapes)
        if (navigation.cursorCell.has_value())
        {
            drawCursorCell(drawList, cursor, cameraBounds, viewport);
        }

        // 3. Render connection numerical text strings safely over top of the active focused node
        if (cellUnderCursor != nullptr)
        {
            drawConnectionCount(drawList, *cellUnderCursor, cameraBounds, viewport);
        }
    }

    // Formats and prints text metrics blocks inside developer logs tracking the active AI graph
    // index profiles
    void drawNavigationProfile(
        ImDrawList& drawList,
        const NavigationConnectionsDebugInfo& navigation,
        ImVec2& position)
    {
        constexpr float Indentation = 12.0F; // Text margin indentation alignment constant

        // Add heading label row to distinguish this telemetry category block
        drawTextLine(drawList, position, "navigation profile", TextHeadingColour);
        char text[48];

        // Display character identity bindings names if known; fallback to showing standard sizing
        // measurements
        if (navigation.actorName.empty())
        {
            std::snprintf(
                text,
                sizeof(text),
                "body:   %gx%g",
                static_cast<double>(navigation.bodySize.x),
                static_cast<double>(navigation.bodySize.y));
        }
        else
        {
            std::snprintf(text, sizeof(text), "body:   %s", navigation.actorName.c_str());
        }
        drawTextLine(drawList, position, text, TextDetailColour, Indentation);

        // Log the active profile rotation counter index (Press N to cycle types during runtime
        // debugging)
        std::snprintf(
            text,
            sizeof(text),
            "shown:  %zu/%zu (N)",
            navigation.profileIndex + 1,
            navigation.profileCount);
        drawTextLine(drawList, position, text, TextDetailColour, Indentation);
    }
}
