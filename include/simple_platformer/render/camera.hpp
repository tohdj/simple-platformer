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

    // Represents the view transformation state for the rendering system.
    struct Camera
    {
        // Top-left position of the viewport in world-space coordinates (pixels).
        glm::vec2 position = {0.0F, 0.0F};

        // The width and height dimensions of the visible viewport screen area.
        glm::vec2 viewportSize = InternalViewportSize;
    };

    // Controls camera behavior, tracking targets smoothly using a dead-zone bounding window.
    struct CameraController
    {
        // Constructs a new Camera Controller object with viewport and dead-zone sizes.
        CameraController(Camera initialCamera, glm::vec2 initialDeadZoneSize);

        // The managed camera instance holding viewport spatial states.
        Camera camera;

        // Width and height thresholds within the screen center defining the passive target window.
        glm::vec2 deadZoneSize;
    };

    // Creates a static camera rigidly locked onto a target, clamped within map boundaries.
    Camera makeLockedCamera(
        const TileMap& map,
        const Aabb& target,
        glm::vec2 viewportSize = InternalViewportSize);

    // Factory function initializing a smooth tracking CameraController.
    CameraController makeCameraController(
        const TileMap& map,
        const Aabb& target,
        glm::vec2 deadZoneSize,
        glm::vec2 viewportSize = InternalViewportSize);

    // Dynamically reframes and shifts the camera tracking state if a target exits the dead zone.
    // Keeps camera positions completely bound inside the active map layout limits.
    void followTarget(CameraController& controller, const TileMap& map, const Aabb& target);

    // Converts a 2D position vector from absolute world coordinates to view-space screen
    // coordinates.
    glm::vec2 worldToScreen(const Camera& camera, glm::vec2 worldPosition);

    // Converts a 2D position vector from screen-space viewport positions back to world-space
    // simulation metrics.
    glm::vec2 screenToWorld(const Camera& camera, glm::vec2 screenPosition);
}
