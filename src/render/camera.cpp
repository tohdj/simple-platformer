/**
 * @file camera.cpp
 * @brief Viewport coordinates, smooth tracking controllers, and coordinate space conversion
 * helpers.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#include "simple_platformer/render/camera.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include <glm/vec2.hpp>

#include "simple_platformer/math/aabb.hpp"
#include "simple_platformer/math/validation.hpp"
#include "simple_platformer/world/tile_map.hpp"

namespace simple_platformer
{
    namespace
    {
        // Asserves that viewport dimensions have non-zero, positive, and stable float boundaries
        void validateViewport(glm::vec2 viewportSize)
        {
            if (!isFinitePositive(viewportSize))
            {
                throw std::invalid_argument("Camera viewport must be positive and finite");
            }
        }

        // Verifies that the input tracking margin scales safely within the current viewport borders
        void validateDeadZone(glm::vec2 deadZoneSize, glm::vec2 viewportSize)
        {
            if (!isFinitePositive(deadZoneSize) || deadZoneSize.x > viewportSize.x ||
                deadZoneSize.y > viewportSize.y)
            {
                throw std::invalid_argument(
                    "Camera dead zone must be positive, finite, and no larger than the viewport");
            }
        }

        // Validates both position tracking states and screen sizes for the structural camera node
        void validateCamera(const Camera& camera)
        {
            validateViewport(camera.viewportSize);
            requireFinite(camera.position, "Camera position");
        }

        // Where the camera's edge settles on one axis: a map smaller than the viewport sits
        // centred in it, otherwise the camera stops at the map's edges. Whole pixels keep the
        // tiles crisp.
        float settleAxis(float cameraPosition, float mapSize, float viewportSize)
        {
            // Center the view on this axis if the level footprint size is narrower than the monitor
            // viewport block
            if (mapSize <= viewportSize)
            {
                return std::round((mapSize - viewportSize) * 0.5F);
            }

            // Otherwise, clamp camera offsets securely between 0 and the final pixel row of the
            // level
            return std::round(std::clamp(cameraPosition, 0.0F, mapSize - viewportSize));
        }

        // Computes tracking translation alignments when a target moves past the edge thresholds of
        // the camera dead-zone window
        float followAxis(
            float cameraPosition,
            float targetCenter,
            float mapSize,
            float viewportSize,
            float deadZoneSize)
        {
            // Derive absolute world-space start and end coordinates of the screen-centered passive
            // dead-zone window
            const float deadZoneStart = cameraPosition + (viewportSize - deadZoneSize) * 0.5F;
            const float deadZoneEnd = deadZoneStart + deadZoneSize;

            // Adjust camera position coordinates only if the character pushes past the window
            // margins
            if (targetCenter < deadZoneStart)
            {
                cameraPosition -= deadZoneStart - targetCenter;
            }
            else if (targetCenter > deadZoneEnd)
            {
                cameraPosition += targetCenter - deadZoneEnd;
            }

            // Bound and round coordinate outputs to keep pixel layouts crisp
            return settleAxis(cameraPosition, mapSize, viewportSize);
        }
    }

    // Constructor: Creates a camera tracking profile while running bounds integrity tests
    CameraController::CameraController(Camera initialCamera, glm::vec2 initialDeadZoneSize)
        : camera(initialCamera),
          deadZoneSize(initialDeadZoneSize)
    {
        validateCamera(camera);
        validateDeadZone(deadZoneSize, camera.viewportSize);
    }

    // Creates a camera configuration centered strictly over a target, clamped to level map extremes
    Camera makeLockedCamera(const TileMap& map, const Aabb& target, glm::vec2 viewportSize)
    {
        validateViewport(viewportSize);

        // Compute top-left alignment offsets to frame the target perfectly in the viewport midpoint
        const glm::vec2 centredOnTarget = centerOf(target) - viewportSize * 0.5F;
        Camera camera;
        camera.viewportSize = viewportSize;
        camera.position = {
            settleAxis(centredOnTarget.x, map.pixelWidth(), viewportSize.x),
            settleAxis(centredOnTarget.y, map.pixelHeight(), viewportSize.y)};
        return camera;
    }

    // Factory Method: Generates a tracking controller framework using initial coordinates and
    // target sizes
    CameraController makeCameraController(
        const TileMap& map,
        const Aabb& target,
        glm::vec2 deadZoneSize,
        glm::vec2 viewportSize)
    {
        return CameraController{makeLockedCamera(map, target, viewportSize), deadZoneSize};
    }

    // Evaluates moving focus coordinates, shifting the camera window if elements push outside
    // dead-zone margins
    void followTarget(CameraController& controller, const TileMap& map, const Aabb& target)
    {
        validateCamera(controller.camera);
        validateDeadZone(controller.deadZoneSize, controller.camera.viewportSize);

        // Fetch midpoint vectors to calculate distance differentials relative to internal dead-zone
        // borders
        const glm::vec2 targetCenter = centerOf(target);
        controller.camera.position = {
            followAxis(
                controller.camera.position.x,
                targetCenter.x,
                map.pixelWidth(),
                controller.camera.viewportSize.x,
                controller.deadZoneSize.x),
            followAxis(
                controller.camera.position.y,
                targetCenter.y,
                map.pixelHeight(),
                controller.camera.viewportSize.y,
                controller.deadZoneSize.y)};
    }

    // Projects a 2D coordinates vector out of absolute simulation world space into local display
    // screen pixels
    glm::vec2 worldToScreen(const Camera& camera, glm::vec2 worldPosition)
    {
        return worldPosition - camera.position;
    }

    // Translates input view-frame display point parameters (e.g., cursor, clicks) back into world
    // map coordinates
    glm::vec2 screenToWorld(const Camera& camera, glm::vec2 screenPosition)
    {
        return screenPosition + camera.position;
    }
}
