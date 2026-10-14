#include "graphics/sprite_renderer.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <filesystem>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include <glad/glad.h>
#include <glm/vec2.hpp>
#include <stb_image.h>

#include "graphics/display_viewport.hpp"
#include "simple_platformer/math/coordinates.hpp"
#include "simple_platformer/render/render_scene.hpp"

namespace simple_platformer
{
    namespace
    {
        // Helper function that rotates an offset point around a center origin using standard 2D
        // rotation matrix math
        glm::vec2 rotatedCorner(glm::vec2 centre, glm::vec2 offset, float cosine, float sine)
        {
            return {
                centre.x + offset.x * cosine -
                    offset.y * sine, // X' = X_center + (x * cos(θ) - y * sin(θ))
                centre.y + offset.x * sine +
                    offset.y * cosine}; // Y' = Y_center + (x * sin(θ) + y * cos(θ))
        }

        // Calculates the 4 rotated corner coordinates of a sprite and packs them into a 6-vertex
        // triangle array layout (2 triangles forming a quad)
        std::array<float, 12> spritePositions(const SpriteDrawCommand& command)
        {
            const glm::vec2 halfSize = command.size * 0.5F; // Compute half bounds dimension vectors
            const glm::vec2 centre =
                command.position + halfSize; // Establish the exact central rotational origin point
            const float cosine =
                std::cos(command.rotationRadians); // Pre-calculate cosine lookup variables
            const float sine =
                std::sin(command.rotationRadians); // Pre-calculate sine lookup variables

            // Determine all four corner positions in world pixel coordinates by applying the
            // rotation matrix
            const glm::vec2 topLeft = rotatedCorner(centre, -halfSize, cosine, sine);
            const glm::vec2 bottomLeft =
                rotatedCorner(centre, {-halfSize.x, halfSize.y}, cosine, sine);
            const glm::vec2 bottomRight = rotatedCorner(centre, halfSize, cosine, sine);
            const glm::vec2 topRight =
                rotatedCorner(centre, {halfSize.x, -halfSize.y}, cosine, sine);

            // Pack the transformed float coordinates into a flat mesh layout sequence
            // (Triangle 1: TL -> BL -> BR | Triangle 2: TL -> BR -> TR)
            return {
                topLeft.x,
                topLeft.y,
                bottomLeft.x,
                bottomLeft.y,
                bottomRight.x,
                bottomRight.y,
                topLeft.x,
                topLeft.y,
                bottomRight.x,
                bottomRight.y,
                topRight.x,
                topRight.y};
        }

        // Compiles a single raw GLSL vertex or fragment shader stage source string into hardware
        // instructions
        unsigned int compileShader(unsigned int type, const char* source)
        {
            const unsigned int shader = glCreateShader(
                type); // Request a new empty shader stage context handle from the GPU driver
            glShaderSource(
                shader,
                1,
                &source,
                nullptr); // Assign the text strings array data into the shader object
            glCompileShader(
                shader); // Direct the graphics compiler to parse and compile the source string code

            int succeeded = 0;
            glGetShaderiv(
                shader,
                GL_COMPILE_STATUS,
                &succeeded); // Query the hardware compilation validation flags
            if (succeeded == GL_TRUE)
            {
                return shader; // Successful verification pass; export active resource pointer index
                               // handles
            }

            // Extract compilation log diagnostic info text descriptions from the GPU driver if
            // execution checks fail
            std::array<char, 1024> message{};
            glGetShaderInfoLog(shader, static_cast<int>(message.size()), nullptr, message.data());
            glDeleteShader(
                shader); // Deallocate failed internal context handles to prevent resource leaks
            throw std::runtime_error(
                "OpenGL shader compilation failed: " + std::string(message.data()));
        }

