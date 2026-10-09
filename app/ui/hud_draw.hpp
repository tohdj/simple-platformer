#pragma once

#include <imgui.h>

// Simple platformer rendering helpers built on top of Dear ImGui's draw list.
namespace simple_platformer
{
    // Forward declaration of a sprite region within a texture atlas.
    // The full definition is provided elsewhere.
    struct SpriteRegion;

    // Forward declaration of a texture resource.
    // The full definition is provided elsewhere.
    struct Texture;

    // Default colour for HUD text: opaque white, in ImGui's packed 32-bit RGBA format.
    constexpr ImU32 HudTextColour = IM_COL32(255, 255, 255, 255);

    // Draws text over a one-pixel dark shadow, so it reads on any part of the scene.
    // drawList: the ImGui draw list to render into.
    // position: the screen-space position of the text.
    // colour:   the colour of the text itself (packed RGBA).
    // text:     the null-terminated string to draw.
    void drawShadowedText(ImDrawList& drawList, ImVec2 position, ImU32 colour, const char* text);

    // Draws plain text at the given screen coordinates with the specified colour.
    // drawList: the ImGui draw list to render into.
    // x, y:     the screen-space position of the text.
    // text:     the null-terminated string to draw.
    // colour:   the colour of the text (packed RGBA).
    void drawText(ImDrawList& drawList, float x, float y, const char* text, ImU32 colour);

    // Draws text horizontally centred on the given X coordinate.
    // drawList: the ImGui draw list to render into.
    // centerX:  the screen-space X coordinate that the text is centred on.
    // y:        the screen-space Y coordinate of the text.
    // text:     the null-terminated string to draw.
    void drawCenteredText(ImDrawList& drawList, float centerX, float y, const char* text);

    // Draws a region of a texture atlas, stretched to fill the given screen rectangle.
    // drawList:      the ImGui draw list to render into.
    // atlas:         the texture atlas containing the sprite.
    // region:        the sub-rectangle of the atlas to draw.
    // topLeft:       the screen-space top-left corner of the destination rectangle.
    // bottomRight:   the screen-space bottom-right corner of the destination rectangle.
    void drawAtlasRegion(
        ImDrawList& drawList,
        const Texture& atlas,
        const SpriteRegion& region,
        ImVec2 topLeft,
        ImVec2 bottomRight);
}
