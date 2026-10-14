#include "npc_names.hpp"

#include "simple_platformer/npc/npc.hpp"

namespace simple_platformer
{
    // Translates an abstract finite-state-machine state code into a readable debug display string
    const char* nameOf(NpcState state)
    {
        switch (state)
        {
        case NpcState::Flee:
            return "Flee"; // Evading threat vectors or breaking line of sight
        case NpcState::Sleep:
            return "Sleep"; // Dormant phase; unresponsive until noise or sight alerts wake it
        case NpcState::Charge:
            return "Charge"; // High-velocity direct movement target assault frame pass
        case NpcState::Stunned:
            return "Stunned"; // Action lockout state; temporarily immobilized by damage effects
        case NpcState::Idle:
            return "Idle"; // Standard static rest configuration loop
        case NpcState::Patrol:
            return "Patrol"; // Actively traversing predetermined path nodes layout tracking arrays
        case NpcState::Chase:
            return "Chase"; // Dynamically pathfinding directly towards a moving player target
                            // bounding box
        case NpcState::Bite:
            return "Bite"; // Executing immediate close-quarters physics boundary contact attacks
        case NpcState::Shoot:
            return "Shoot"; // Dispatching projectile entities along target vision calculations
                            // paths
        case NpcState::Search:
            return "Search"; // Inspecting the vicinity around a player's last known tracking
                             // coordinate map location
        case NpcState::Retreat:
            return "Retreat"; // Tactical structural repositioning down towards defense spawn
                              // anchors
        case NpcState::Watch:
            return "Watch"; // Stationary alert scanning coverage sweeps phase
        }

        return "Unknown"; // Fallback catch for unregistered state tracking parameters entries
    }

    // Maps high-level behavioral personality profiles down into readable debugging string names
    const char* nameOf(NpcTactic tactic)
    {
        switch (tactic)
        {
        case NpcTactic::Coward:
            return "Coward"; // Prioritizes escape patterns and disengages aggressively upon taking
                             // damage
        case NpcTactic::Charger:
            return "Charger"; // Commits to straight-line physics path acceleration attacks on sight
        case NpcTactic::Pursuer:
            return "Pursuer"; // Relentlessly updates active pathfinders nodes to follow player
                              // tracking coordinates
        case NpcTactic::KeepDistance:
            return "KeepDistance"; // Attempts to maintain an ideal target grid circle perimeter gap
                                   // for long-range combat
        }

        return "Unknown"; // Fallback choice if non-catalog structural settings pass checks
    }
}
