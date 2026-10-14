#include "debug_draw.hpp"

#include <imgui.h>

#include <glm/vec2.hpp>

#include "graphics/display_viewport.hpp"
#include "simple_platformer/math/aabb.hpp"
#include "ui/hud_draw.hpp"

namespace simple_platformer
{
    // Converts absolute 2D world spatial parameters into relative hardware pixel screen slots on
    // your layout view
    ImVec2 screenPosition(
        glm::vec2 worldPosition,
        const Aabb& cameraBounds,
        const WindowViewport& viewport)
    {
        // Subtracts the camera view's upper-left anchor tracking offset, then scales and adjusts
        // the point to match the OS display margins
        return {
            viewport.topLeft.x + (worldPosition.x - cameraBounds.topLeft.x) * viewport.scale.x,
            viewport.topLeft.y + (worldPosition.y - cameraBounds.topLeft.y) * viewport.scale.y};
    }

    // Projects the outline of an Axis-Aligned Bounding Box (AABB) directly onto the viewport window
    // layout maps
    void drawWorldBounds(
        ImDrawList& drawList,
        const Aabb& bounds,
        const Aabb& cameraBounds,
        const WindowViewport& viewport,
        ImU32 colour)
    {
        // Project the top-left structural coordinates out of physics metrics spaces into standard
        // UI canvas pixels
        const ImVec2 minimum = screenPosition(bounds.topLeft, cameraBounds, viewport);
        // Extrapolate the bottom-right frame coordinates by applying the box dimensions scaled
        // alongside viewport metrics
        const ImVec2 maximum = {
            minimum.x + bounds.size.x * viewport.scale.x,
            minimum.y + bounds.size.y * viewport.scale.y};

        // Render a flat wireframe border element onto the overlay window using a structural
        // thickness profile of 2.0 pixels
        drawList.AddRect(minimum, maximum, colour, 0.0F, 0, 2.0F);
    }

    // Draws a text label on display overlays, shifting the layout tracking line down sequentially
    // to prevent item overlap
    void drawTextLine(
        ImDrawList& drawList,
        ImVec2& position, // Passed by mutable reference to auto-increment drawing coordinates for
                          // consecutive calls
        const char* text,
        ImU32 colour,
        float indentation)
    {
        // Add a dropped-shadow textual value into the pipeline to verify fonts remain readable
        // across bright/complex background textures
        drawShadowedText(drawList, {position.x + indentation, position.y}, colour, text);

        // Push the reference positioning coordinates down by exactly one line height space to
        // isolate consecutive text drops
        position.y += ImGui::GetTextLineHeight();
    }
}
