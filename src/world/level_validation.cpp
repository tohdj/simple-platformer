/**
 * @file level_validation.cpp
 * @brief Spatial integrity validation and spawn point clearance verifications for level actor
 * configurations.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#include "simple_platformer/world/level_validation.hpp"

#include <stdexcept>
#include <string>
#include <string_view>

#include <glm/vec2.hpp>

#include "simple_platformer/actor/actor.hpp"
#include "simple_platformer/actor/actor_id.hpp"
#include "simple_platformer/math/aabb.hpp"
#include "simple_platformer/math/coordinates.hpp"
#include "simple_platformer/movement/surface_climb.hpp"
#include "simple_platformer/npc/npc.hpp"
#include "simple_platformer/world/tile_map.hpp"
#include "simple_platformer/world/world.hpp"

namespace simple_platformer
{
    namespace
    {
        // Scans all grid cells overlapping an Axis-Aligned Bounding Box (AABB)
        // to verify that the bounding region is free from solid, movement-blocking tiles.
        bool hasClearance(const TileMap& map, const Aabb& bounds)
        {
            const CellRange cells = cellsCovered(map.tileSize(), bounds);
            for (int row = cells.first.y; row <= cells.last.y; ++row)
            {
                for (int column = cells.first.x; column <= cells.last.x; ++column)
                {
                    if (map.blocksMovement({column, row}))
                    {
                        return false; // Collision detected with environmental tile
                    }
                }
            }
            return true; // No solid geometry overlaps the region
        }

        // Checks the horizontal row of grid tiles immediately beneath the bottom edge
        // of a bounding region to ensure that an entity has solid standing ground.
        bool hasGroundSupport(const TileMap& map, const Aabb& bounds)
        {
            const CellRange cells = cellsCovered(map.tileSize(), bounds);
            // Project down slightly past the bottom boundary edge using an tolerance offset
            const int rowBelow =
                cellAt(map.tileSize(), {bounds.topLeft.x, bottomOf(bounds) + EdgeTolerance}).y;

            for (int column = cells.first.x; column <= cells.last.x; ++column)
            {
                if (map.blocksMovement({column, rowBelow}))
                {
                    return true; // Found stable standing floor geometry
                }
            }
            return false; // No supporting blocks are underneath the entity bounds
        }

        // Formats a diagnostic string mapping out level information and actor IDs for exception
        // tracking logs
        std::string actorLocation(int level, ActorId actor, std::string_view place)
        {
            return "Level " + std::to_string(level) + " actor " + std::to_string(actor.value) +
                   " " + std::string(place);
        }

        // Verifies actor positioning conditions, throwing an argument exception if bounding
        // space overlaps solid blocks or if gravity-dependent entities lack ground support.
        void validatePlacement(
            const TileMap& map,
            const Actor& actor,
            const Aabb& bounds,
            int level,
            std::string_view place,
            bool needsGround)
        {
            const std::string location = actorLocation(level, actor.id, place);
            if (!hasClearance(map, bounds))
            {
                throw std::invalid_argument(location + " overlaps a blocked tile");
            }
            if (needsGround && !hasGroundSupport(map, bounds))
            {
                throw std::invalid_argument(location + " has no ground support");
            }
        }

        // Simulates a standing bounding box using target base-foot coordinates,
        // passing the derived boundaries along for structural placement analysis.
        void validateAtFeet(
            const TileMap& map,
            const Actor& actor,
            glm::vec2 feet,
            int level,
            std::string_view place,
            bool needsGround)
        {
            const Aabb bounds = boxStandingOn(feet, actor.body.bounds.size);
            validatePlacement(map, actor, bounds, level, place, needsGround);
        }

        // Determines whether an AI actor requires floor geometry to navigate.
        // A climber can patrol to a wall or ceiling; navigation takes it to the nearest
        // place it can hold.
        bool patrolNeedsGround(const Actor& actor)
        {
            return actor.platformerMovement.has_value() && !actor.surfaceClimb.has_value();
        }
    }

    // Evaluates the initial positions and AI navigation bounds of all level actors
    // before runtime execution to ensure no map geometry conflicts exist.
    void validateLevelActors(const TileMap& map, const World& world, int level)
    {
        // 1. Process validation checking paths for all configured level actors
        for (const Actor& actor : world.actors())
        {
            const bool platformer = actor.platformerMovement.has_value();
            // Assert that the initial starting spawn bounding box fits safely within the level
            // geometry
            validatePlacement(map, actor, actor.body.bounds, level, "spawn", platformer);

            if (!actor.patrol.has_value())
            {
                continue; // Skip waypoint checking if this actor does not patrol
            }

            // Assert that both AI destination travel endpoints rest on safe, valid coordinate nodes
            const bool needsGround = patrolNeedsGround(actor);
            validateAtFeet(
                map, actor, actor.patrol->firstFeet, level, "first patrol point", needsGround);
            validateAtFeet(
                map, actor, actor.patrol->secondFeet, level, "second patrol point", needsGround);
        }

        // 2. Locate the primary controlled hero entity to verify its global recovery point
        const Actor* player = world.findActor(world.playerId());
        if (player != nullptr)
        {
            // Assert that the default player spawn anchor configuration provides safe, unblocked
            // standing space
            validateAtFeet(
                map,
                *player,
                world.playerSpawnFeet(),
                level,
                "respawn",
                player->platformerMovement.has_value());
        }
    }
}
