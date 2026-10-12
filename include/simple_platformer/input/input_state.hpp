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
        Count /// Helper definition tracking the total number of enum actions.
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
        /// Maintain the existing grip state without modifications.
        Keep,
        /// Latch onto a contacted wall or ladder, or continue holding the current surface.
        Hold,
        /// Intentionally let go of the currently scaled wall or ladder surface.
        Release
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
        /// 2D movement navigation request vector (normalized values ranging between -1.0 and 1.0).
        glm::vec2 direction = {0.0F, 0.0F};
        /// 2D target aim pointing vector for targeting projectile paths.
        glm::vec2 aimDirection = {0.0F, 0.0F};
        /// Flag indicating the jump action button transitioned to a down state during this frame
        /// window.
        bool jumpPressed = false;
        /// Flag tracking if the jump button remains continuously held (useful for variable jump
        /// heights).
        bool jumpHeld = false;
        /// Flag indicating the primary action attack command transitioned to down during this frame
        /// window.
        bool primaryAttackPressed = false;
        /// Context flag dictating changes to ladder or wall scaling logic this tick.
        ClimbGrip climbGrip = ClimbGrip::Keep;
        /// True if AI or input scripts request stopping short of edge drop-offs while walking.
        bool avoidLedges = false;
        /// Flag enabling or disabling the continuous contact-damage checking process if supported.
        bool contactDamage = false;
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
        /// Internal array count mapping raw storage limits.
        static constexpr std::size_t ButtonCount = static_cast<std::size_t>(InputButton::Count);

        /// Tracks continuous hold actions down per slot.
        std::array<bool, ButtonCount> held{};
        /// Edge-trigger array capturing low-to-high state updates until consumed.
        std::array<bool, ButtonCount> pressed{};
        /// Edge-trigger array capturing high-to-low state updates until consumed.
        std::array<bool, ButtonCount> released{};
    };
}
