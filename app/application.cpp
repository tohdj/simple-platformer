#include "application.hpp"

#include <cstddef>
#include <cstdlib>
#include <optional>
#include <utility>

#include <glm/vec2.hpp>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <imgui.h>

#include "content/game_catalogs.hpp"
#include "content/level_catalog.hpp"
#include "debug/debug_tools.hpp"
#include "game/game.hpp"
#include "graphics/display_viewport.hpp"
#include "graphics/game_window.hpp"
#include "graphics/imgui_session.hpp"
#include "graphics/sprite_renderer.hpp"
#include "ui/interface_ui.hpp"
#include "simple_platformer/input/input_state.hpp"
#include "simple_platformer/render/render_scene.hpp"
#include "simple_platformer/timing/fixed_step.hpp"
#include "simple_platformer/timing/stopwatch.hpp"

namespace simple_platformer
{
    namespace
    {
        // Holds the runtime state and configuration flags for the running application
        struct ApplicationContext
        {
            InputState input; // Tracks current keyboard/mouse actions for the player
            glm::vec2 aimDirection = {
                1.0F,
                0.0F};                     // Normalized directional vector for player aiming
            bool showDebugOverlay = false; // Toggles visibility of developer tool overlays

            // Which NPC body's navigation the overlay shows; N moves to the next.
            std::size_t debugBodyIndex = 0;

            // B with the overlay open breaks the tile under the cursor, as a shot would.
            bool breakTileRequested = false;
            bool inventoryOpen = false; // Tracks whether the inventory UI menu is currently open

            // Set when the inventory opens or closes, or the game restarts.
            // The next frame clears gameplay input and discards
            // accumulated time before simulation can continue.
            bool playInterrupted = false;
            bool restartRequested = false; // Flag to signal that a game reset is pending
        };

        // Maps a GLFW framework physical key code to the matching abstract game input button
        std::optional<InputButton> buttonForKey(int key)
        {
            switch (key)
            {
            case GLFW_KEY_A:
            case GLFW_KEY_LEFT:
                return InputButton::Left;
            case GLFW_KEY_D:
            case GLFW_KEY_RIGHT:
                return InputButton::Right;
            case GLFW_KEY_W:
            case GLFW_KEY_UP:
            case GLFW_KEY_SPACE:
                return InputButton::Jump;
            case GLFW_KEY_DOWN:
                return InputButton::Down;
            default:
                return std::nullopt; // Key does not correspond to a gameplay movement command
            }
        }

        // Callback function triggered by GLFW on keyboard key press, release, or repeat events
        void handleKey(GLFWwindow* window, int key, int, int action, int)
        {
            // Retrieve custom context pointer attached to the operating window
            auto* context = static_cast<ApplicationContext*>(glfwGetWindowUserPointer(window));

            // Q key toggles inventory state and pauses/interrupts active simulation time
            if (key == GLFW_KEY_Q && action == GLFW_PRESS)
            {
                context->inventoryOpen = !context->inventoryOpen;
                context->playInterrupted = true;
                return;
            }
            // R key triggers a game restart request
            if (key == GLFW_KEY_R && action == GLFW_PRESS)
            {
                context->restartRequested = true;
                return;
            }
            // ESC key sets window flag to close down the application
            if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
            {
                glfwSetWindowShouldClose(window, GLFW_TRUE);
                return;
            }
            // F1 key toggles developer debug overlay tools
            if (key == GLFW_KEY_F1 && action == GLFW_PRESS)
            {
                context->showDebugOverlay = !context->showDebugOverlay;
                return;
            }
            // N key cycles through target indexes for entity debugging
            if (key == GLFW_KEY_N && action == GLFW_PRESS)
            {
                ++context->debugBodyIndex;
                return;
            }
            // B key requests a tile destruction under cursor (only active if debug mode is visible)
            if (key == GLFW_KEY_B && action == GLFW_PRESS && context->showDebugOverlay)
            {
                context->breakTileRequested = true;
                return;
            }

            // Filter out repeat key actions; we only track discrete press and release edges
            if (action != GLFW_PRESS && action != GLFW_RELEASE)
            {
                return;
            }

            // Translate the physical button state to game input actions
            const std::optional<InputButton> button = buttonForKey(key);
            if (!button.has_value())
            {
                return;
            }

            context->input.setButton(*button, action == GLFW_PRESS);
        }

