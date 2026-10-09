#pragma once

#include <optional>

#include "simple_platformer/actor/actor_id.hpp"
#include "simple_platformer/combat/combat.hpp"
#include "simple_platformer/input/input_state.hpp"
#include "simple_platformer/inventory/inventory.hpp"
#include "simple_platformer/movement/flying_movement.hpp"
#include "simple_platformer/movement/platformer_movement.hpp"
#include "simple_platformer/movement/surface_climb.hpp"
#include "simple_platformer/navigation/path_follower.hpp"
#include "simple_platformer/npc/npc.hpp"
#include "simple_platformer/physics/body.hpp"
#include "simple_platformer/render/animation.hpp"
#include "simple_platformer/render/sprite.hpp"

namespace simple_platformer
{
    /// @brief Hit point data for an actor.
    struct Health
    {
        /// Current hit points. When this reaches zero the actor is typically considered dead.
        int current = 1;

        /// Maximum hit points the actor can have.
        int maximum = 1;
    };

    /// @brief High-level life state of an actor.
    enum class LifeState
    {
        /// The actor is alive and fully active.
        Alive,

        /// The actor has been killed and is playing its death sequence before removal.
        Dying
    };

    /// @brief Aggregate describing a single actor (player, NPC, enemy, etc.) in the game world.
    ///
    /// The actor is a composition of optional components. A component is present only when
    /// the actor needs that behavior, so systems can check for `has_value()` before using it.
    struct Actor
    {
        /// Unique identifier for this actor.
        ActorId id;

        /// Physical body used for position, size, and collision in the physics simulation.
        Body body;

        /// Input intentions for this frame (e.g., move left, jump, attack).
        /// Filled by the player controller or by NPC AI.
        InputIntentions intentions;

        /// Ground-based platformer movement (walking, running, jumping). Absent for actors without
        /// it.
        std::optional<PlatformerMovement> platformerMovement;

        /// Flight movement behavior. Absent for actors that do not fly.
        std::optional<FlyingMovement> flyingMovement;

        /// Ability to climb surfaces such as ladders or walls. Absent for actors that cannot climb.
        std::optional<SurfaceClimb> surfaceClimb;

        /// Direction the actor is currently facing. Defaults to right.
        Facing facing = Facing::Right;

        /// Current life state (alive or dying). Defaults to alive.
        LifeState life = LifeState::Alive;

        /// Seconds remaining in the death sequence. Only meaningful while `life` is `Dying`.
        float deathTimeRemaining = 0.0F;

        /// Timestamp, in seconds, of the most recent damage taken. Unset if the actor has never
        /// been damaged.
        std::optional<double> lastDamageTimeSeconds;

        /// Visual sprite used to render the actor. Absent for non-visual actors.
        std::optional<Sprite> sprite;

        /// Animation controller that drives the sprite's frames. Absent if the actor is not
        /// animated.
        std::optional<Animator> animator;

        /// Screen visibility from 0 to 1, eased by cover fading. The player uses it for
        /// shading; other actors use it for opacity. Unset until first presented.
        std::optional<float> screenVisibility;

        /// Hit points for the actor. Absent for invulnerable actors.
        std::optional<Health> health;

        /// Items carried by the actor. Absent for actors that cannot hold items.
        std::optional<Inventory> inventory;

        /// Faction or side this actor belongs to, used to determine who can damage whom.
        /// Defaults to neutral.
        Team team = Team::Neutral;

        /// Ranged attack capability (e.g., a projectile weapon). Absent if the actor has no ranged
        /// attack.
        std::optional<RangedWeapon> rangedWeapon;

        /// Close-range biting attack. Absent if the actor cannot bite.
        std::optional<BiteAttack> bite;

        /// Damage dealt to other actors on physical contact. Absent if the actor does no contact
        /// damage.
        std::optional<ContactDamage> contactDamage;

        /// Decision-making logic for NPCs (state machine or behavior controller). Absent for
        /// player-controlled or purely static actors.
        std::optional<NpcBrain> brain;

        /// Sensing data used to detect the world, such as visible targets. Absent for actors that
        /// do not perceive.
        std::optional<NpcPerception> perception;

        /// Sensory configuration (e.g., range and field of view) that the perception system uses.
        /// Absent if unused.
        std::optional<NpcSenses> senses;

        /// Patrol route the actor follows when idle or unaware. Absent for actors that do not
        /// patrol.
        std::optional<Patrol> patrol;

        /// Navigation component that moves the actor along a computed path. Absent for actors that
        /// do not follow paths.
        std::optional<PathFollower> pathFollower;
    };
}
