/**
 * @file level_validation.hpp
 * @brief Spatial integrity validation and spawn point clearance verifications for level actor
 * configurations.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#pragma once

namespace simple_platformer
{
    class TileMap;
    class World;

    /**
     * @brief Audits all configured actor spawn coordinates, checkpoints, and patrol pathways
     * against map geometry.
     *
     * Asserts that every character layout structure fits safely within open air without clipping
     * solid grid intersections. Platformer-based actors are subjected to extra checks confirming
     * their starting location blocks and patrol endpoints are properly supported by ground blocks
     * underneath, unless they possess climbing configurations.
     *
     * @param map The static environment geometry grid layout context used to evaluate blocking
     * elements.
     * @param world The target world simulation data provider carrying level actor configurations.
     * @param level The index identifying the specific level context currently undergoing validation
     * queries.
     * @throws std::runtime_error or similar asset failure exception if an unviable spawn or path
     * configuration is discovered.
     */
    void validateLevelActors(const TileMap& map, const World& world, int level);
}
