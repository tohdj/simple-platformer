/**
 * @file npc.hpp
 * @brief State variables, behavioral tactics, sensory configurations, and memory metrics for NPC AI
 * systems.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#pragma once

#include <optional>

#include <glm/vec2.hpp>

#include "simple_platformer/actor/actor_id.hpp"

namespace simple_platformer
{
    /**
     * @brief Classifies the distinct operational states in which an NPC's cognitive loop can
     * execute.
     */
    enum class NpcState
    {
        Idle,
        Patrol,
        Chase,
        Bite,
        Shoot,
        Search,
        Retreat,
        Watch,
        Flee,
        Sleep,
        Charge,
        Stunned
    };

    /**
     * @brief Determines the foundational behavioral playbook guiding an NPC's state transition
     * rules.
     */
    enum class NpcTactic
    {
        Pursuer,      /**< Closes in aggressively; searches last known positions upon losing
                         line-of-sight tracking. */
        KeepDistance, /**< Relocates backward within an established standoff radius; watches
                         defensively if the target drops out of sight. */
        Coward,       /**< Flees rapidly from nearby threats; initiates bite counter-attacks only at
                         emergency close proximity. */
        Charger /**< Awakes instantly on nearby acoustic landing impulses; holds a linear ramming
                   vector until blocked by structural walls. */
    };

    /**
     * @brief Persistent cognitive core storing strategic memory and timing metrics across frames.
     */
    struct NpcBrain
    {
        NpcTactic tactic =
            NpcTactic::Pursuer; /**< The governing strategy archetype directing state changes. */
        NpcState state = NpcState::Idle; /**< Active state loop context executing this frame. */
        float stateElapsed =
            0.0F; /**< Continuous elapsed duration in seconds spent within the active state. */
        float targetLostElapsed =
            0.0F; /**< Continuous elapsed duration in seconds since a living target was last
                     verified, crucial for fleeing behaviors. */
        float chargeDirection = 1.0F; /**< Cached horizontal signing vector established at charge
                                         initiation and locked until recovery. */
        std::optional<ActorId>
            target; /**< Unique tracked identifier tracking the current hunted opponent entity. */
        glm::vec2 lastKnownTargetFeet = {
            0.0F,
            0.0F}; /**< Spatial world coordinates tracking the threat's last confirmed baseline
                      footprint position. */
        float targetMemoryRemaining = 0.0F; /**< Remaining active countdown timer in seconds before
                                               cached spatial threat target information expires. */
    };

    /**
     * @brief Transient sensory data refreshed completely on every environmental perception loop.
     */
    struct NpcPerception
    {
        bool targetVisible = false; /**< True if a clean, unobstructed line of sight is established
                                       to the target actor. */
        bool heardLanding = false;  /**< True if a player landing noise event was registered within
                                       acoustic ranges during this sensory frame. */
    };

    /**
     * @brief Static parameter limits configuring an NPC's baseline sensory thresholds and ranges.
     */
    struct NpcSenses
    {
        float noticeDistance = 96.0F;      /**< Absolute sight or primary awareness radius threshold
                                              measured in world pixels. */
        float targetMemoryDuration = 1.5F; /**< Total continuous lifespan allocation in seconds
                                              given to target memory tracking before fading. */
        float searchDuration =
            2.0F; /**< Timeline duration allocated to active hunting at last-known positions before
                     returning to standard loops. Zero skips searching. */
        float standoffDistance =
            48.0F; /**< Strategic radius boundary evaluated by specific tactical algorithms to drive
                      fleeing or shooting standoffs. */
    };

    /**
     * @brief Linear guard routing nodes storing spatial parameters for periodic movement cycles.
     */
    struct Patrol
    {
        glm::vec2 firstFeet = {0.0F, 0.0F};  /**< Baseline pixel world position defining the origin
                                                anchor node of the patrol path. */
        glm::vec2 secondFeet = {0.0F, 0.0F}; /**< Baseline pixel world position defining the
                                                termination anchor node of the patrol path. */
        bool headingToSecond = true; /**< Flag tracking the active movement vector towards the
                                        termination node; switches upon reaching targets. */
    };
}
