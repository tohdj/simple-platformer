/**
 * @file input_state.hpp
 * @brief Enums, state snapshot data structures, and edge-trigger tracking buffers for digital game
 * inputs.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#pragma once

#include <array>
#include <cstddef>

#include <glm/vec2.hpp>

namespace simple_platformer
{
    /**
     * @brief Maps physical keyboard keys or gamepad inputs to generic digital game actions.
     */
    enum class InputButton
    {
        Left,
        Right,
        Up,
        Down,
        Jump,
        PrimaryAttack,
        Count /**< Helper definition tracking the total number of enum actions. */
    };

    /**
     * @brief Expresses how the current tick framework modifies an actor's ladder or wall grip
     * state.
     *
     * Defaulting to `Keep` ensures that callers can repeatedly submit movement tracking requests
     * without accidentally triggering unexpected grabs or drops.
     */
    enum class ClimbGrip
    {
        Keep,   /**< Maintain the existing grip state without modifications. */
        Hold,   /**< Latch onto a contacted wall or ladder, or continue holding the current surface.
                 */
        Release /**< Intentionally let go of the currently scaled wall or ladder surface. */
    };

    /**
     * @brief A decoupled, high-level intent payload representing raw commands interpreted as
     * actions.
     *
     * This snapshot decouples system polling (like keyboard hardware layers) from actors and
     * physics code.
     */
    struct InputIntentions
    {
        glm::vec2 direction = {0.0F, 0.0F}; /**< 2D movement navigation request vector (normalized
                                               values ranging between -1.0 and 1.0). */
        glm::vec2 aimDirection = {
            0.0F,
            0.0F}; /**< 2D target aim pointing vector for targeting projectile paths. */
        bool jumpPressed = false; /**< Flag indicating the jump action button transitioned to a down
                                     state during this frame window. */
        bool jumpHeld = false;    /**< Flag tracking if the jump button remains continuously held
                                     (useful for variable jump heights). */
        bool primaryAttackPressed = false; /**< Flag indicating the primary action attack command
                                              transitioned to down during this frame window. */
        ClimbGrip climbGrip = ClimbGrip::Keep; /**< Context flag dictating changes to ladder or wall
                                                  scaling logic this tick. */
        bool avoidLedges = false;   /**< True if AI or input scripts request stopping short of edge
                                       drop-offs while walking. */
        bool contactDamage = false; /**< Flag enabling or disabling the continuous contact-damage
                                       checking process if supported. */
    };

    /**
     * @brief Buffer tracker managing down, pressed, and released states for mapped buttons.
     */
    class InputState
    {
    public:
        /**
         * @brief Updates the registration state of a specific action button.
         * @param button The targeted mapping action to alter.
         * @param down True if the button is physically held down, false otherwise.
         */
        void setButton(InputButton button, bool down);

        /**
         * @brief Flushes all buffered edge flags down for a targeted action button instantly.
         * @param button The target mapping action to clear out.
         */
        void clearButton(InputButton button);

        /**
         * @brief Checks if a digital button action is actively being held down.
         * @param button The action token to evaluate.
         * @return true if held, false otherwise.
         */
        bool isHeld(InputButton button) const;

        /**
         * @brief Checks if an action key transitioned to down within this frame block.
         * @param button The action token to evaluate.
         * @return true if pressed this frame, false otherwise.
         */
        bool wasPressed(InputButton button) const;

        /**
         * @brief Checks if an action key transitioned to up within this frame block.
         * @param button The action token to evaluate.
         * @return true if released this frame, false otherwise.
         */
        bool wasReleased(InputButton button) const;

        /**
         * @brief Packs current triggers into an intention snapshot and flushes transition flags.
         *
         * Transition flags (pressed and released edges) remain buffered until a fixed physics
         * update cycle explicitly consumes them.
         *
         * @return An InputIntentions payload configured with the current frame triggers.
         */
        InputIntentions consumeIntentions();

    private:
        static constexpr std::size_t ButtonCount = static_cast<std::size_t>(
            InputButton::Count); /**< Internal array count mapping raw storage limits. */

        std::array<bool, ButtonCount> held{}; /**< Tracks continuous hold actions down per slot. */
        std::array<bool, ButtonCount> pressed{}; /**< Edge-trigger array capturing low-to-high state
                                                    updates until consumed. */
        std::array<bool, ButtonCount> released{}; /**< Edge-trigger array capturing high-to-low
                                                     state updates until consumed. */
    };
}