        // Compiles both shader stages, hooks them into a unified pipeline executable block program,
        // and links variables
        unsigned int createShaderProgram()
        {
            // Vertex Shader String: Scales screen pixel spaces into normalized -1.0 to +1.0
            // hardware OpenGL Clip Spaces
            constexpr const char* VertexSource = R"(
            #version 330 core
            layout (location = 0) in vec2 vertexPosition; // Input structural location coordinate vertex streams
            layout (location = 1) in vec2 vertexUv;       // Input texture UV coordinate channel lookup arrays
            uniform vec2 viewportSize;                    // Dimensions size vector mapping internal screen sizes
            out vec2 textureUv;                           // Output texture UV interface variables matching fragment pipeline targets

            void main()
            {
                // Normalize absolute pixel coordinates into a 0.0 -> 1.0 boundary range array range
                vec2 normalised = vertexPosition / viewportSize;
                
                // Remap normalized configurations into native -1.0 to +1.0 hardware coordinates, while flipping Y (0,0 is Top-Left)
                gl_Position = vec4(normalised.x * 2.0 - 1.0, 1.0 - normalised.y * 2.0, 0.0, 1.0);
                textureUv = vertexUv; // Pass through coordinates downstream to interpolation stages
            }
        )";

            // Fragment Shader String: Samples colors out of sheets and applies visual fx layers
            // (opacity, shade, damage flashes)
            constexpr const char* FragmentSource = R"(
            #version 330 core
            in vec2 textureUv;                     // Interpolated position coordinate anchors incoming from the vertex stages
            uniform sampler2D spriteTexture;       // Texture unit index pointer handle mapping core sheets
            uniform float spriteOpacity;           // Alpha scaling coefficient parameters (0.0 full invisible -> 1.0 solid)
            uniform float spriteWhiteFlashAmount;  // Factor setting color blends towards solid white (Damage feedback alerts)
            uniform float spriteShadeAmount;       // Factor setting color overrides down towards pitch black (Shadowing updates)
            out vec4 colour;                       // Output target register color configurations for drawing outputs pixel layers

            void main()
            {
                // Sample baseline colors right out of the graphic sheets texture coordinates mappings
                colour = texture(spriteTexture, textureUv);
                
                // Linearly interpolate color spectrum paths towards solid white based on damage variables
                colour.rgb = mix(colour.rgb, vec3(1.0), spriteWhiteFlashAmount);
                // Linearly interpolate color spectrum paths towards complete black based on shadowing variables
                colour.rgb = mix(colour.rgb, vec3(0.0), spriteShadeAmount);
                
                // Scale structural transparency settings against alpha modifier targets
                colour.a *= spriteOpacity;
            }
        )";

            // Compile the separate isolated program stages independently
            const unsigned int vertex = compileShader(GL_VERTEX_SHADER, VertexSource);
            const unsigned int fragment = compileShader(GL_FRAGMENT_SHADER, FragmentSource);

            // Build the core executable binding pipeline shell context container handle
            const unsigned int program = glCreateProgram();
            glAttachShader(
                program, vertex); // Link the vertex shader stage module onto the execution block
            glAttachShader(
                program,
                fragment); // Link the fragment shader stage module onto the execution block
            glLinkProgram(program); // Finalize variable linkages and optimize structural operations
                                    // internally

            // Flag individual object stages for deletion immediately; they stay alive implicitly
            // until the program drops
            glDeleteShader(vertex);
            glDeleteShader(fragment);

            int succeeded = 0;
            glGetProgramiv(program, GL_LINK_STATUS, &succeeded); // Verify linking state checks
            if (succeeded == GL_TRUE)
            {
                return program; // Success; export working graphics device program handles
            }

