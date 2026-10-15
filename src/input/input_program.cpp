/**
 * @file input_program.cpp
 * @brief Scripted input macros, time-slice replication steps, and timeline sampling utilities for
 * automated replays.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#include "simple_platformer/input/input_program.hpp"

#include <stdexcept>

#include "simple_platformer/input/input_state.hpp"
#include "simple_platformer/math/validation.hpp"

namespace simple_platformer
{
    // Accumulates individual step durations to determine the total continuous macro runtime length
    float durationOf(const InputProgram& program)
    {
        float duration = 0.0F;
        for (const InputStep& step : program)
        {
            // Enforce structural timing integrity constraints on every sequence block step
            if (!isFinitePositive(step.duration))
            {
                throw std::invalid_argument("Input program durations must be finite and positive");
            }
            duration += step.duration;
        }
        return duration;
    }

    // Iterates through input blocks chronologically to isolate and extract intent packets mapped to an elapsed timestamp
    InputIntentions replayInput(const InputProgram& program, float elapsed)
    {
        if (!isFiniteNonNegative(elapsed))
        {
            throw std::invalid_argument(
                "Input program elapsed time must be finite and non-negative");
        }

        float endsAt = 0.0F;
        for (const InputStep& step : program)
        {
            if (!isFinitePositive(step.duration))
            {
                throw std::invalid_argument("Input program durations must be finite and positive");
            }
            
            // Advance the timeline marker boundary by adding the current segment duration window
            endsAt += step.duration;
            
            // Return the matching intentions package immediately if the target timestamp falls inside this step's window
            if (elapsed < endsAt)
            {
                return step.intentions;
            }
        }
        return {}; // Return an unmapped, empty control intent layout if the macro timeline has expired
    }
}
