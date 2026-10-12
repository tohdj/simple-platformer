#pragma once

#include <optional>
#include <vector>

#include <glm/vec2.hpp>

#include "simple_platformer/actor/actor_id.hpp"
#include "simple_platformer/math/aabb.hpp"
#include "simple_platformer/render/sprite.hpp"

namespace simple_platformer
{
    /**
     * @brief Affiliation categories used to classify actors for targeting and alignment rules.
     */
    enum class Team
    {
        Neutral,
        Player,
        Enemy
    };

    /**
     * @brief Evaluates whether two teams are mutually hostile towards each other.
     * @param first The team categorization of the first party.
     * @param second The team categorization of the second party.
     * @return true if they are targeted opponents (e.g., Player vs Enemy), false otherwise.
     */
    bool areOpponents(Team first, Team second);

    /**
     * @brief Operational states mapping the lifecycle stages of a ranged weapon attack.
     */
    enum class RangedPhase
    {
        Ready,   /// Weapon is available to fire.
        Shoot,   /// Weapon is actively deploying the shot; input might be locked.
        Recovery /// Cooldown phase before returning to a ready state.
    };

    /**
     * @brief Component defining attributes and tracking execution phases for projectile combat.
     */
    struct RangedWeapon
    {
        /// Base health deductions applied to target entities on impact.
        int damage = 1;
        /// Structural boundaries of the shot's physics box measured in world pixels.
        glm::vec2 projectileSize = {4.0F, 2.0F};
        /// Velocity scale applied along the projectile's movement axis.
        float projectileSpeed = 180.0F;
        /// Maximum duration in seconds a projectile can persist without colliding.
        float projectileLifetime = 2.0F;
        /// Duration in seconds spent processing the firing/shooting logic frame steps.
        float shootDuration = 0.15F;
        /// Duration in seconds needed to cool down before firing again.
        float recoveryDuration = 0.20F;
        /// Determines if impacts from these shots can fragment destructible environmental tiles.
        bool breaksTiles = false;

        /// Current stage context of the weapon's execution cycle.
        RangedPhase phase = RangedPhase::Ready;
        /// Continuous time remaining in seconds for the active operational phase block.
        float phaseTimeRemaining = 0.0F;
        /// Timestamp index of the last firing sequence used to coordinate visual effects.
        std::optional<double> lastFiredTimeSeconds;
        /// Visual rendering component detailing how the shot appears independently of its physics
        /// body size.
        Sprite projectileSprite = {0, {{0.0F, 0.0F}, {4.0F, 2.0F}}};
    };

    /**
     * @brief Operational states detailing the lifecycle stages of a close-quarters strike.
     */
    enum class BitePhase
    {
        Ready,   /// Attack is off cooldown and ready to trigger.
        Windup,  /// Pre-attack anticipation frames where the hit box is not yet hot.
        Active,  /// Hit box is active in the world and checking spatial intersections.
        Recovery /// Post-attack lag frames where the actor is recovering.
    };

    /**
     * @brief Component managing properties and tracking variables for close-quarters physical
     * attacks.
     */
    struct BiteAttack
    {
        /// Health deductions applied to overlapped targets.
        int damage = 1;
        /// Dimensions of the damage contact frame in world pixels.
        glm::vec2 hitboxSize = {10.0F, 8.0F};
        /// Horizontal offset distance ahead of the actor's edge where the box is placed.
        float reach = 4.0F;
        /// Anticipation duration in seconds before damage checks activate.
        float windupDuration = 0.12F;
        /// Active collision evaluation lifetime in seconds.
        float activeDuration = 0.08F;
        /// Recovery duration in seconds required before returning to the ready loop.
        float recoveryDuration = 0.30F;

        /// Current stage context of the strike's attack tracking.
        BitePhase phase = BitePhase::Ready;
        /// Continuous time remaining in seconds for the active execution segment.
        float phaseTimeRemaining = 0.0F;
        /// Tracks entity identifiers already penalized during the active frame window to prevent
        /// double hits.
        std::vector<ActorId> actorsHit;
    };

    /**
     * @brief Component managing contact-based environmental damage, such as touching hazardous
     * spikes or spikes on NPCs.
     *
     * Operates continuously and independently of structural movement or targeted player commands.
     */
    struct ContactDamage
    {
        /// Health deductions applied to overlapping opponent structures.
        int damage = 1;
        /// Flag enabling or disabling the continuous contact checking frame step.
        bool active = false;
        /// Tracks entity identifiers already penalized during the active overlap to prevent
        /// multi-hit frames.
        std::vector<ActorId> actorsHit;
    };

    /**
     * @brief An active entity payload simulating a moving projectile inside the world simulation
     * grid.
     */
    struct Projectile
    {
        /// Spatial boundaries and world coordinates tracking the projectile.
        Aabb bounds;
        /// Spatial tracking speed vector managing movement steps per second.
        glm::vec2 velocity = {0.0F, 0.0F};
        /// Damage payload carried by the projectile.
        int damage = 1;
        /// Remaining active operational time in seconds before self-termination.
        float lifetimeRemaining = 1.0F;
        /// The specific identifier of the actor that deployed the shot.
        std::optional<ActorId> owner;
        /// Team alignment configuration used to evaluate friendly-fire criteria.
        Team team = Team::Neutral;
        /// Visual sprite block handling drawing properties.
        Sprite sprite;
        /// Snapshot rule cloned at launch to preserve tile-breaking behaviors if the launcher
        /// changes states.
        bool breaksTiles = false;
    };

    /**
     * @brief Termination triggers causing a projectile to detonate or expire.
     */
    enum class ProjectileBurstCause
    {
        Impact,         /// Terminated by contacting structural walls, tiles, or actor bodies.
        LifetimeExpired /// Terminated because its internal update life clock hit zero.
    };

    /**
     * @brief Transient visual or structural particle layout spawned when a projectile expires or
     * lands.
     */
    struct ProjectileBurst
    {
        /// Reason trigger classification for spawning the effect.
        ProjectileBurstCause cause = ProjectileBurstCause::Impact;
        /// Anchor focal midpoint coordinate vector for the particle display.
        glm::vec2 center = {0.0F, 0.0F};
        /// Alignment trajectory vector mapping particle distribution scaling orientations.
        glm::vec2 direction = {1.0F, 0.0F};
        /// Rendering description mapping textures to draw.
        Sprite sprite;
        /// Complete active presentation life windows intended for the effect.
        float duration = 0.1F;
        /// Time remaining in seconds before removing the component structure from view loops.
        float lifetimeRemaining = 0.1F;
    };
}
