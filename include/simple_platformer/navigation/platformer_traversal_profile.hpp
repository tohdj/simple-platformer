/**
 * @file platformer_traversal_profile.hpp
 * @brief Configuration profiles detailing static movement capabilities and physical constraints for
 * actor archetypes.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#pragma once

#include <optional>

#include <glm/vec2.hpp>

#include "simple_platformer/movement/platformer_movement.hpp"
#include "simple_platformer/movement/surface_climb.hpp"

namespace simple_platformer
{
    /**
     * @brief A configuration profile defining the static traversal capabilities of an actor
     * category.
     *
     * This structure represents immutable layout capabilities and simulation step targets rather
     * than runtime state. It serves as a parameter provider for node connectivity or motion
     * predictions.
     */
    struct PlatformerTraversalProfile
    {
        /**
         * @brief Constructs an empty, default profile configuration.
         */
        PlatformerTraversalProfile() = default;

        /**
         * @brief Constructs a fully customized platformer traversal profile.
         * @param bodySize Width and height measurements of the actor's physics bounding box.
         * @param movementConfig Parameters controlling running acceleration, friction, and jump
         * heights.
         * @param seconds The target physics timestep duration allocated per evaluation cycle.
         * @param climbConfig Optional parameters defining ladder or wall scaling thresholds and
         * speeds.
         */
        PlatformerTraversalProfile(
            glm::vec2 bodySize,
            PlatformerMovementConfig movementConfig,
            float seconds,
            std::optional<SurfaceClimbConfig> climbConfig = std::nullopt)
            : size(bodySize),
              movement(movementConfig),
              stepSeconds(seconds),
              climb(climbConfig)
        {
        }

        glm::vec2 size = {
            0.0F,
            0.0F}; /**< Bounding physics footprint layout dimensions measured in world pixels. */
        PlatformerMovementConfig movement; /**< Core structural physics rules containing
                                              coefficients for friction, speeds, and forces. */
        float stepSeconds = 0.0F; /**< Fixed snapshot step duration metric matching the target
                                     simulation framerate slice. */
        std::optional<SurfaceClimbConfig>
            climb; /**< Optional configuration details enabling and detailing ladder or vertical
                      wall-scaling parameters. */
    };

    /**
     * @brief Evaluates two traversal profiles for logical equality.
     *
     * Compares core bounds, physics parameters, timelines, and evaluates matching optional climbing
     * properties.
     *
     * @param left The first configuration profile block.
     * @param right The second configuration profile block.
     * @return true if both capability layouts mirror each other, false otherwise.
     */
    inline bool operator==(
        const PlatformerTraversalProfile& left,
        const PlatformerTraversalProfile& right)
    {
        if (left.size != right.size || left.movement != right.movement ||
            left.stepSeconds != right.stepSeconds ||
            left.climb.has_value() != right.climb.has_value())
        {
            return false;
        }
        return !left.climb.has_value() || left.climb->speed == right.climb->speed;
    }
}
