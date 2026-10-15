/**
 * @file stopwatch.cpp
 * @brief Measures real-world elapsed seconds and monitors performance metrics using a monotonic
 * system clock.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#include "simple_platformer/timing/stopwatch.hpp"

#include <chrono>

namespace simple_platformer
{
    // Constructor: Captures the initial high-resolution timeline anchor and zeroes telemetry
    // balances
    Stopwatch::Stopwatch()
        : start(std::chrono::steady_clock::now()),
          nFrames(0),
          totalFrameTime(0.0f),
          frameRate(0.0f)
    {
    }

    // Computes absolute chronological duration elapsed since the system initialization timestamp
    float Stopwatch::elapsedSeconds() const noexcept
    {
        return std::chrono::duration<float>(std::chrono::steady_clock::now() - start).count();
    }

    // Calculates delta step durations, advancing historical sample sets and smoothing out current
    // framerate metrics
    float Stopwatch::lapSeconds()
    {
        const auto now = std::chrono::steady_clock::now();
        const float elapsed = std::chrono::duration<float>(now - start).count();
        start =
            now; // Shift time reference anchor forward to calculate the next delta interval loop

        // Telemetry accumulation logic
        nFrames++;
        totalFrameTime += elapsed;

        // Sliding Window Performance Filter: Update the active framerate metric every 1 second
        if (totalFrameTime > 1.0f)
        {
            frameRate = static_cast<float>(nFrames) / totalFrameTime;
            nFrames = 0; // Reset loop counters for the subsequent sampling window
            totalFrameTime = 0.0f;
        }
        return elapsed;
    }

    // Fetches the most recently computed performance metric value (Frames Per Second)
    float Stopwatch::getFramerate()
    {
        return frameRate;
    }
}
