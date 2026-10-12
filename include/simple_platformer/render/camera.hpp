/**
 * @file camera.hpp
 * @brief Viewport coordinates, smooth tracking controllers, and coordinate space conversion
 * helpers.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#pragma once

#include <glm/vec2.hpp>

#include "simple_platformer/math/aabb.hpp"
#include "simple_platformer/math/coordinates.hpp"

namespace simple_platformer
{
    class TileMap;

    /**
     * @brief Represents the view transformation state for the rendering system.
     */
    struct Camera
    {
        glm::vec2 position = {
            0.0F,
            0.0F}; /**< Top-left position of the viewport in world-space coordinates (pixels). */
        glm::vec2 viewportSize = InternalViewportSize; /**< The width and height dimensions of the
                                                          visible viewport screen area. */
    };

    /**
     * @brief Controls camera behavior, tracking targets smoothly using a dead-zone bounding window.
     */
    struct CameraController
    {
        /**
         * @brief Constructs a new Camera Controller object.
         * @param initialCamera The starting configurations and dimensions for the camera.
         * @param initialDeadZoneSize The bounding box size where target movement won't shift the
         * camera view.
         */
        CameraController(Camera initialCamera, glm::vec2 initialDeadZoneSize);

        Camera camera;          /**< The managed camera instance holding viewport spatial states. */
        glm::vec2 deadZoneSize; /**< Width and height thresholds within the screen center defining
                                   the passive target window. */
    };

    /**
     * @brief Creates a static camera rigidly locked onto a target, clamped within map boundaries.
     * @param map The static environment map context used to clamp camera boundaries.
     * @param target The bounding volume of the entity to center the camera on.
     * @param viewportSize Extents of the screen view area.
     * @return A newly initialized Camera centered on the target.
     */
    Camera makeLockedCamera(
        const TileMap& map,
        const Aabb& target,
        glm::vec2 viewportSize = InternalViewportSize);

    /**
     * @brief Factory function initializing a smooth tracking CameraController.
     * @param map The static environment map context used to clamp camera boundaries.
     * @param target The tracking source object to initially frame within the window.
     * @param deadZoneSize The dimensional window margins for target free-movement.
     * @param viewportSize Extents of the screen view area.
     * @return An configured CameraController tracking framework.
     */
    CameraController makeCameraController(
        const TileMap& map,
        const Aabb& target,
        glm::vec2 deadZoneSize,
        glm::vec2 viewportSize = InternalViewportSize);

    /**
     * @brief Dynamically reframes and shifts the camera tracking state if a target exits the dead
     * zone.
     *
     * Keeps camera positions completely bound inside the active map layout limits.
     *
     * @param controller The camera tracking runtime state structure to adjust.
     * @param map The static environment layout defining boundaries.
     * @param target The moving bounding volume to track inside the space.
     */
    void followTarget(CameraController& controller, const TileMap& map, const Aabb& target);

    /**
     * @brief Converts a 2D position vector from absolute world coordinates to view-space screen
     * coordinates.
     * @param camera The active camera view matrix provider.
     * @param worldPosition Target coordinates inside the global simulation landscape.
     * @return Transformed position vector relative to the screen display frame.
     */
    glm::vec2 worldToScreen(const Camera& camera, glm::vec2 worldPosition);

    /**
     * @brief Converts a 2D position vector from screen-space viewport positions back to world-space
     * simulation metrics.
     * @param camera The active camera view matrix provider.
     * @param screenPosition Cursor, mouse click, or interface coordinates.
     * @return Transformed absolute position vector inside the simulated world.
     */
    glm::vec2 screenToWorld(const Camera& camera, glm::vec2 screenPosition);
}