            // Extract pipeline linking diagnostic message info streams out of driver memory blocks
            // upon setup crash scenarios
            std::array<char, 1024> message{};
            glGetProgramInfoLog(program, static_cast<int>(message.size()), nullptr, message.data());
            glDeleteProgram(program); // Clear leaking pipeline program contexts
            throw std::runtime_error(
                "OpenGL shader linking failed: " + std::string(message.data()));
        }
    }

    // Constructor: Sets up Vertex Array Objects (VAO), dynamic buffers, shader uniforms, and the
    // low-res asset offscreen framebuffer
    SpriteRenderer::SpriteRenderer()
        : shader(createShaderProgram())
    {
        // Generate and bind a Vertex Array Object to store the layout state configurations for our
        // sprite stream
        glGenVertexArrays(1, &vertexArray);
        glBindVertexArray(vertexArray);

        // 12 floats represent 2 coordinates (X, Y) per vertex over 6 vertices (2 triangles making
        // up 1 quad)
        constexpr std::size_t ValuesPerBuffer = 12;

        // Setup Vertex Buffer Object (VBO) for structural screen positions
        glGenBuffers(1, &positionBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);
        glBufferData(
            GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(ValuesPerBuffer * sizeof(float)),
            nullptr, // Left null because vertex definitions are explicitly populated during render
                     // dispatch passes
            GL_DYNAMIC_DRAW); // Flagged dynamic since coordinates morph constantly as objects
                              // traverse maps
        glEnableVertexAttribArray(0); // Attribute layout index 0 corresponds to positional vectors
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);

        // Setup Vertex Buffer Object (VBO) for mapping source texture UV coordinates
        glGenBuffers(1, &textureCoordinateBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, textureCoordinateBuffer);
        glBufferData(
            GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(ValuesPerBuffer * sizeof(float)),
            nullptr,
            GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(
            1); // Attribute layout index 1 corresponds to texture texture anchors
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);

        // Extract and cache hardware uniform location handles out of the compiled binary shader
        // pipeline program
        viewportLocation = glGetUniformLocation(shader, "viewportSize");
        opacityLocation = glGetUniformLocation(shader, "spriteOpacity");
        whiteFlashLocation = glGetUniformLocation(shader, "spriteWhiteFlashAmount");
        shadeLocation = glGetUniformLocation(shader, "spriteShadeAmount");

        // Activate our target shader pipeline program and assign texture layer unit index 0 for
        // draw lookups
        glUseProgram(shader);
        glUniform1i(glGetUniformLocation(shader, "spriteTexture"), 0);

        // Setup a custom Framebuffer Object (FBO) to draw our retro scene at a clean, pixel-snapped
        // target resolution
        glGenFramebuffers(1, &framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

        // Generate a matching texture asset wrapper to bind onto the color channel slot of our
        // custom canvas surface
        glGenTextures(1, &framebufferTexture);
        glBindTexture(GL_TEXTURE_2D, framebufferTexture);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGBA8,
            InternalWidth,
            InternalHeight, // Locks standard vintage low-res dimensions internally
            0,
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            nullptr); // Data left uninitialized; the pipeline populates this surface during draw
                      // iterations

        // Enforce NEAREST filtering rules to preserve sharp, blocky pixel boundaries during canvas
        // scaling steps
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        // Link the texture container array onto color attachment index 0 of the active offscreen
        // canvas buffer
        glFramebufferTexture2D(
            GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebufferTexture, 0);

        // Validate configuration states to verify hardware driver configuration compatibility
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            throw std::runtime_error("OpenGL could not create the internal framebuffer");
        }

        // Return back to default display buffer contexts to safely conclude initialization phases
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    // Destructor: Safely deallocates GPU graphic textures, canvas framebuffers, vertex structures,
    // and compiled shader scripts
    SpriteRenderer::~SpriteRenderer()
    {
        // Iterate through all custom loaded texture sheets and release their internal hardware
        // memory allocations
        for (const Texture& texture : textures)
        {
            glDeleteTextures(1, &texture.handle);
        }
        // Delete the explicit texture target attached to the offscreen pixel rendering buffer
        // canvas
        glDeleteTextures(1, &framebufferTexture);
        // Free up the offscreen pixel-snapped Framebuffer Object allocation
        glDeleteFramebuffers(1, &framebuffer);
        // Delete the UV texture coordinate Vertex Buffer Object resource handle
        glDeleteBuffers(1, &textureCoordinateBuffer);
        // Delete the spatial layout position Vertex Buffer Object resource handle
        glDeleteBuffers(1, &positionBuffer);
        // Clean up the main binding container configuration layout handle
        glDeleteVertexArrays(1, &vertexArray);
        // Completely destroy the compiled and linked vertex/fragment GPU pipeline program
        glDeleteProgram(shader);
    }

    // Loads a physical graphic file via stb_image and converts the bytes directly into an OpenGL 2D
    // texture sheet unit
    int SpriteRenderer::loadTexture(const std::string& path)
    {
        int width = 0;
        int height = 0;
        int channels = 0;
        // Enforce 4-channel image conversion parameters to reliably stream pixel maps down as
        // structured RGBA blocks
        unsigned char* pixels = stbi_load(path.c_str(), &width, &height, &channels, 4);
        if (pixels == nullptr)
        {
            throw std::runtime_error(
                "Could not load texture '" + std::filesystem::absolute(path).string() +
                "': " + stbi_failure_reason());
        }

        Texture texture;
        texture.width = width;
        texture.height = height;
        // Request a raw hardware texture block placement index handle from the GPU driver
        glGenTextures(1, &texture.handle);
        // Bind the current handle target definition layer to accept incoming image definitions
        glBindTexture(GL_TEXTURE_2D, texture.handle);
        // Allocate texture graphics memory structures and transfer the loaded image data array
        // blocks downward
        glTexImage2D(
            GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        // Free the file data out of system RAM because the graphic bits now reside entirely in GPU
        // memory
        stbi_image_free(pixels);

        // Enforce NEAREST textures filters to ensure retro pixels look perfectly blocky instead of
        // blurry when scaled
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        // Clamp textures boundary behaviors tightly onto image margins to prevent alpha border
        // bleeding issues
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // Append the tracking configuration parameters into the vector array and pass back its
        // index identifier footprint
        textures.push_back(texture);
        return static_cast<int>(textures.size() - 1);
    }

    // Performs a safe, bounds-checked validation check to fetch a texture resource metadata struct
    // by index
    Texture SpriteRenderer::texture(int textureId) const
    {
        // Enforce boundary safety check to prevent reading out-of-bounds index tracking limits
        if (textureId < 0 || static_cast<std::size_t>(textureId) >= textures.size())
        {
            throw std::out_of_range("Unknown texture ID");
        }

        // Return a copy of the validated texture entry structure
        return textures[static_cast<std::size_t>(textureId)];
    }

    // Handles the double-buffered rendering execution loop: draws everything to an offscreen
    // canvas, then scales it to fit the window
    void SpriteRenderer::render(
        const RenderScene& scene,
        int framebufferWidth,
        int framebufferHeight)
    {
        // --- STAGE 1: BIND FRAMEBUFFER AND DRAW RAW SPRITES TO THE PIXEL-LOCKED INTERNAL CANVAS
        // --- Route all upcoming hardware draw calls into our offscreen internal layout canvas
        // surface
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        // Lock render clipping bounds directly onto our low-res retro specifications
        glViewport(0, 0, InternalWidth, InternalHeight);
        // Set up corporate dark blue-gray tint color properties as the baseline background slate
        glClearColor(0.08F, 0.12F, 0.18F, 1.0F);
        // Clear color memory buffers to sweep the previous frame's layout clean
        glClear(GL_COLOR_BUFFER_BIT);

        // Engage alpha blending hardware states so translucent pixels compile properly on top of
        // tiles
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // Direct the graphics pipeline to use our custom compiled shader program
        glUseProgram(shader);
        // Pass base texture dimensions down to tracking uniform registers inside our active vertex
        // program
        glUniform2f(
            viewportLocation,
            static_cast<float>(InternalWidth),
            static_cast<float>(InternalHeight));
        // Activate our structural graphic attributes layout mapping context (VAO)
        glBindVertexArray(vertexArray);

        // Iterate sequentially through every sprite item request pack dispatched for this frame
        for (const SpriteDrawCommand& command : scene.sprites)
        {
            // Verify that the target element refers to a valid loaded texture asset handle
            // signature index
            if (command.textureId < 0 ||
                static_cast<std::size_t>(command.textureId) >= textures.size())
            {
                throw std::out_of_range("A sprite refers to an unknown texture");
            }

            // Extract the validated texture meta profile out of the tracker array list storage
            const Texture& texture = textures[static_cast<std::size_t>(command.textureId)];

            // Convert source asset pixel dimensions down into standard 0.0 to 1.0 hardware UV
            // textures space coordinates
            float leftUv = command.source.position.x / static_cast<float>(texture.width);
            float rightUv = (command.source.position.x + command.source.size.x) /
                            static_cast<float>(texture.width);

            // If movement animation requests require a horizontal orientation reflection, swap left
            // and right UV anchors
            if (command.flipHorizontal)
            {
                std::swap(leftUv, rightUv);
            }
            const float topUv = command.source.position.y / static_cast<float>(texture.height);
            const float bottomUv = (command.source.position.y + command.source.size.y) /
                                   static_cast<float>(texture.height);

            // Generate standard vertex layout positions matching real-time destination world
            // locations
            const std::array<float, 12> positions = spritePositions(command);

            // Build matching triangular grid meshes (Triangle 1: TL->BL->BR | Triangle 2:
            // TL->BR->TR)
            const std::array<float, 12> textureCoordinates = {
                leftUv,
                topUv, // Top Left
                leftUv,
                bottomUv, // Bottom Left
                rightUv,
                bottomUv, // Bottom Right
                leftUv,
                topUv, // Top Left
                rightUv,
                bottomUv, // Bottom Right
                rightUv,
                topUv // Top Right
            };

            // Bind our targeted sprite atlas sheet onto physical graphics texture unit index slot 0
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texture.handle);

            // Push rendering effect float variables up into the active fragment shaders pipeline
            // tracking registers
            glUniform1f(opacityLocation, command.opacity);
            glUniform1f(whiteFlashLocation, command.whiteFlashAmount);
            glUniform1f(shadeLocation, command.shadeAmount);

            // Stream spatial coordinates downstream into the hardware layout array positions buffer
            // memory
            glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);
            glBufferSubData(
                GL_ARRAY_BUFFER,
                0,
                static_cast<GLsizeiptr>(positions.size() * sizeof(float)),
                positions.data());

            // Stream UV lookup placement values down into the hardware textures mapping coordinate
            // arrays
            glBindBuffer(GL_ARRAY_BUFFER, textureCoordinateBuffer);
            glBufferSubData(
                GL_ARRAY_BUFFER,
                0,
                static_cast<GLsizeiptr>(textureCoordinates.size() * sizeof(float)),
                textureCoordinates.data());

            // Direct OpenGL to draw our 6 configured mesh points as a pair of operational drawing
            // triangles
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        // --- STAGE 2: STRETCH AND BLIT THE DRAWN CANVAS DIRECTLY ONTO THE PHYSICAL OS WINDOW ---
        // Redirect target render output instructions away from our custom FBO and back down to
        // desktop displays
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        // Expand the drawing viewport limits to envelope the explicit physical window resolution
        // profile
        glViewport(0, 0, framebufferWidth, framebufferHeight);
        // Fill outer window background edges with clean black to frame mismatching stretch ratios
        // nicely (Letterboxing)
        glClearColor(0.0F, 0.0F, 0.0F, 1.0F);
        // Wipe away the color rendering buffers once more to prepare for display outputs placement
        glClear(GL_COLOR_BUFFER_BIT);

        // Evaluate dynamic window layout attributes to extract correct scaling factors and aspect
        // ratio margins
        const std::optional<DisplayViewport> output =
            makeDisplayViewport({framebufferWidth, framebufferHeight});
        if (!output.has_value())
        {
            return; // Display window minimized or dropped tracking context visibility; skip render
                    // passes safely
        }

        // Bind our internal offscreen low-res drawing surface texture directly into the layout
        // reader slot layer
        glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);

        // Execute a raw high-speed pixel copy blit to scale our internal retro frames out across
        // physical screen coordinates. Handles scaling computations instantaneously while using
        // GL_NEAREST pixel filters to prevent muddy interpolation blur.
        glBlitFramebuffer(
            0,
            0,
            InternalWidth,
            InternalHeight, // Source rectangular bounding constraints (Our internal canvas
                            // resolution)
            output->topLeftMargin.x,
            output->bottomMargin, // Destination bounding lower coordinate bounds (Applies centering
                                  // margins padding)
            output->topLeftMargin.x + output->size.x,
            output->bottomMargin + output->size.y, // Destination bounding upper layout constraints
            GL_COLOR_BUFFER_BIT,
            GL_NEAREST); // NEAREST pixel locking choice totally avoids bilinear distortion blurs

        // Disengage active frame buffering interfaces to leave context engines back in clean
        // default states
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
}
