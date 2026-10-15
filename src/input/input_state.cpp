/**
 * @file input_state.cpp
 * @brief Enums, state snapshot data structures, and edge-trigger tracking buffers for digital game
 * inputs.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#include "simple_platformer/input/input_state.hpp"

#include <algorithm>
#include <cstddef>
#include <stdexcept>

namespace simple_platformer
{
    namespace
    {
        // Safe conversion utility checking enum boundaries to isolate index mappings safely
        std::size_t indexOf(InputButton button)
        {
            const std::size_t index = static_cast<std::size_t>(button);
            if (index >= static_cast<std::size_t>(InputButton::Count))
            {
                throw std::invalid_argument("Input button is invalid");
            }

            return index;
        }

        // Resolves opposite boolean direction flags down into a single steering float axis scale
        // (-1.0 to 1.0)
        float axis(bool negative, bool positive)
        {
            return static_cast<float>(positive) - static_cast<float>(negative);
        }
    }

    // Updates button tracks, managing transient state indicators upon register level value switches
    void InputState::setButton(InputButton button, bool down)
    {
        const std::size_t index = indexOf(button);
        if (held[index] == down)
        {
            return; // No value adjustment necessary if button state matches active logs
        }

        held[index] = down;

        // Latch edge triggers safely to prevent missing rapid key taps between hardware ticks
        pressed[index] = pressed[index] || down;
        released[index] = released[index] || !down;
    }

    // Completely purges all tracking variables associated with a single key mapping entry
    void InputState::clearButton(InputButton button)
    {
        const std::size_t index = indexOf(button);
        held[index] = false;
        pressed[index] = false;
        released[index] = false;
    }

    // Returns true continuously as long as the hardware key remains actively engaged
    bool InputState::isHeld(InputButton button) const
    {
        return held[indexOf(button)];
    }

    // Returns true if the key transitioned to a pressed state during the current frame update
    // window
    bool InputState::wasPressed(InputButton button) const
    {
        return pressed[indexOf(button)];
    }

    // Returns true if the key transitioned to a released state during the current frame update
    // window
    bool InputState::wasReleased(InputButton button) const
    {
        return released[indexOf(button)];
    }

    // Compiles active hardware button snapshots into a clean locomotion intentions package, then
    // clears edge latches
    InputIntentions InputState::consumeIntentions()
    {
        InputIntentions intentions;

        // Map discrete directional layouts to full 2D vector steering coordinates
        intentions.direction = {
            axis(isHeld(InputButton::Left), isHeld(InputButton::Right)),
            axis(isHeld(InputButton::Up), isHeld(InputButton::Down))};

        intentions.jumpPressed = wasPressed(InputButton::Jump);
        intentions.jumpHeld = isHeld(InputButton::Jump);
        intentions.primaryAttackPressed = wasPressed(InputButton::PrimaryAttack);

        // Wipe edge trigger trackers instantly to start a fresh measurement frame cycle
        std::fill(pressed.begin(), pressed.end(), false);
        std::fill(released.begin(), released.end(), false);

        return intentions;
    }
}
