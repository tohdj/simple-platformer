/**
 * @file        actor_validation.cpp
 * @brief       Provides validation signatures ensuring structurally complete and safe
 * initialization parameters for new actors.
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright   Copyright (c) 2026 Simple Platformer. All rights reserved.
 */

#include "simple_platformer/actor/actor_validation.hpp"

#include <stdexcept>

#include "simple_platformer/actor/actor.hpp"
#include "simple_platformer/actor/actor_id.hpp"
#include "simple_platformer/combat/combat.hpp"
#include "simple_platformer/math/validation.hpp"
#include "simple_platformer/movement/surface_climb.hpp"

namespace simple_platformer
{
    namespace
    {
        // Ensures that the uninitialized actor does not possess a pre-assigned entity ID
        void validateIdentity(const Actor& actor)
        {
            if (isValid(actor.id))
            {
                throw std::invalid_argument("World assigns actor IDs");
            }
        }

        // Assures that bounding boxes, positions, and velocity vectors contain finite, positive
        // data
        void validateBody(const Actor& actor)
        {
            if (!isFinite(actor.body.bounds.topLeft) || !isFinite(actor.body.bounds.size) ||
                !isFinite(actor.body.velocity) || actor.body.bounds.size.x <= 0.0F ||
                actor.body.bounds.size.y <= 0.0F)
            {
                throw std::invalid_argument("Actors require finite positive-sized bodies");
            }
        }

        // Verifies movement constraints, ensuring exactly one locomotion component is active
        void validateMovement(const Actor& actor)
        {
            // XOR check: An actor must be exclusively a platformer or a flyer
            if (actor.platformerMovement.has_value() == actor.flyingMovement.has_value())
            {
                throw std::invalid_argument("Actors require exactly one movement component");
            }
            if (actor.flyingMovement.has_value() &&
                (!isFiniteNonNegative(actor.flyingMovement->speed)))
            {
                throw std::invalid_argument(
                    "Flying movement speed must be finite and non-negative");
            }
            // Climbing setups are structural extensions that depend directly on base platformer
            // logic
            if (actor.surfaceClimb.has_value())
            {
                if (!actor.platformerMovement.has_value())
                {
                    throw std::invalid_argument("Surface climbing requires platformer movement");
                }
                validateSurfaceClimbConfig(actor.surfaceClimb->config);
            }
        }

        // Validates visual dependencies, rendering structures, and clip arrays
        void validatePresentation(const Actor& actor)
        {
            if (actor.animator.has_value() && !actor.sprite.has_value())
            {
                throw std::invalid_argument("Animated actors require a sprite");
            }
            if (actor.animator.has_value())
            {
                requireSeconds(actor.animator->elapsed, "Actor animation elapsed");
                if (actor.animator->animationSet.clips.empty())
                {
                    throw std::invalid_argument("Actor animation data is invalid");
                }
            }
        }

        // Enforces combat parameters, cooldown limits, attack exclusivity, and team layouts
        void validateCombat(const Actor& actor)
        {
            // Verify structural parameter sanity fields for ranged configurations
            if (actor.rangedWeapon.has_value())
            {
                const RangedWeapon& weapon = *actor.rangedWeapon;
                requireSeconds(weapon.phaseTimeRemaining, "Ranged weapon phase time remaining");
                if (weapon.damage <= 0 || !isFinitePositive(weapon.projectileSize) ||
                    !isFinitePositive(weapon.projectileSpeed) ||
                    !isFinitePositive(weapon.projectileLifetime) ||
                    !isFinitePositive(weapon.shootDuration) ||
                    !isFinitePositive(weapon.recoveryDuration) ||
                    !isFinitePositive(weapon.projectileSprite.region.size))
                {
                    throw std::invalid_argument("Actor ranged weapon data is invalid");
                }
            }
            // Verify proximity limits and damage scaling fields for melee profiles
            if (actor.bite.has_value())
            {
                const BiteAttack& bite = *actor.bite;
                requireSeconds(bite.phaseTimeRemaining, "Bite phase time remaining");
                if (bite.damage <= 0 || !isFinitePositive(bite.hitboxSize) ||
                    !isFiniteNonNegative(bite.reach) || !isFinitePositive(bite.windupDuration) ||
                    !isFinitePositive(bite.activeDuration) ||
                    !isFinitePositive(bite.recoveryDuration))
                {
                    throw std::invalid_argument("Actor bite data is invalid");
                }
            }
            if (actor.contactDamage.has_value() && actor.contactDamage->damage <= 0)
            {
                throw std::invalid_argument("Actor contact damage must be positive");
            }
            // Ensure entities do not carry more than one active primary weapon capability
            const int attacks = static_cast<int>(actor.rangedWeapon.has_value()) +
                                static_cast<int>(actor.bite.has_value());
            if (attacks > 1)
            {
                throw std::invalid_argument("An actor can have only one primary attack");
            }
            // Aggressive or hazardous actors cannot belong to the passive Neutral team alliance
            if ((attacks > 0 || actor.contactDamage.has_value()) && actor.team == Team::Neutral)
            {
                throw std::invalid_argument("Actors with attacks require a non-neutral team");
            }
            // Confirm health value variables stay bounded cleanly within safe range thresholds
            if (actor.health.has_value() &&
                (actor.health->maximum <= 0 || actor.health->current < 0 ||
                 actor.health->current > actor.health->maximum))
            {
                throw std::invalid_argument("Actor health must be within zero and its maximum");
            }
        }

