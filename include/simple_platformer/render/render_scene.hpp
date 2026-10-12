/**
 * @file render_scene.hpp
 * @brief Transforms simulation metrics into unified screen-space sprite drawing command matrices.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#pragma once

#include <vector>

#include <glm/vec2.hpp>

#include "simple_platformer/render/camera.hpp"
#include "simple_platformer/render/sprite.hpp"

namespace simple_platformer
{
    class TileMap; /**< Forward declaration for the map grid database. */
    class World;   /**< Forward declaration for the active simulation state provider. */

    /**
     * @brief A self-contained draw command passed directly to the graphics rendering pipeline.
     */
    struct SpriteDrawCommand
    {
        int textureId = 0;                  /**< Resource index pointing to the cached GPU texture asset array wrapper. */
        glm::vec2 position = {0.0F, 0.0F};  /**< Top-left placement point on screen, measured in internal viewport pixels. */
        glm::vec2 size = {0.0F, 0.0F};      /**< Screen dimensions of the drawn asset in target viewport pixels. */
        SpriteRegion source;                /**< Section layout area defining what pixels from the atlas sheet are displayed. */
        bool flipHorizontal = false;        /**< Flags whether to mirror the visual texture mapping horizontally. */
        float rotationRadians = 0.0F;       /**< Clockwise rotation angle in radians calculated relative to the sprite center point. */
        float opacity = 1.0F;               /**< Texture transparency weight multiplier (1.0 for fully opaque, 0.0 for invisible). */
        float whiteFlashAmount = 0.0F;      /**< Interpolation index shifting towards absolute white, typically for hurt or hit frames (0.0 to 1.0). */
        float shadeAmount = 0.0F;           /**< Interpolation index shifting towards absolute black, typically for shadow mapping or stealth layers (0.0 to 1.0). */
    };

    /**
     * @brief Ambient shading multiplier threshold evaluated whenever a player entity is fully
     * obscured from vision.
     * @note The player's sprite darkens by this opacity scalar instead of disappearing completely
     * from view.
     */
    constexpr float PlayerConcealedShade = 0.6F;

    /**
     * @brief Contains an isolated, flat array stack of visual primitives ready for rasterization.
     */
    struct RenderScene
    {
        std::vector<SpriteDrawCommand>
            sprites; /**< Ordered draw command stack evaluating background, environment tiles,
                        items, and actor structures. */
    };

    /**
     * @brief Evaluates spatial world elements and extracts a rasterization command array.
     *
     * @param map The static environment geometry grid used to parse structural cells into drawing
     * layers.
     * @param tileTextureId Resource index identifier mapping to the environment tile sheet catalog.
     * @param camera Viewport perspective state computing viewport spatial mapping offsets.
     * @param world The target world simulation data provider carrying active visual entities.
     * @return RenderScene Populated render frame container listing individual command nodes.
     */
    RenderScene buildRenderScene(
        const TileMap& map,
        int tileTextureId,
        const Camera& camera,
        const World& world);
}
