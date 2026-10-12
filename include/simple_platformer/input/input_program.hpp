#pragma once

#include <vector>

#include "simple_platformer/input/input_state.hpp"

namespace simple_platformer
{
    /**
     * @brief A timed sequence segment tracking a static set of inputs for automation or testing.
     *
     * Replay evaluation returns edge flags (like initial press events) consistently on every call
     * within this timeframe block. The step durations must be aligned with fixed physics
     * increments.
     */
    struct InputStep
    {
        /// Total execution duration in seconds to continuously feed these inputs.
        float duration = 0.0F;
        /// The set of input commands to mimic during this time slice.
        InputIntentions intentions;
    };

    /// A linear collection of input stages defining a scripted input macro sequence.
    using InputProgram = std::vector<InputStep>;

    /**
     * @brief Calculates the total continuous timeline duration of an entire input macro program.
     *
     * Every sub-step in the collection must specify a finite, positive time factor greater than
     * zero.
     *
     * @param program The sequenced collection of input frames to profile.
     * @return The absolute time length in seconds for the entire automated run.
     */
    float durationOf(const InputProgram& program);

    /**
     * @brief Samples the input profile timeline at a specific point in time.
     *
     * Evaluates sequence boundaries to extract intent values mapping to the time scale index.
     *
     * @param program The sequenced collection of input frames to read.
     * @param elapsed Total execution runtime in seconds tracking playback progression.
     * @return The active InputIntentions configuration context, or an empty default payload if the
     * timeline has expired.
     */
    InputIntentions replayInput(const InputProgram& program, float elapsed);
}
