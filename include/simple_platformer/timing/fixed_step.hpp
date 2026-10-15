/**
 * @file fixed_step.hpp
 * @brief Fixed-timestep loop engine accumulator for decoupling deterministic simulation steps from
 * variable frame rates.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#pragma once

#include <cstddef>
#include <functional>

namespace simple_platformer
{
    // The standard target simulation timestep in seconds (matches a 60Hz tick rate).
    constexpr double FixedDeltaSeconds = 1.0 / 60.0;

    // The maximum real-time duration allowed per frame to prevent a "spiral of death" cascade when
    // performance hitches occur.
    constexpr double MaximumFrameSeconds = 0.25;

    // The data payload returned after processing a frame update cycle.
    struct FixedStepResult
    {
        std::size_t updates =
            0; // Total number of fixed simulation steps evaluated during this frame call.
        double interpolation =
            0.0; // Blending factor scalar (0.0 to 1.0) tracking remaining time, used to smoothly
                 // interpolate rendering transforms between physics frames.
    };

    // Manages a fixed-timestep game loop accumulator.
    // Ensures physics and game simulation run at a constant deterministic tick rate, decoupled from
    // variable graphics frame rates.
    class FixedStep
    {
    public:
        // Constructs a new Fixed Step loop engine profile using the target step and max frame clamp
        // thresholds.
        explicit FixedStep(
            double stepSeconds = FixedDeltaSeconds,
            double maximumFrameSeconds = MaximumFrameSeconds);

        // Accumulates real elapsed frame time and fires simulation tick steps accordingly.
        // Clamps input frame time to the configured maximum, appends it to the accumulator, and
        // invokes the callback for each complete timestep met.
        FixedStepResult advance(double frameSeconds, const std::function<void(float)>& fixedUpdate);

        // Flushes out all accumulated fractions and resets the structural time state tracking
        // metrics.
        void reset();

        // Read-only accessor for the targeted physics timestep duration in seconds.
        double stepSeconds() const;

        // Read-only accessor for the remaining fractional time values currently buffered in the
        // accumulator.
        double pendingSeconds() const;

    private:
        double step;         // The immutable duration window size assigned per step.
        double maximumFrame; // The safety ceiling bound allowed for incoming real frame durations.
        double accumulator = 0.0; // Internal buffer accumulating fractions of frame time until they
                                  // cross the tick threshold.
    };
}
