#include "hud_draw.hpp"

#include <imgui.h>

#include "graphics/sprite_renderer.hpp"
#include "simple_platformer/render/sprite.hpp"

// Contains the HUD text and atlas drawing helpers for the simple_platformer game.
namespace simple_platformer
{
    // Internal helpers that are only visible within this translation unit.
    namespace
    {
        // The text shadow's colour and how far down and right it falls, in window pixels.
        constexpr ImU32 ShadowColour = IM_COL32(0, 0, 0, 220);
        constexpr float ShadowOffset = 1.0F;
    }

    /**
     * Draws text with a drop shadow so it stays readable against any background.
     *
     * The shadow is drawn first, offset down and to the right by ShadowOffset pixels,
     * and the main text is drawn on top of it at the requested position.
     *
     * @param drawList  The ImGui draw list to append the text to.
     * @param position  The top-left screen position of the text, in window pixels.
     * @param colour    The colour of the main text.
     * @param text      The null-terminated string to draw.
     */
    void drawShadowedText(ImDrawList& drawList, ImVec2 position, ImU32 colour, const char* text)
    {
        // Draw the shadow first so the main text appears on top of it.
        drawList.AddText(
            {position.x + ShadowOffset, position.y + ShadowOffset}, ShadowColour, text);
        // Draw the main text at the original position.
        drawList.AddText(position, colour, text);
    }

    /**
     * Convenience wrapper around drawShadowedText that takes separate X and Y coordinates.
     *
     * @param drawList  The ImGui draw list to append the text to.
     * @param x         The horizontal screen position of the text's left edge, in window pixels.
     * @param y         The vertical screen position of the text's top edge, in window pixels.
     * @param text      The null-terminated string to draw.
     * @param colour    The colour of the main text.
     */
    void drawText(ImDrawList& drawList, float x, float y, const char* text, ImU32 colour)
    {
        drawShadowedText(drawList, {x, y}, colour, text);
    }

    /**
     * Draws a line of HUD text horizontally centred on a given X coordinate, with a shadow.
     *
     * The text is always drawn using the shared HudTextColour constant.
     *
     * @param drawList  The ImGui draw list to append the text to.
     * @param centerX   The horizontal screen position to centre the text on, in window pixels.
     * @param y         The vertical screen position of the text's top edge, in window pixels.
     * @param text      The null-terminated string to draw.
     */
    void drawCenteredText(ImDrawList& drawList, float centerX, float y, const char* text)
    {
        // Measure the text width and shift the left edge back by half of it to centre the text.
        const float left = centerX - ImGui::CalcTextSize(text).x * 0.5F;
        drawShadowedText(drawList, {left, y}, HudTextColour, text);
    }

    /**
     * Draws a sub-rectangle of a texture atlas into a screen-space rectangle.
     *
     * The region is given in texture pixels and is converted to normalised (0..1)
     * UV coordinates using the atlas dimensions before being submitted to ImGui.
     *
     * @param drawList     The ImGui draw list to append the image to.
     * @param atlas        The texture atlas containing the sprite(s), including its pixel size and
     * handle.
     * @param region       The sub-rectangle of the atlas to draw, in atlas pixels.
     * @param topLeft      The top-left screen position to draw the region at, in window pixels.
     * @param bottomRight  The bottom-right screen position to draw the region at, in window pixels.
     */
    void drawAtlasRegion(
        ImDrawList& drawList,
        const Texture& atlas,
        const SpriteRegion& region,
        ImVec2 topLeft,
        ImVec2 bottomRight)
    {
        // Atlas dimensions in pixels, used to normalise the region coordinates.
        const float width = static_cast<float>(atlas.width);
        const float height = static_cast<float>(atlas.height);

        // Submit the image: the first UV pair is the region's top-left corner and the
        // second is its bottom-right corner, both expressed as fractions of the atlas size.
        drawList.AddImage(
            static_cast<ImTextureID>(atlas.handle),
            topLeft,
            bottomRight,
            {region.position.x / width, region.position.y / height},
            {(region.position.x + region.size.x) / width,
             (region.position.y + region.size.y) / height});
    }
}
