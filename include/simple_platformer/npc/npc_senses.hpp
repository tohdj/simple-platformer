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

    /**
     * @brief Locates the target actor tracked by the brain, ensuring they are alive.
     *
     * @param world The active world simulation data provider carrying all entities.
     * @param brain Cognitive state containing target memory keys.
     * @return const Actor* Pointer to the living target actor, or nullptr if the target is dead or
     * missing.
     */
    const Actor* livingTarget(const World& world, const NpcBrain& brain);

    /**
     * @brief Evaluates whether two bounding boxes share an uninterrupted, level horizontal ground
     * surface.
     *
     * Checks if both physical feet alignments occupy the same horizontal row index, and verifies
     * that the observer's body dimensions can stand safely in every intermediate grid cell without
     * clipping.
     *
     * @note Grounded states are independent of this test and must be validated separately.
     * @param map The static environment geometry grid layout context.
     * @param observer Bounding box metrics of the sensing actor.
     * @param target Bounding box metrics of the observed target actor.
     * @return true if an unobstructed ground-level run links both bodies, false otherwise.
     */
    bool onSameGroundRun(const TileMap& map, const Aabb& observer, const Aabb& target);

    /**
     * @brief Refreshes perception snapshots, line-of-sight tracking, and acoustic listeners for all
     * active NPCs.
     *
     * @param map The static environment geometry grid layout context used for sight casts.
     * @param world The game world containing the collections of active actors and transient noise
     * events.
     * @param deltaTime The discrete frame simulation duration step length in seconds.
     */
    void updateNpcSenses(const TileMap& map, World& world, float deltaTime);
}
