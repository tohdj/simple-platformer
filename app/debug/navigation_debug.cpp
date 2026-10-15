/**
 * @file        navigation_debug.cpp
 * @brief       Declares data structures and diagnostic sampling functions for
 *              extracting pathfinding grids, trajectory arcs, and cell connectivity.
 *
 * @author      Mohd Fadzuli
 * @author      Adapted by: Toh Da Jun
 * @copyright   Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#include "navigation_debug.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include <glm/vec2.hpp>

#include "simple_platformer/actor/actor.hpp"
#include "simple_platformer/math/aabb.hpp"
#include "simple_platformer/math/coordinates.hpp"
#include "simple_platformer/movement/platformer_movement.hpp"
#include "simple_platformer/movement/surface_climb.hpp"
#include "simple_platformer/navigation/actor_navigation.hpp"
#include "simple_platformer/input/input_program.hpp"
#include "simple_platformer/navigation/route.hpp"
#include "simple_platformer/navigation/platformer_cells.hpp"
#include "simple_platformer/navigation/platformer_traversal_profile.hpp"
#include "simple_platformer/navigation/platformer_connection_table.hpp"
#include "simple_platformer/navigation/traversal.hpp"
#include "simple_platformer/physics/body.hpp"
#include "simple_platformer/world/tile_map.hpp"
#include "simple_platformer/world/world.hpp"

namespace simple_platformer
{
    namespace
    {
        // Converts a discrete tile cell coordinate (Column, Row) into a screen-space Axis-Aligned
        // Bounding Box (AABB) configuration
        Aabb cellBounds(int tileSize, Cell cell)
        {
            const auto size = static_cast<float>(tileSize);
            return {// Compute absolute world position offsets for the top-left structural corner of
                    // the cell
                    {static_cast<float>(cell.x) * size, static_cast<float>(cell.y) * size},
                    // Set uniform square dimensional scaling constraints based on map sizes
                    {size, size}};
        }

        // Unpacks routing graph relationships to extract detailed cell telemetry profiles right
        // under the mouse cursor crosshairs
        CursorCellDebugInfo cursorCellDebugInfo(
            const TileMap& map,
            const PlatformerConnectionTable& table,
            const PlatformerTraversalProfile& profile,
            Cell cell)
        {
            const int tileSize = map.tileSize();
            CursorCellDebugInfo info;
            info.bounds = cellBounds(
                tileSize, cell); // Extract bounding bounds for the fundamental cell frame

            // Drop out early with standard box configurations if graph linkages haven't finished
            // building
            if (!table.isBuilt(profile))
            {
                return info;
            }

            // --- STEP 1: COMPUTE COMBINED ACTOR COLLISION FOOTPRINT OVER MULTIPLE TILES ---
            const CellRange& footprint = table.footprint(cell, profile);
            const Aabb first = cellBounds(tileSize, footprint.first);
            const Aabb last = cellBounds(tileSize, footprint.last);

            // Combine the top-left point of the first cell with the extreme corner edges of the
            // final cell to form an aggregated footprint block
            info.footprint = Aabb{first.topLeft, last.topLeft + last.size - first.topLeft};

            // --- STEP 2: LOOP AND MAP EVERY OUTGOING ARCHITECTURAL CONTEXT LINK ---
            for (const RouteConnection& connection : table.connections(cell, profile))
            {
                // Translate the source surface plane configuration parameters into absolute anchor
                // positions
                const glm::vec2 fromFeet = feetOf(
                    boundsAtSurface(tileSize, {cell, connection.sourceSurface}, profile.size));

                // Package structural travel data variables (Origin node, destination node, travel
                // cost, and kinematic arcs)
                info.connections.push_back(
                    {fromFeet,
                     feetOf(boundsAtSurface(tileSize, connection.step.destination, profile.size)),
                     connection.step.traversal,
                     connection.cost,
                     // Run trajectory delta tick loops to build parabolic predictive paths loops
                     // for jumps/falls
                     sampleAirborneProgram(
                         map,
                         fromFeet,
                         profile.size,
                         profile.movement,
                         connection.step.traversal,
                         connection.step.inputs,
                         profile.stepSeconds)});
            }
            return info;
        }
    }

    // Simulates an entire sequential input program sequence over discrete ticks to chart a jump or
    // fall trajectory path
    std::vector<glm::vec2> sampleAirborneProgram(
        const TileMap& map,
        glm::vec2 startFeet,
        glm::vec2 bodySize,
        const PlatformerMovementConfig& movement,
        Traversal traversal,
        const InputProgram& inputs,
        float stepSeconds)
    {
        // Trajectory generation is only valid for vertical airborne states containing active inputs
        if ((traversal != Traversal::Jump && traversal != Traversal::Fall) || inputs.empty())
        {
            return {};
        }

        // Setup a temporary hardware physics box shell configuration matching the starting anchor
        // coordinate location
        Body body;
        body.bounds = boxStandingOn(startFeet, bodySize);
        // Force state parameters to reflect mid-air physics overrides (e.g., bypass initial wall
        // attachments)
        PlatformerMovement flight{movement, true, 0.0F, 0.0F};

        std::vector<glm::vec2> sampledFeet;
        sampledFeet.push_back(
            feetOf(body.bounds)); // Record the baseline initial position coordinate step

        // 1. Unpack chronological keyboard/movement macro execution steps loops
        for (const InputStep& input : inputs)
        {
            // Convert macro time durations down into uniform integer time step slices
            const long ticks = std::lround(input.duration / stepSeconds);

            // 2. Iterate kinematic position delta state ticks step-by-step
            for (long tick = 0; tick < ticks; ++tick)
            {
                // Process physical map collision limits and alter velocity transformations safely
                updatePlatformerMovement(map, body, flight, input.intentions, stepSeconds);
                // Record the updated step positions frame bounds inside our track path matrix list
                // array
                sampledFeet.push_back(feetOf(body.bounds));
            }
        }
        return sampledFeet;
    }

    // Unpacks runtime navigation networks states and filters active graph metadata details within
    // visibility view limits
    std::optional<NavigationConnectionsDebugInfo> makeNavigationConnectionsDebugInfo(
        const World& world,
        const TileMap& map,
        float simulationStepSeconds,
        const NavigationDebugView& view,
        std::optional<Aabb> visibleBounds)
    {
        std::vector<PlatformerTraversalProfile> profiles;

        // --- PHASE 1: ASSEMBLE A UNIQUE SET OF ACTOR PHYSICS TRAVERSAL PROFILES ---
        for (const Actor& actor : world.actors())
        {
            if (!actor.pathFollower.has_value() || !actor.platformerMovement.has_value())
            {
                continue; // Skip static elements or objects lacking graph-based navigation traits
            }

            const PlatformerTraversalProfile candidate =
                platformerTraversalProfileFor(actor, simulationStepSeconds);

            // Check if a configuration matching these size and speed attributes is already cached
            const bool known = std::any_of(
                profiles.begin(),
                profiles.end(),
                [&candidate](const PlatformerTraversalProfile& profile)
                { return profile == candidate; });

            if (!known)
            {
                profiles.push_back(candidate); // Append unique search characteristics sets
            }
        }

        // Terminate tracking updates safely if no navigation actors reside inside the active
        // physics layer
        if (profiles.empty())
        {
            return std::nullopt;
        }

        // --- PHASE 2: EXTRACT STRUCTURAL PROPERTIES FROM THE HIGHLIGHTED PROFILE INDEX ---
        // Cycle securely using modulus limits (Triggered via user pressing N keys inside interface
        // managers)
        const std::size_t shown = view.profileIndex % profiles.size();
        const PlatformerTraversalProfile& profile = profiles[shown];

        NavigationConnectionsDebugInfo info;
        info.bodySize = profile.size;

        // Look up readable definition names mapping onto the highlighted dimensions configurations
        for (const NamedNavigationProfile& named : view.namedProfiles)
        {
            if (named.profile == profile)
            {
                info.actorName = named.name;
                break;
            }
        }
        info.profileIndex = shown;
        info.profileCount = profiles.size();

        const PlatformerConnectionTable& table = world.platformerConnections();
        const bool built =
            table.isBuilt(profile); // Track if backend node graph calculation structures completed

        // --- PHASE 3: GRID SCAN OPERATIONS - Extract nodes telemetry matching viewport constraints
        // ---
        for (int row = 0; row < map.height(); ++row)
        {
            for (int column = 0; column < map.width(); ++column)
            {
                const Cell cell{column, row};
                const bool standable = canStandAt(map, cell, profile.size);

                // Discard empty/air grid blocks entirely if the actor cannot walk or climb through
                // them
                if (!standable &&
                    !(profile.climb.has_value() && canClimbAt(map, cell, profile.size)))
                {
                    continue;
                }

                const Aabb bounds = cellBounds(map.tileSize(), cell);
                // Perform camera frustum bounding culling tests to discard offscreen telemetry
                // generation overhead
                if (visibleBounds.has_value() && !overlaps(bounds, *visibleBounds))
                {
                    continue;
                }

                // If the tracking link graph is still compilation-locked, append basic cell box
                // metrics placeholders
                if (!built)
                {
                    info.cells.push_back({bounds, std::nullopt, standable});
                    continue;
                }

                // Extract structural outgoing link counts relative to current profile dimensions
                // parameters
                const std::size_t connections = table.connections(cell, profile).size();
                info.cells.push_back({bounds, connections, standable});
            }
        }

        // --- PHASE 4: EXTRACT LOCAL TELEMETRY BLOCKS UNDER CROSSHAIR SELECTIONS ---
        if (view.cursorWorld.has_value())
        {
            const glm::vec2 cursor = view.cursorWorld.value_or(glm::vec2{0.0F, 0.0F});
            // Confirm user coordinate selection vectors reside completely inside layout map borders
            if (cursor.x >= 0.0F && cursor.y >= 0.0F && cursor.x < map.pixelWidth() &&
                cursor.y < map.pixelHeight())
            {
                // Query absolute connection maps paths details located directly right under
                // pointing device coordinates
                info.cursorCell =
                    cursorCellDebugInfo(map, table, profile, cellAt(map.tileSize(), cursor));
            }
        }
        return info;
    }
}
