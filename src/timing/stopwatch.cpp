#include "simple_platformer/timing/stopwatch.hpp"

#include <chrono>

namespace simple_platformer
{
    Stopwatch::Stopwatch()
        : start(std::chrono::steady_clock::now()),
          nFrames(0),
          totalFrameTime(0.0f),
          frameRate(0.0f)
    {
    }

    float Stopwatch::elapsedSeconds() const noexcept
    {
        return std::chrono::duration<float>(std::chrono::steady_clock::now() - start).count();
    }

    float Stopwatch::lapSeconds()
    {
        const auto now = std::chrono::steady_clock::now();
        const float elapsed = std::chrono::duration<float>(now - start).count();
        start = now;

        nFrames++;
        totalFrameTime += elapsed;
        if (totalFrameTime > 1.0f)
        {
            frameRate = nFrames / totalFrameTime;
            nFrames = 0;
            totalFrameTime = 0.0f;
        }
        return elapsed;
    }

            // Get the frame rate
    float Stopwatch::getFramerate()
    {
        return frameRate;
    }

}
