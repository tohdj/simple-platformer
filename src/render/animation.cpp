/**
 * @file animation.hpp
 * @brief Component declarations, playback tracking, and state machines for actor sprite
 * animations.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#include "simple_platformer/render/animation.hpp"

#include <algorithm>
#include <cstddef>
#include <stdexcept>

#include <glm/vec2.hpp>

#include "simple_platformer/math/validation.hpp"
#include "simple_platformer/render/sprite.hpp"

namespace simple_platformer
{
    namespace
    {
        // Resolves standard locomotion and aerial clips based on kinetic landing data and
        // directional velocities
        AnimationName selectMovementAnimation(bool grounded, glm::vec2 velocity)
        {
            // 1. Parse airborne states when the character model is disconnected from ground
            // elements
            if (!grounded)
            {
                // Negative Y velocity maps to rising upward thrust, positive Y maps to downward
                // gravity pull
                return velocity.y < 0.0F ? AnimationName::Jump : AnimationName::Fall;
            }

            // 2. Parse terrestrial navigation loops when standing firmly over solid level
            // coordinates
            return velocity == glm::vec2{0.0F, 0.0F} ? AnimationName::Idle : AnimationName::Move;
        }
    }

    // Performs a safe dictionary lookup to isolate a targeting clip signature inside the set
    // database
    const AnimationClip& clipFor(const AnimationSet& animationSet, AnimationName name)
    {
        const auto clip = std::find_if(
            animationSet.clips.begin(),
            animationSet.clips.end(),
            [name](const AnimationClip& candidate) { return candidate.name == name; });
        if (clip == animationSet.clips.end())
        {
            throw std::invalid_argument(
                "The animation set does not contain the selected animation");
        }
        return *clip;
    }

    // Maps runtime time counters to concrete atlas frame array indices based on clip wrapping
    // properties
    const SpriteRegion& frameAt(const AnimationClip& clip, float elapsedSeconds)
    {
        if (clip.frames.empty())
        {
            throw std::invalid_argument("Animation clips require at least one frame");
        }
        if (!isFinitePositive(clip.frameDuration) || !isFiniteNonNegative(elapsedSeconds))
        {
            throw std::invalid_argument("Animation timing must be positive and finite");
        }

        // Convert the elapsed running timeline down into raw frame index steps
        std::size_t frame = static_cast<std::size_t>(elapsedSeconds / clip.frameDuration);
        if (clip.looping)
        {
            frame %= clip.frames.size(); // Wrap index counts indefinitely across looping clips
        }
        else
        {
            frame = std::min(
                frame,
                clip.frames.size() - 1); // Freeze onto the final frame index for terminal clips
        }

        return clip.frames[frame];
    }

    // Handles contextual state swaps, ticks track clock variables, and binds the output atlas
    // texture sheet coordinates
    void updateAnimation(
        Animator& animator,
        Sprite& sprite,
        AnimationName selected,
        float deltaTime)
    {
        requireSeconds(deltaTime, "Animations time step");

        // Reset state clocks instantly if logic demands a clip state transition path
        if (animator.current != selected)
        {
            animator.current = selected;
            animator.elapsed = 0.0F;
        }
        else
        {
            animator.elapsed += deltaTime; // Advance playback runtime tracker continuously
        }

        // Locate correct asset clips and fetch current texture coordinates
        sprite.region = frameAt(clipFor(animator.animationSet, selected), animator.elapsed);
    }

    // Prioritization State Machine: filters active behavioral components to select the proper clip
    // path
    AnimationName selectActorAnimation(
        bool dying,
        bool attacking,
        bool grounded,
        glm::vec2 velocity)
    {
        // 1. Highest Priority: Terminal state overrides active actions completely
        if (dying)
        {
            return AnimationName::Death;
        }

        // 2. Secondary Priority: Combat sequences override standard movement loops
        if (attacking)
        {
            return AnimationName::Attack;
        }

        // 3. Fallback: Default back to locomotion parameters if no action locks exist
        return selectMovementAnimation(grounded, velocity);
    }
}
