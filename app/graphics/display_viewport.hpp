/**
 * @file        display_viewport.hpp
 * @brief       Declares the layout structures and viewport math projection functions for
 *              handling letterboxing, scaling, and high-DPI coordinate conversions.
 *
 * @author      Mohd Fadzuli
 * @author      Adapted by: Toh Da Jun
 * @copyright   Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#pragma once

#include <optional>

#include <glm/vec2.hpp>

namespace simple_platformer
{
    // Framebuffer-space viewport layout configurations for rendering letterboxed output.
    struct DisplayViewport
    {
        glm::ivec2 topLeftMargin = {
            0,
            0};                   // Offset padding from the top-left window corner in pixels.
        int bottomMargin = 0;     // Offset padding from the bottom window edge in pixels.
        glm::ivec2 size = {0, 0}; // Active dimensions of the display screen area in pixels.
        int scale = 1;            // Integer upscale factor calculated for the resolution.
    };

    // Window-space viewport properties used to transform UI mouse coordinates.
    struct WindowViewport
    {
        glm::vec2 topLeft = {
            0.0F,
            0.0F}; // Top-left boundary of the game area in logical screen points.
        glm::vec2 scale = {
            1.0F,
            1.0F}; // Conversion ratio matching logical points back to game coordinates.
    };

    // Fits an integer-scaled internal image into framebuffer pixels, centred with
    // letterboxing. Returns nothing if the framebuffer cannot fit one internal image.
    std::optional<DisplayViewport> makeDisplayViewport(glm::ivec2 framebufferSize);

    // Calculates the game viewport boundary and scale ratios measured in logical screen points.
    std::optional<WindowViewport> makeWindowViewport(
        glm::ivec2 windowSize,
        glm::ivec2 framebufferSize);

    // Transforms mouse coordinates from logical window points into the internal game coordinate
    // space. Returns std::nullopt if the pointer is hovering outside the active screen within the
    // letterbox bounds.
    std::optional<glm::vec2> windowToInternal(
        glm::vec2 windowPosition,
        glm::ivec2 windowSize,
        glm::ivec2 framebufferSize);
}
