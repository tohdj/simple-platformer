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
    /**
     * @brief Supported states for actor animations.
     */
    enum class AnimationName
    {
        Idle,
        Move,
        Jump,
        Fall,
        Attack,
        Death
    };

    /**
     * @brief Holds data for a single sequence of animation frames.
     */
    struct AnimationClip
    {
        /// The identifier for this specific clip.
        AnimationName name = AnimationName::Idle;
        /// Sub-regions of the texture atlas making up the frames.
        std::vector<SpriteRegion> frames;
        /// Time in seconds to display each individual frame.
        float frameDuration = 0.1F;
        /// True if the animation should wrap around when it finishes.
        bool looping = true;
    };

    /**
     * @brief A collection of animation clips available to an entity.
     */
    struct AnimationSet
    {
        /// List of clips defined for the actor or entity type.
        std::vector<AnimationClip> clips;
    };

    /**
     * @brief Component managing the active animation state and playback runtime.
     */
    struct Animator
    {
        /// The clip key currently being tracked and evaluated.
        AnimationName current = AnimationName::Idle;
        /// Cumulative playback time in seconds spent on the current clip.
        float elapsed = 0.0F;
        /// The collection of animations available to this instance.
        AnimationSet animationSet;
    };

    /**
     * @brief Looks up a specific animation clip within an animation set.
     * @param animationSet The source collection of clips.
     * @param name The target clip identifier to find.
     * @return Reference to the matching clip, or a fallback (e.g., Idle) if not found.
     */
    const AnimationClip& clipFor(const AnimationSet& animationSet, AnimationName name);

    /**
     * @brief Determines the exact sprite frame to display based on elapsed time.
     * @param clip The source animation clip containing the frames.
     * @param elapsedSeconds Total playback time advanced on this clip.
     * @return Reference to the active SpriteRegion structure.
     */
    const SpriteRegion& frameAt(const AnimationClip& clip, float elapsedSeconds);

    /**
     * @brief Advances playback state and binds the calculated frame data to a sprite.
     *
     * Handles state transitions if `selected` differs from `animator.current`, increments
     * the elapsed time tracker, tracks loop limits, and updates the target sprite region.
     *
     * @param animator The runtime playback state to update.
     * @param sprite The target renderable object receiving the frame update.
     * @param selected The intended animation state requested by logic this frame.
     * @param deltaTime Elapsed frame time in seconds.
     */
    void updateAnimation(
        Animator& animator,
        Sprite& sprite,
        AnimationName selected,
        float deltaTime);

    /**
     * @brief State machine helper that resolves an actor's animation based on status flags.
     *
     * Evaluates flags in priority order (e.g., Death overrides Attack, Attack overrides Movement).
     *
     * @param dying Is the actor undergoing a death sequence?
     * @param attacking Is the actor currently performing an attack action?
     * @param grounded Is the actor standing firmly on static geometry?
     * @param velocity The current 2D physics velocity vector of the actor.
     * @return The determined AnimationName state matching the conditions.
     */
    AnimationName selectActorAnimation(
        bool dying,
        bool attacking,
        bool grounded,
        glm::vec2 velocity);
}
