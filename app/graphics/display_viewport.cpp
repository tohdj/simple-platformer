#include "graphics/display_viewport.hpp"

#include <algorithm>
#include <optional>

#include <glm/vec2.hpp>

#include "simple_platformer/math/coordinates.hpp"
#include "simple_platformer/math/validation.hpp"

namespace simple_platformer
{
    /**
     * @brief Computes a pixel-perfect, integer-scaled viewport centered within the framebuffer.
     *        Maintains the core game aspect ratio (InternalWidth x InternalHeight).
     *
     * @param framebufferSize Dimensions of the current rendering surface in pixels.
     * @return std::optional<DisplayViewport> The calculated viewport properties, or std::nullopt if
     * the buffer is too small.
     */
    std::optional<DisplayViewport> makeDisplayViewport(glm::ivec2 framebufferSize)
    {
        // Guard against invalid or collapsed framebuffers
        if (framebufferSize.x <= 0 || framebufferSize.y <= 0)
        {
            return std::nullopt;
        }

        // Calculate the maximum integer scale factor that completely fits within the destination
        // dimensions
        const int scale =
            std::min(framebufferSize.x / InternalWidth, framebufferSize.y / InternalHeight);
        if (scale <= 0)
        {
            return std::nullopt; // Framebuffer is smaller than the base resolution
        }

        // Determine the dimension of the actual scaled gameplay area
        const glm::ivec2 size = {InternalWidth * scale, InternalHeight * scale};

        // Calculate empty margins for centering (handling remainder pixels evenly)
        const glm::ivec2 lowerMargin = (framebufferSize - size) / 2;
        const int topMargin = framebufferSize.y - size.y - lowerMargin.y;

        return DisplayViewport{{lowerMargin.x, topMargin}, lowerMargin.y, size, scale};
    }

    /**
     * @brief Creates a viewport mapped directly to window coordinates (useful for UI libraries like
     * Dear ImGui). Handles high-DPI disparities where window coordinates differ from raw pixel
     * framebuffers.
     *
     * @param windowSize Logical dimensions of the OS window.
     * @param framebufferSize Physical pixel dimensions of the rendering surface.
     * @return std::optional<WindowViewport> Scaled viewport limits in logical units.
     */
    std::optional<WindowViewport> makeWindowViewport(
        glm::ivec2 windowSize,
        glm::ivec2 framebufferSize)
    {
        if (windowSize.x <= 0 || windowSize.y <= 0)
        {
            return std::nullopt;
        }

        const std::optional<DisplayViewport> viewport = makeDisplayViewport(framebufferSize);
        if (!viewport.has_value())
        {
            return std::nullopt;
        }

        // Determine the scaling ratio between raw physical pixels and logical window coordinates
        const glm::vec2 framebufferToWindow = {
            static_cast<float>(windowSize.x) / static_cast<float>(framebufferSize.x),
            static_cast<float>(windowSize.y) / static_cast<float>(framebufferSize.y)};

        // Map the physical margin offset and integer scale factor into logical window scale factors
        return WindowViewport{
            glm::vec2(viewport->topLeftMargin) * framebufferToWindow,
            glm::vec2(static_cast<float>(viewport->scale)) * framebufferToWindow};
    }

    /**
     * @brief Translates an arbitrary screen position (e.g., mouse cursor) into the game's internal
     * coordinate grid.
     *
     * @param windowPosition Mouse or cursor position in logical window spaces.
     * @param windowSize Current dimensions of the OS window.
     * @param framebufferSize Physical pixel size of the backing renderer buffer.
     * @return std::optional<glm::vec2> Coordinates inside the internal viewport, or std::nullopt if
     * the point falls out of bounds (black bars).
     */
    std::optional<glm::vec2> windowToInternal(
        glm::vec2 windowPosition,
        glm::ivec2 windowSize,
        glm::ivec2 framebufferSize)
    {
        // Input validation to reject NaN, Infinities, or collapsed window vectors
        if (!isFinite(windowPosition) || windowSize.x <= 0 || windowSize.y <= 0)
        {
            return std::nullopt;
        }

        const std::optional<DisplayViewport> viewport = makeDisplayViewport(framebufferSize);
        if (!viewport.has_value())
        {
            return std::nullopt;
        }

        // Project the logical pointer coordinates back into physical framebuffer pixel space
        const glm::vec2 framebufferPosition = {
            windowPosition.x * static_cast<float>(framebufferSize.x) /
                static_cast<float>(windowSize.x),
            windowPosition.y * static_cast<float>(framebufferSize.y) /
                static_cast<float>(windowSize.y)};

        // Calculate offset vector relative to the top-left edge of the active render zone
        const glm::vec2 relative = framebufferPosition - glm::vec2(viewport->topLeftMargin);

        // Bounds checking: check if the position lands on the external framing
        // letterboxes/pillarboxes
        if (relative.x < 0.0F || relative.y < 0.0F ||
            relative.x >= static_cast<float>(viewport->size.x) ||
            relative.y >= static_cast<float>(viewport->size.y))
        {
            return std::nullopt;
        }

        // Divide the relative offset by the scaling factor to return exact native pixel coordinates
        return relative / static_cast<float>(viewport->scale);
    }
}
