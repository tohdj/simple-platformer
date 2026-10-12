/**
 * @file stopwatch.hpp
 * @brief Measures real-world elapsed seconds and monitors performance metrics using a monotonic
 * system clock.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#pragma once

#include <chrono>

namespace simple_platformer
{
    /**
     * @brief Measures real-world elapsed seconds using a steady monotonic system clock.
     *
     * @note Gameplay logic should generally receive a decoupled `deltaTime` parameter instead of
     * polling this directly, allowing test suites to mock or advance simulations deterministically.
     */
    class Stopwatch
    {
    public:
        /**
         * @brief Initializes a new Stopwatch and starts the real-time tracker.
         */
        Stopwatch();

        /**
         * @brief Measures the total accumulated seconds since the Stopwatch was constructed.
         * @return A thread-safe, non-mutating float value representing elapsed time in seconds.
         */
        float elapsedSeconds() const noexcept;

        /**
         * @brief Measures seconds since construction or the last lap snapshot, then restarts the
         * clock interval.
         *
         * Typically called once per frame by the primary application loop to calculate game engine
         * frame time.
         *
         * @return A float value representing delta seconds elapsed during this interval.
         */
        float lapSeconds();

        /**
         * @brief Calculates the ongoing average frame execution frequency.
         * @return The frame rate expressed in Frames Per Second (FPS).
         */
        float getFramerate();

    private:
        std::chrono::steady_clock::time_point
            start;   /**< The fixed time anchor point of the active interval. */
        int nFrames; /**< Total count of frame iterations registered in the active calculation
                        cycle. */
        float totalFrameTime; /**< Accumulated delta durations tracked for computing the average
                                 frequency. */
        float frameRate;      /**< The latest computed frames-per-second performance metric. */
    };
}
