#pragma once

#include <cstddef>
#include <functional>

namespace simple_platformer
{
    /// The standard target simulation timestep in seconds (matches a 60Hz tick rate).
    constexpr double FixedDeltaSeconds = 1.0 / 60.0;

    /// The maximum real-time duration allowed per frame to prevent a "spiral of death"
    /// cascade when performance hitches occur.
    constexpr double MaximumFrameSeconds = 0.25;

    /**
     * @brief The data payload returned after processing a frame update cycle.
     */
    struct FixedStepResult
    {
        /// Total number of fixed simulation steps evaluated during this frame call.
        std::size_t updates = 0;

        /// Blending factor scalar (ranging from 0.0 to 1.0) tracking the remaining time,
        /// used to smoothly interpolate rendering transforms between physics frames.
        double interpolation = 0.0;
    };

    /**
     * @brief Manages a fixed-timestep game loop accumulator.
     *
     * Ensures physics and game simulation run at a constant deterministic tick rate,
     * decoupled from variable graphics frame rates.
     */
    class FixedStep
    {
    public:
        /**
         * @brief Constructs a new Fixed Step loop engine profile.
         * @param stepSeconds The desired time slice per physics update step. Defaults to 60Hz.
         * @param maximumFrameSeconds Upper frame duration threshold limit to clamp large delta
         * spikes.
         */
        explicit FixedStep(
            double stepSeconds = FixedDeltaSeconds,
            double maximumFrameSeconds = MaximumFrameSeconds);

        /**
         * @brief Accumulates real elapsed frame time and fires simulation tick steps accordingly.
         *
         * Clamps the input frame time to the configured maximum, appends it to the time
         * accumulator, and loops executing the `fixedUpdate` callback for every complete timestep
         * window met.
         *
         * @param frameSeconds The actual measured real-world time elapsed since the previous
         * graphics frame.
         * @param fixedUpdate The callback function invoked for every individual simulation tick
         * step.
         * @return A FixedStepResult detailing the execution metrics and sub-frame render blending
         * factor.
         */
        FixedStepResult advance(double frameSeconds, const std::function<void(float)>& fixedUpdate);

        /**
         * @brief Flushes out all accumulated fractions and resets the structural time state
         * tracking metrics.
         */
        void reset();

        /// Read-only accessor for the targeted physics timestep duration in seconds.
        double stepSeconds() const;

        /// Read-only accessor for the remaining fractional time values currently buffered in the
        /// accumulator.
        double pendingSeconds() const;

    private:
        /// The immutable duration window size assigned per step.
        double step;
        /// The safety ceiling bound allowed for incoming real frame durations.
        double maximumFrame;
        /// Internal buffer accumulating fractions of frame time until they cross the tick
        /// threshold.
        double accumulator = 0.0;
    };
}
