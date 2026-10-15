/**
 * @file animation_system.cpp
 * @brief Advances active skeletal or sprite sheet frame updates independently of structural physics
 * steps.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#include "simple_platformer/render/animation_system.hpp"

#include <stdexcept>

#include "simple_platformer/actor/actor.hpp"
#include "simple_platformer/combat/combat.hpp"
#include "simple_platformer/math/validation.hpp"
#include "simple_platformer/movement/surface_climb.hpp"
#include "simple_platformer/render/animation.hpp"
#include "simple_platformer/world/world.hpp"

namespace simple_platformer
{
    namespace
    {
        // Cycles through active level characters to match their real physics states to correct
        // animation sequences
        void updateActorAnimations(World& world, float deltaTime)
        {
            for (Actor& actor : world.actors())
            {
                if (!actor.animator.has_value())
                {
                    continue; // Skip entities that lack dynamic animation controller configurations
                }

                // Assert that data profiles have the prerequisite sprite and kinematics linkages
                if ((!actor.platformerMovement.has_value() && !actor.flyingMovement.has_value()) ||
                    !actor.sprite.has_value())
                {
                    throw std::logic_error("An animated actor is missing a required component");
                }

                // Parse action state parameters across tracking fields
                const bool biting = actor.bite.has_value() && actor.bite->phase != BitePhase::Ready;
                const bool shooting = actor.rangedWeapon.has_value() &&
                                      actor.rangedWeapon->phase == RangedPhase::Shoot;
                const bool attacking = biting || shooting;

                // Resolve climbing constraints to deduce attachment angles
                const ClimbSurface surface = actor.surfaceClimb.has_value()
                                                 ? actor.surfaceClimb->surface
                                                 : ClimbSurface::None;

                // A climber holding a surface stands on it: it idles or moves there.
                const bool grounded = surface != ClimbSurface::None ||
                                      !actor.platformerMovement.has_value() ||
                                      actor.platformerMovement->grounded;

                // 1. Pass behavioral states into the dictionary query to select the matching asset
                // key
                const AnimationName selected = selectActorAnimation(
                    actor.life == LifeState::Dying, attacking, grounded, actor.body.velocity);

                // 2. Advance texture region layout frame indexes based on the passed tracking clock
                // duration
                updateAnimation(*actor.animator, *actor.sprite, selected, deltaTime);
            }
        }
    }

    // Advances time steps and cycles frame calculations across all active character visual models
    void updateWorldAnimations(World& world, float deltaTime)
    {
        requireSeconds(deltaTime, "Animations time step");

        // Dispatch temporal updates to resolve frame counts inside actor runtime animators
        updateActorAnimations(world, deltaTime);
    }
}
