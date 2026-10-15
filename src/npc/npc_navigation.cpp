/**
 * @file npc_navigation.cpp
 * @brief Utility for calculating input steering macros and route changes to guide NPCs toward
 * target goals.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#include "simple_platformer/npc/npc_navigation.hpp"

#include <optional>
#include <utility>

#include <glm/geometric.hpp>
#include <glm/vec2.hpp>

#include "simple_platformer/actor/actor.hpp"
#include "simple_platformer/input/input_state.hpp"
#include "simple_platformer/math/aabb.hpp"
#include "simple_platformer/navigation/actor_navigation.hpp"
#include "simple_platformer/navigation/navigation_path.hpp"
#include "simple_platformer/navigation/path_follower.hpp"
#include "simple_platformer/npc/npc_update.hpp"
#include "simple_platformer/world/tile_map.hpp"
#include "simple_platformer/world/world.hpp"

namespace simple_platformer
{
    namespace
    {
        // In pixels: keep the path until the goal moves farther than this, so a drifting
        // target does not trigger a search every update.
        constexpr float ReplanDistance = 8.0F;

        // Needs a path if none is stored, the planned goal has moved, or the actor has
        // been displaced from the end of a finished path.
        bool needsPath(const PathFollower& follower, glm::vec2 feet, glm::vec2 goal)
        {
            // Trigger an immediate recalculation if the internal routing caches are uninitialized
            if (!follower.path.has_value() || !follower.goal.has_value())
            {
                return true;
            }

            // Replan if the destination endpoint shifts past the tracking distance threshold margin
            if (glm::distance(*follower.goal, goal) > ReplanDistance)
            {
                return true;
            }

            // Replan if the agent has reached the end of its path but remains far from the actual
            // goal
            return pathComplete(follower) &&
                   glm::distance(feet, endOf(*follower.path)) > ReplanDistance;
        }

        // Whether a tile has broken since the path was planned. The path may run through
        // it, so the follower plans again at once, whatever the goal.
        bool plannedBeforeABreak(const TileMap& map, const PathFollower& follower)
        {
            // Compare historical graph markers to detect environmental changes or broken obstacles
            return follower.breaksWhenPlanned != map.brokenCells().size();
        }

        // Keeps a usable path or searches again after a goal change, displacement, or
        // tile break. New connections use the same step as the actor's next movement.
        void planPathTo(
            const NpcUpdate& update,
            const Actor& actor,
            PathFollower& follower,
            glm::vec2 goalFeet)
        {
            const TileMap& map = update.map;

            // Skip processing tasks if environmental bounds are intact and current routes remain
            // valid
            if (!plannedBeforeABreak(map, follower) &&
                !needsPath(follower, feetOf(actor.body.bounds), goalFeet))
            {
                return;
            }

            // Dispatch an A* or graph search routine to build a new waypoint list
            std::optional<NavigationPathResult> pathResult = findActorPath(
                map, actor, goalFeet, update.deltaTime, update.world.platformerConnections());
            if (!pathResult.has_value())
            {
                return; // Navigation search is incomplete or throttled this frame
            }
            if (!pathResult->path.has_value())
            {
                clearPath(follower); // Completely flush old path parameters if the target point is
                                     // completely unreachable
            }

            // Update tracking anchors and save current destructible cell change count logs
            follower.goal = goalFeet;
            follower.breaksWhenPlanned = map.brokenCells().size();

            // Bind the newly generated waypoint structures into the navigation controller component
            if (pathResult->path.has_value())
            {
                setPath(follower, std::move(pathResult->path.value()));
            }
        }
    }

    // Processes steering math and maps controller input bits toward a specified target baseline
    // coordinate location
    InputIntentions intentionsToReach(
        const NpcUpdate& update,
        Actor& actor,
        PathFollower& follower,
        glm::vec2 goalFeet)
    {
        // 1. Maintain, clear, or calculate a fresh waypoint track path list based on layout metrics
        planPathTo(update, actor, follower, goalFeet);

        // 2. Routing Bridge: evaluate movement type components to call the correct steering module
        if (actor.flyingMovement.has_value())
        {
            // Calculate multi-directional vector inputs for levitating or flying actors
            return followFlyingPath(
                actor.body.bounds, *actor.flyingMovement, follower, update.deltaTime);
        }
        if (actor.platformerMovement.has_value())
        {
            // Calculate horizontal runs, jumps, and ledge metrics for gravity-bound or climbing
            // entities
            return followPlatformerPath(
                actor.body,
                *actor.platformerMovement,
                follower,
                update.deltaTime,
                actor.surfaceClimb.has_value() ? &*actor.surfaceClimb : nullptr);
        }
        return {}; // Return unmapped, empty control intents if the actor lacks a kinematic
                   // configuration
    }
}
