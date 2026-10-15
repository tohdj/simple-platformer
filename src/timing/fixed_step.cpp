/**
 * @file fixed_step.cpp
 * @brief Fixed-timestep loop engine accumulator for decoupling deterministic simulation steps from
 * variable frame rates.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#include "simple_platformer/timing/fixed_step.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <stdexcept>

namespace simple_platformer
{
    // Constructor: Assigns the fixed update frequency and the security cap, enforcing positive data
    // ranges
    FixedStep::FixedStep(double stepSeconds, double maximumFrameSeconds)
        : step(stepSeconds),
          maximumFrame(maximumFrameSeconds)
    {
        if (!std::isfinite(step) || step <= 0.0)
            throw std::invalid_argument("Fixed step must be finite and above zero");

        if (!std::isfinite(maximumFrame) || maximumFrame < 0.0)
            throw std::invalid_argument("Maximum frame time must be finite and non-negative");
    }

    // Steps forward the accumulator with real frame time, executing the fixed update tick callback
    // sequentially
    FixedStepResult FixedStep::advance(
        double frameSeconds,
        const std::function<void(float)>& fixedUpdate)
    {
        if (!std::isfinite(frameSeconds))
            throw std::invalid_argument("Frame time must be finite");

        // Clamp input time to prevent "spiral of death" lockups when processing heavy lag spikes
        accumulator += std::clamp(frameSeconds, 0.0, maximumFrame);

        // Consume accumulator chunks in fixed intervals to run deterministic physics/logic updates
        std::size_t updates = 0;
        while (accumulator >= step)
        {
            accumulator -= step;
            fixedUpdate(static_cast<float>(step));
            ++updates;
        }

        // Return total step ticks processed and the remainder percentage for visual rendering
        // interpolation
        return {updates, accumulator / step};
    }

    // Resets the residual time accumulator value back to zero
    void FixedStep::reset()
    {
        accumulator = 0.0;
    }

    // Returns the targeted fixed step interval time size configuration
    double FixedStep::stepSeconds() const
    {
        return step;
    }

    // Returns the current leftover time balance awaiting consumer execution loops
    double FixedStep::pendingSeconds() const
    {
        return accumulator;
    }
}