        // Callback function triggered by GLFW on mouse button actions
        void handleMouseButton(GLFWwindow* window, int button, int action, int)
        {
            // Filter actions to respond exclusively to standard left clicks
            if (button != GLFW_MOUSE_BUTTON_LEFT ||
                (action != GLFW_PRESS && action != GLFW_RELEASE))
            {
                return;
            }

            // Set or clear the player's primary attack state flag
            auto* context = static_cast<ApplicationContext*>(glfwGetWindowUserPointer(window));
            context->input.setButton(InputButton::PrimaryAttack, action == GLFW_PRESS);
        }

        // Restarts the game logic loop if requested by context flags and game state is complete
        void restartIfRequested(ApplicationContext& context, Game& game)
        {
            if (!context.restartRequested)
            {
                return;
            }
            if (game.complete())
            {
                game.restart();
                context.inventoryOpen = false;
                context.aimDirection = {1.0F, 0.0F};
                context.playInterrupted = true;
            }
            context.restartRequested = false;
        }

        // Processes structural requests generated by the UI interface layout layer
        void applyInterfaceRequests(
            ApplicationContext& context,
            Game& game,
            const InterfaceRequests& interfaceRequests)
        {
            if (interfaceRequests.toggleInventory)
            {
                context.inventoryOpen = !context.inventoryOpen;
                context.playInterrupted = true;
            }
            if (interfaceRequests.useInventorySlot.has_value())
            {
                game.useInventoryItem(*interfaceRequests.useInventorySlot);
            }
        }

        // Evaluates debug tile destruction commands based on current calculated mouse position
        void breakTileIfRequested(
            ApplicationContext& context,
            Game& game,
            const std::optional<glm::vec2>& internalCursor)
        {
            if (!context.breakTileRequested)
            {
                return;
            }
            if (internalCursor.has_value())
            {
                game.breakTileAt(*internalCursor);
            }
            context.breakTileRequested = false;
        }

        // UI mouse capture changes gameplay aim and firing, not simulation time.
        std::optional<glm::vec2> gameplayCursor(
            const ApplicationContext& context,
            const std::optional<glm::vec2>& internalCursor,
            bool mouseCaptured)
        {
            // Disable crosshair tracking if cursor is interacting with menus or inventory is active
            if (mouseCaptured || context.inventoryOpen)
            {
                return std::nullopt;
            }
            return internalCursor;
        }

        // Clear buttons and pending edges when gameplay cannot use them. Keyboard
        // capture blocks input, but does not pause simulation or reset accumulated time.
        void preparePlayerInput(
            ApplicationContext& context,
            bool simulationBlocked,
            const std::optional<glm::vec2>& gameCursor,
            bool keyboardCaptured)
        {
            if (simulationBlocked || context.playInterrupted || keyboardCaptured)
            {
                context.input = {}; // Reset the input tracking model entirely
            }
            else if (!gameCursor.has_value())
            {
                // Force cease fire if the pointer tracking drops off screen
                context.input.clearButton(InputButton::PrimaryAttack);
            }
        }

        // Consumes held buttons and pending edges for one simulation step. A gameplay
        // cursor updates aim; without one, keep the last aim and suppress firing.
        InputIntentions playerIntentions(
            ApplicationContext& context,
            const Game& game,
            const std::optional<glm::vec2>& gameCursor)
        {
            InputIntentions intentions = context.input.consumeIntentions();
            if (gameCursor.has_value())
            {
                const glm::vec2 aimDirection = game.playerAimDirection(*gameCursor);
                if (aimDirection != glm::vec2{0.0F, 0.0F})
                {
                    context.aimDirection =
                        aimDirection; // Cache current non-zero vector orientation
                }
            }
            else
            {
                intentions.primaryAttackPressed = false; // Halt firing if cursor position is lost
            }
            intentions.aimDirection = context.aimDirection;
            return intentions;
        }