        // Verifies component combinations and variables for artificial intelligence tracking loops
        void validateNpc(const Actor& actor)
        {
            const bool hasAnyNpcComponent = actor.brain.has_value() || actor.senses.has_value() ||
                                            actor.perception.has_value() ||
                                            actor.patrol.has_value() ||
                                            actor.pathFollower.has_value();
            // Core NPC behaviors depend on a closed loop of all four structural modules working
            // together
            const bool hasRequiredNpcComponents =
                actor.brain.has_value() && actor.perception.has_value() &&
                actor.senses.has_value() && actor.pathFollower.has_value();
            if (hasAnyNpcComponent && !hasRequiredNpcComponents)
            {
                throw std::invalid_argument(
                    "NPC actors require a brain, perception, senses, and path follower");
            }
            // Check state machines and temporal tracking variables within the brain instance
            if (actor.brain.has_value())
            {
                requireSeconds(actor.brain->stateElapsed, "NPC state elapsed");
                requireSeconds(actor.brain->targetLostElapsed, "NPC target lost elapsed");
                if (actor.brain->chargeDirection != -1.0F && actor.brain->chargeDirection != 1.0F)
                {
                    throw std::invalid_argument("NPC charge direction must be left or right");
                }
                requireSeconds(actor.brain->targetMemoryRemaining, "NPC target memory remaining");
                if (!isFinite(actor.brain->lastKnownTargetFeet))
                {
                    throw std::invalid_argument("NPC brain runtime data is invalid");
                }
            }
            // Ensure sensory distance caps and search interval profiles map out cleanly
            if (actor.senses.has_value())
            {
                requireSeconds(actor.senses->targetMemoryDuration, "NPC target memory duration");
                requireSeconds(actor.senses->searchDuration, "NPC search duration");
                if (!isFiniteNonNegative(actor.senses->noticeDistance) ||
                    !isFiniteNonNegative(actor.senses->standoffDistance))
                {
                    throw std::invalid_argument("NPC senses data is invalid");
                }
            }
            if (actor.patrol.has_value() &&
                (!isFinite(actor.patrol->firstFeet) || !isFinite(actor.patrol->secondFeet)))
            {
                throw std::invalid_argument("NPC patrol endpoints must be finite");
            }
        }

        // Confirms that the navigation waypoint tracking module contains valid timers
        void validatePathFollower(const Actor& actor)
        {
            if (actor.pathFollower.has_value())
            {
                requireSeconds(actor.pathFollower->programElapsed, "NPC path program elapsed");
            }
        }
    }

    // Runs an exhaustive suite of structural validity checks on an actor before registering it in
    // the world
    void validateActor(const Actor& actor)
    {
        // Assures the actor ID is unassigned and reserved for the factory setup
        validateIdentity(actor);

        // Confirms spatial dimensions, origin positions, and velocity vectors are valid
        validateBody(actor);

        // Verifies the exclusive choice between platformer or flying movement configurations
        validateMovement(actor);

        // Check that animation controllers match active, valid sprite configurations
        validatePresentation(actor);

        // Audits health boundaries, attack properties, cooldown durations, and team alliances
        validateCombat(actor);

        // Enforces co-dependency rule constraints across modular AI component tracking blocks
        validateNpc(actor);

        // Confirms frame macro timelines inside active path trackers match valid numbers
        validatePathFollower(actor);
    }

}
