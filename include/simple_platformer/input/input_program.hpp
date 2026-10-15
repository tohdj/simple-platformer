/**
 * @file input_program.hpp
 * @brief Scripted input macros, time-slice replication steps, and timeline sampling utilities for
 * automated replays.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#pragma once

#include <vector>

#include "simple_platformer/input/input_state.hpp"

namespace simple_platformer
{
    // A timed sequence segment tracking a static set of inputs for automation or path-finding
    // replay. Replay evaluation returns edge flags consistently on every call within this timeframe
    // block. The step durations must be aligned with fixed physics increments.
    struct InputStep
    {
        float duration =
            0.0F; // Total execution duration in seconds to continuously feed these inputs.
        InputIntentions intentions; // The set of input commands to mimic during this time slice.
    };

    // A linear collection of input stages defining a scripted input macro sequence.
    using InputProgram = std::vector<InputStep>;

    // Calculates the total continuous timeline duration of an entire input macro program.
    // Every sub-step in the collection must specify a finite, positive time factor greater than
    // zero.
    float durationOf(const InputProgram& program);

    // Samples the input profile timeline at a specific point in time to extract matching command
    // choices. Returns an empty default intentions payload if the playback time exceeds total
    // timeline limits.
    InputIntentions replayInput(const InputProgram& program, float elapsed);
}