        // Updates simulation logic loops using a consistent fixed time stepping frequency
        void runGameUpdates(
            ApplicationContext& context,
            Game& game,
            FixedStep& fixedStep,
            float frameSeconds,
            bool simulationBlocked,
            const std::optional<glm::vec2>& gameCursor)
        {
            if (simulationBlocked || context.playInterrupted)
            {
                // Time that built up would otherwise be simulated in a burst on resuming.
                fixedStep.reset();
                context.playInterrupted = false;
                return;
            }
            // Process game system delta ticks safely over multiple micro steps
            fixedStep.advance(
                frameSeconds,
                [&](float deltaTime)
                {
                    const InputIntentions intentions = playerIntentions(context, game, gameCursor);
                    game.update(intentions, deltaTime);
                });
        }
    }

    // High level orchestration engine managing runtime creation, update polling, and destruction
    int runApplication()
    {
        // 1. Core Window and Graphics Framework Initialization
        const GameWindow window("Simple Platformer", {960, 540});
        ApplicationContext context;
        glfwSetWindowUserPointer(window.handle(), &context);
        glfwSetKeyCallback(window.handle(), handleKey);
        glfwSetMouseButtonCallback(window.handle(), handleMouseButton);
        const ImGuiSession imgui(window.handle());
        // 2. Resource loading and dependency layout settings
        SpriteRenderer renderer;
        const int atlas = renderer.loadTexture("assets/textures/sprites.png");
        const Texture atlasTexture = renderer.texture(atlas);
        FixedStep fixedStep;
        LevelCatalog levelCatalog = loadLevelCatalog("assets/levels/levels.json");
        GameCatalogs gameCatalogs =
            loadGameCatalogs("assets/catalogs", {atlasTexture.width, atlasTexture.height});
        Game game(
            atlas,
            std::move(levelCatalog),
            std::move(gameCatalogs),
            static_cast<float>(fixedStep.stepSeconds()));
        Stopwatch frameClock;
        // 3. Central Application Runtime Loop
        while (!window.shouldClose())
        {
            // Process OS windows callback inputs and start ImGui drawing phase
            glfwPollEvents();
            imgui.beginFrame();
            restartIfRequested(context, game);
            // Fetch physical frame structural sizes and calculate aspect viewports
            const WindowReading reading = window.read();
            const std::optional windowViewport =
                makeWindowViewport(reading.size, reading.framebufferSize);
            // Calculate precise timing step intervals
            const float frameSeconds = frameClock.lapSeconds();
            const InterfaceRequests interfaceRequests =
                drawInterface(game, atlasTexture, windowViewport, context.inventoryOpen);
            applyInterfaceRequests(context, game, interfaceRequests);
            // Normalize coordinate spaces from system values to layout simulation pixels
            const std::optional<glm::vec2 > internalCursor =
                windowToInternal(reading.cursor, reading.size, reading.framebufferSize);
            breakTileIfRequested(context, game, internalCursor);
            const std::optional<glm::vec2> gameCursor =
                gameplayCursor(context, internalCursor, ImGui::GetIO().WantCaptureMouse);
            // Determine blocks, update player controls, and advance simulation ticks
            const bool simulationBlocked = context.inventoryOpen || game.complete();
            preparePlayerInput(
                context, simulationBlocked, gameCursor, ImGui::GetIO().WantCaptureKeyboard);
            runGameUpdates(context, game, fixedStep, frameSeconds, simulationBlocked, gameCursor);
            // 4. Render Pipeline Execution
            const RenderScene scene = game.buildScene();
            renderer.render(scene, reading.framebufferSize.x, reading.framebufferSize.y);
            // Display visual overlays for debugging if toggled on
            if (context.showDebugOverlay)
            {
                drawDebugTools( game.debugOverlay( static_cast<float>(atlasTexture.width),
                                                   internalCursor,
                                                   context.debugBodyIndex),
                                windowViewport);
            }
            imgui.render();   // Complete UI draw pipelines
            window.present(); // Swap display buffers
        }
        return EXIT_SUCCESS;
    }
}
