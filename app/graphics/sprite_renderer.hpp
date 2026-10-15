/**
 * @file        sprite_renderer.hpp
 * @brief       Declares the SpriteRenderer class and Texture structure for managing,
 *              loading, and rendering GPU-accelerated sprites and texture resources.
 *
 * @author      Mohd Fadzuli
 * @author      Adapted by: Toh Da Jun
 * @copyright   Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#pragma once

#include <string>
#include <vector>

namespace simple_platformer
{
    struct RenderScene;

    // A view of a loaded texture. SpriteRenderer owns the GPU resource; copying this
    // value does not transfer ownership or extend its lifetime.
    struct Texture
    {
        unsigned int handle = 0; // OpenGL hardware texture object identifier.
        int width = 0;           // Texture width in pixels.
        int height = 0;          // Texture height in pixels.
    };

    // Manages low-level graphics pipeline initialization, asset texture memory,
    // shader configuration states, and composite batch-rendering passes.
    class SpriteRenderer
    {
    public:
        // Sets up default pipeline states, compiles shaders, and generates vertex arrays.
        SpriteRenderer();

        // Releases all allocated texture registry objects, shader programs, and buffers.
        ~SpriteRenderer();

        // Prevent copying to enforce explicit hardware resource lifecycle ownership.
        SpriteRenderer(const SpriteRenderer&) = delete;
        SpriteRenderer& operator=(const SpriteRenderer&) = delete;

        // Decodes an image file into memory and pushes it to a new GPU texture registry spot.
        // Returns a unique texture tracking ID indexing the vector cache slot.
        int loadTexture(const std::string& path);

        // Retrieves size layout metadata and the underlying driver handle for an ID slot.
        Texture texture(int textureId) const;

        // Processes render queue entities, staging shader uniforms before drawing blocks.
        void render(const RenderScene& scene, int framebufferWidth, int framebufferHeight);

    private:
        unsigned int shader = 0;         // Linked GPU rendering shader program handle.
        unsigned int vertexArray = 0;    // Vertex Array Object (VAO) pipeline state link tracker.
        unsigned int positionBuffer = 0; // VBO stream containing active quad vertex vertices data.
        unsigned int textureCoordinateBuffer =
            0; // VBO stream containing texture coordinate matching UV indices.
        unsigned int framebuffer = 0; // Target offscreen Framebuffer Object (FBO) system link.
        unsigned int framebufferTexture =
            0; // Dynamic texture backing the offscreen FBO color channel output.
        int viewportLocation =
            -1; // Shader location tracking viewport window aspect size parameters.
        int opacityLocation = -1; // Shader location managing entity visibility fading parameters.
        int whiteFlashLocation = -1; // Shader location masking hit damage status flash indicators.
        int shadeLocation = -1;      // Shader location modifying tint coloring multiplier updates.
        std::vector<Texture>
            textures; // Active cache array collection listing loaded hardware assets.
    };
}
