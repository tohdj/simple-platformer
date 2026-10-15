/**
 * @file combat.hpp
 * @brief Component declarations, attack lifecycles, and tracking structures for weapon and contact
 * combat.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#pragma once

#include <optional>
#include <vector>

#include <glm/vec2.hpp>

#include "simple_platformer/actor/actor_id.hpp"
#include "simple_platformer/math/aabb.hpp"
#include "simple_platformer/render/sprite.hpp"

namespace simple_platformer
{
    // Affiliation categories used to classify actors for targeting and alignment rules.
    enum class Team
    {
        Neutral,
        Player,
        Enemy
    };

    // Evaluates whether two teams are mutually hostile towards each other (e.g., Player vs Enemy).
    bool areOpponents(Team first, Team second);

    // Operational states mapping the lifecycle stages of a ranged weapon attack.
    enum class RangedPhase
    {
        Ready,   // Weapon is available to fire.
        Shoot,   // Weapon is actively deploying the shot; input might be locked.
        Recovery // Cooldown phase before returning to a ready state.
    };

    // Component defining attributes and tracking execution phases for projectile combat.
    struct RangedWeapon
    {
        int damage = 1; // Base health deductions applied to target entities on impact.
        glm::vec2 projectileSize = {
            4.0F,
            2.0F}; // Structural boundaries of the shot's physics box measured in world pixels.
        float projectileSpeed =
            180.0F; // Velocity scale applied along the projectile's movement axis.
        float projectileLifetime =
            2.0F; // Maximum duration in seconds a projectile can persist without colliding.
        float shootDuration =
            0.15F; // Duration in seconds spent processing the firing/shooting logic frame steps.
        float recoveryDuration =
            0.20F;                // Duration in seconds needed to cool down before firing again.
        bool breaksTiles = false; // Determines if impacts from these shots can fragment
                                  // destructible environmental tiles.
        RangedPhase phase =
            RangedPhase::Ready; // Current stage context of the weapon's execution cycle.
        float phaseTimeRemaining =
            0.0F; // Continuous time remaining in seconds for the active operational phase block.
        std::optional<double> lastFiredTimeSeconds; // Timestamp index of the last firing sequence
                                                    // used to coordinate visual effects.
        Sprite projectileSprite = {
            0,
            {{0.0F, 0.0F}, {4.0F, 2.0F}}}; // Visual rendering component detailing how the shot
                                           // appears independently of its physics body size.
    };

    // Operational states detailing the lifecycle stages of a close-quarters strike.
    enum class BitePhase
    {
        Ready,   // Attack is off cooldown and ready to trigger.
        Windup,  // Pre-attack anticipation frames where the hit box is not yet hot.
        Active,  // Hit box is active in the world and checking spatial intersections.
        Recovery // Post-attack lag frames where the actor is recovering.
    };

    // Component managing properties and tracking variables for close-quarters physical attacks.
    struct BiteAttack
    {
        int damage = 1; // Health deductions applied to overlapped targets.
        glm::vec2 hitboxSize = {
            10.0F,
            8.0F}; // Dimensions of the damage contact frame in world pixels.
        float reach =
            4.0F; // Horizontal offset distance ahead of the actor's edge where the box is placed.
        float windupDuration =
            0.12F; // Anticipation duration in seconds before damage checks activate.
        float activeDuration = 0.08F; // Active collision evaluation lifetime in seconds.
        float recoveryDuration =
            0.30F; // Recovery duration in seconds required before returning to the ready loop.
        BitePhase phase =
            BitePhase::Ready; // Current stage context of the strike's attack tracking.
        float phaseTimeRemaining =
            0.0F; // Continuous time remaining in seconds for the active execution segment.
        std::vector<ActorId> actorsHit; // Tracks entity identifiers already penalized during the
                                        // active frame window to prevent double hits.
    };

    // Component managing contact-based environmental damage, such as touching hazardous spikes or
    // spikes on NPCs. Operates continuously and independently of structural movement or targeted
    // player commands.
    struct ContactDamage
    {
        int damage = 1; // Health deductions applied to overlapping opponent structures.
        bool active =
            false; // Flag enabling or disabling the continuous contact checking frame step.
        std::vector<ActorId> actorsHit; // Tracks entity identifiers already penalized during the
                                        // active overlap to prevent multi-hit frames.
    };

    // An active entity payload simulating a moving projectile inside the world simulation grid.
    struct Projectile
    {
        Aabb bounds; // Spatial boundaries and world coordinates tracking the projectile.
        glm::vec2 velocity = {
            0.0F,
            0.0F};      // Spatial tracking speed vector managing movement steps per second.
        int damage = 1; // Damage payload carried by the projectile.
        float lifetimeRemaining =
            1.0F; // Remaining active operational time in seconds before self-termination.
        std::optional<ActorId>
            owner; // The specific identifier of the actor that deployed the shot.
        Team team =
            Team::Neutral; // Team alignment configuration used to evaluate friendly-fire criteria.
        Sprite sprite;     // Visual sprite block handling drawing properties.
        bool breaksTiles = false; // Snapshot rule cloned at launch to preserve tile-breaking
                                  // behaviors if the launcher changes states.
    };

    // Termination triggers causing a projectile to detonate or expire.
    enum class ProjectileBurstCause
    {
        Impact,         // Terminated by contacting structural walls, tiles, or actor bodies.
        LifetimeExpired // Terminated because its internal update life clock hit zero.
    };

    // Transient visual or structural particle layout spawned when a projectile expires or lands.
    struct ProjectileBurst
    {
        ProjectileBurstCause cause =
            ProjectileBurstCause::Impact; // Reason trigger classification for spawning the effect.
        glm::vec2 center = {
            0.0F,
            0.0F}; // Anchor focal midpoint coordinate vector for the particle display.
        glm::vec2 direction = {1.0F, 0.0F}; // Alignment trajectory vector mapping particle
                                            // distribution scaling orientations.
        Sprite sprite;                      // Rendering description mapping textures to draw.
        float duration = 0.1F; // Complete active presentation life windows intended for the effect.
        float lifetimeRemaining = 0.1F; // Time remaining in seconds before removing the component
                                        // structure from view loops.
    };
}
