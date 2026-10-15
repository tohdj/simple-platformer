/**
 * @file npc_facts.hpp
 * @brief Telemetry aggregation and conditional fact parsing for NPC AI decision-making.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#pragma once

namespace simple_platformer
{
    class TileMap;
    struct Actor;
    struct NpcBrain;
    struct NpcPerception;

    // A consolidated snapshot of environmental, sensory, and kinematic data used to evaluate AI
    // state transitions. Isolating this state data into a clean structure keeps cognitive state
    // decisions highly testable and decoupled.
    struct NpcFacts
    {
        bool targetKnown = false; // True if a living target is currently remembered, regardless of
                                  // immediate visibility.
        bool targetVisible =
            false; // True if a direct, unobstructed line of sight to the target exists.
        bool targetInBiteRange = false; // True if the target is visible and falls within the active
                                        // overlapping bounds of the close-range bite hitbox.
        bool biteReady =
            false; // True if the internal close-range melee action cooldown has completely reset.
        bool targetInSights = false; // True if the target is visible and the NPC currently holds a
                                     // functional ranged weapon.
        bool targetWithinStandoffDistance =
            false; // True if the distance to the target's last known footprint is strictly within
                   // the configured tactical standoff range.
        bool heardLanding = false; // True if an audio impact event signature was intercepted during
                                   // this sensory update.
        bool targetOnSameRun = false; // True if the target occupies the same horizontal plane level
                                      // (used by charging behavior even without direct sight).
        bool targetWithinNoticeDistance =
            false; // True if the target's current absolute positioning falls within the general
                   // awareness threshold radius.
        bool movementBlocked = false; // True if the physical locomotion body is currently arrested
                                      // by a solid wall or obstacle block.
        bool hasPatrol = false; // True if a valid patrol route path container has been configured
                                // for this character.
        bool searches = false;  // True if lookaround searching mechanics are enabled for this
                                // specific behavioral archetype.
        bool searchTimeUp =
            false; // True if the active search timer interval has fully ticked down and expired.
        float stateElapsed =
            0.0F; // Total continuous duration in seconds spent within the active state loop.
        float targetLostElapsed = 0.0F; // Total continuous duration in seconds since a living
                                        // target was last verified visually.
    };

    // Aggregates component registers and telemetry data to compile a unified facts profile for an
    // NPC. Reads spatial grids, physical actor states, persistent cognitive brain variables, and
    // transient sensory fields to compute a static boolean matrix guiding finite state machine
    // evaluations.
    NpcFacts gatherNpcFacts(
        const TileMap& map,
        const Actor& actor,
        const NpcBrain& brain,
        const NpcPerception& perception,
        const Actor* target,
        float stateElapsed);
}
