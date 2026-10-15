/**
 * @file animation.hpp
 * @brief Component declarations, playback tracking, and state machines for actor sprite
 * animations.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */                                                                                                \

#pragma once

#include <vector>

#include <glm/vec2.hpp>

#include "simple_platformer/render/sprite.hpp"

namespace simple_platformer
{
    // Supported states for actor animations.
    enum class AnimationName
    {
        Idle,
        Move,
        Jump,
        Fall,
        Attack,
        Death
    };

    // Holds data for a single sequence of animation frames.
    struct AnimationClip
    {
        AnimationName name = AnimationName::Idle; // The identifier for this specific clip.
        std::vector<SpriteRegion> frames; // Sub-regions of the texture atlas making up the frames.
        float frameDuration = 0.1F;       // Time in seconds to display each individual frame.
        bool looping = true; // True if the animation should wrap around when it finishes.
    };

    // A collection of animation clips available to an entity.
    struct AnimationSet
    {
        std::vector<AnimationClip> clips; // List of clips defined for the actor or entity type.
    };

    // Component managing the active animation state and playback runtime.
    struct Animator
    {
        AnimationName current =
            AnimationName::Idle;   // The clip key currently being tracked and evaluated.
        float elapsed = 0.0F;      // Cumulative playback time in seconds spent on the current clip.
        AnimationSet animationSet; // The collection of animations available to this instance.
    };

    // Looks up a specific animation clip within an animation set.
    const AnimationClip& clipFor(const AnimationSet& animationSet, AnimationName name);

    // Determines the exact sprite frame to display based on elapsed time.
    const SpriteRegion& frameAt(const AnimationClip& clip, float elapsedSeconds);

    // Advances playback state, handles transitions, and binds the calculated frame data to a
    // sprite.
    void updateAnimation(
        Animator& animator,
        Sprite& sprite,
        AnimationName selected,
        float deltaTime);

    // State machine helper that resolves an actor's animation based on status flags in priority
    // order.
    AnimationName selectActorAnimation(
        bool dying,
        bool attacking,
        bool grounded,
        glm::vec2 velocity);
}
