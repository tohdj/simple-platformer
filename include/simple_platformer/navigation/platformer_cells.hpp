/**
 * @file route_occupancy.hpp
 * @brief Spatial occupancy checks and geometric bounds alignment routines for pathfinding nodes.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#pragma once

#include <glm/vec2.hpp>

#include "simple_platformer/math/aabb.hpp"
#include "simple_platformer/math/coordinates.hpp"
#include "simple_platformer/movement/surface_climb.hpp"
#include "simple_platformer/navigation/route.hpp"

namespace simple_platformer
{
    class TileMap;

    /**
     * @brief Evaluates whether an actor can stably stand within a designated grid cell.
     *
     * Verifies that there is adequate vertical clearance for the actor's height profile
     * and solid physical ground support immediately underneath the cell baseline.
     *
     * @param map The static environment geometry grid layout context.
     * @param cell The target grid coordinate cell to inspect.
     * @param bodySize Bounding physics footprint layout dimensions (width and height).
     * @return true if the actor has enough clearance and grounding support, false otherwise.
     */
    bool canStandAt(const TileMap& map, Cell cell, glm::vec2 bodySize);

    /**
     * @brief Evaluates whether an actor can attach to any climbable geometry inside a cell.
     *
     * Checks if the actor's dimensions overlap at least one valid climbable tile asset
     * acting as a vertical wall or ceiling anchor interface.
     *
     * @param map The static environment geometry grid layout context.
     * @param cell The target grid coordinate cell to inspect.
     * @param bodySize Bounding physics footprint layout dimensions (width and height).
     * @return true if the actor fits against a climbable surface, false otherwise.
     */
    bool canClimbAt(const TileMap& map, Cell cell, glm::vec2 bodySize);

    /**
     * @brief Computes the absolute spatial axis-aligned bounding box (AABB) for an actor at a
     * specific surface.
     *
     * Calculates the resting physical matrix bounding edges based on whether the entity is standing
     * on the floor, flush against a left/right wall, or suspended underneath the ceiling of the
     * cell.
     *
     * @param tileSize The length of one square tile cell in world pixels.
     * @param location The structured route node specifying cell coordinates and target anchoring
     * plane.
     * @param bodySize Bounding physics footprint layout dimensions (width and height).
     * @return Aabb The resolved physical bounding box coordinates in world space.
     */
    Aabb boundsAtSurface(int tileSize, RouteLocation location, glm::vec2 bodySize);

    /**
     * @brief Validates if an actor can successfully occupy a target route position point.
     *
     * Asserts that the destination node is within map bounds, verifies the actor's bounding
     * dimensions fit without intersecting solid obstructions, and confirms that adequate supporting
     * anchor elements are present (solid floor blocks below, or climbable tiles along the active
     * bounding walls/ceilings).
     *
     * @param map The static environment geometry grid layout context.
     * @param location The structured route node specifying cell coordinates and target anchoring
     * plane.
     * @param bodySize Bounding physics footprint layout dimensions (width and height).
     * @return true if the node is valid and can structurally support the actor's mass profile,
     * false otherwise.
     */
    bool canOccupy(const TileMap& map, RouteLocation location, glm::vec2 bodySize);
}
