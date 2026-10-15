/**
 * @file npc_senses.hpp
 * @brief Sensory update systems, target life tracking, and ground run validation for NPC
 * characters.
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
    struct Aabb;
    struct Actor;
    struct NpcBrain;

    // Locates the target actor tracked by the brain, returning nullptr if they are dead, missing,
    // or invalid.
    const Actor* livingTarget(const World& world, const NpcBrain& brain);

    // Evaluates whether two bounding boxes share an uninterrupted, level horizontal ground surface
    // row path. Checks shared floor row alignments and verifies the observer can stand inside all
    // intermediate cells. Grounded states are independent of this structural cell test and must be
    // checked separately.
    bool onSameGroundRun(const TileMap& map, const Aabb& observer, const Aabb& target);

    // Refreshes perception snapshots, line-of-sight tracking raycasts, and acoustic noise tracking
    // for all active NPCs.
    void updateNpcSenses(const TileMap& map, World& world, float deltaTime);
}
