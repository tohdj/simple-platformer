#include "actor_definition.hpp"
#include "animation_catalog.hpp"
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <string>
#include "simple_platformer/actor/actor.hpp"
#include <utility>
#include "simple_platformer/actor/actor_validation.hpp"
#include "simple_platformer/combat/combat.hpp"
#include "simple_platformer/math/aabb.hpp"
#include "simple_platformer/render/animation.hpp"
#include "simple_platformer/npc/npc.hpp"
#include "simple_platformer/movement/platformer_movement.hpp"
#include "simple_platformer/movement/surface_climb.hpp"
#include "simple_platformer/navigation/path_follower.hpp"

namespace simple_platformer
{
    // Central factory orchestrator that transforms raw catalog template constraints into a runtime
    // world entity model
    Actor composeActor(
        const ActorDefinition& definition,
        const AnimationCatalog& animations,
        int textureId,
        glm::vec2 spawnFeet,
        std::optional<Patrol> patrol)
    {
        Actor actor;

        // 1. Establish structural orientation properties and physical axis-aligned bounding box
        // layouts
        actor.body.bounds = boxStandingOn(spawnFeet, definition.bodySize);
        actor.team = definition.team;
        actor.facing = definition.facing;

        // --- PHASE 2: INITIALIZE PHYSICAL SYSTEM INDIRECTION WRAPPERS ---
        // Setup specialized kinematics rules if configuration requests standard floor platforming
        // properties
        if (definition.platformer)
        {
            const auto& config = *definition.platformer;
            validatePlatformerMovementConfig(config);
            actor.platformerMovement = PlatformerMovement{};
            actor.platformerMovement->config = config;
            actor.platformerMovement->grounded =
                true; // Enforce safe ground landing checks upon initial level allocation steps
        }

        // Transfer floating mechanics matrices properties directly down to core storage parameters
        actor.flyingMovement = definition.flying;

        // Bind relative mechanical capabilities if wall/ceiling anchor traversal options exist
        if (definition.surfaceClimb)
        {
            actor.surfaceClimb = SurfaceClimb{*definition.surfaceClimb};
        }

        // Allocate structural health limits, mapping maximum value criteria directly down to modern
        // tracking ticks
        if (definition.health)
        {
            actor.health = Health{*definition.health, *definition.health};
        }

        // Set up inventory size capacities, throwing structural errors if structural thresholds
        // register negative counts
        if (definition.inventorySlots)
        {
            if (*definition.inventorySlots <= 0)
            {
                throw std::invalid_argument("inventorySlots must be positive");
            }
            actor.inventory = Inventory{static_cast<std::size_t>(*definition.inventorySlots)};
        }

        // --- PHASE 3: STATE INITIALIZATION FOR AUTOMATED INTELLIGENCE ENGINE MODULES ---
        if (definition.senses)
        {
            actor.brain = NpcBrain{};
            actor.perception = NpcPerception{};
            actor.brain->tactic = definition.tactic; // Link unique high-level combat tactics
                                                     // vectors (e.g., coward vs chaser)
            actor.senses = definition.senses;
            actor.pathFollower = PathFollower{}; // Setup individual routing node meshes containers
        }
        else if (definition.tactic != NpcTactic::Pursuer)
        {
            // Intercept logical fallacies early: behaviors depend completely on sensory monitoring
            // profiles to update correctly
            throw std::invalid_argument("A tactic requires senses");
        }

        // --- PHASE 4: INITIALIZE COMBAT MODULE PARAMETERS & CYCLE ACTIONS COOLDOWNS ---
        actor.patrol = patrol;
        actor.bite = definition.bite;
        actor.contactDamage = definition.contactDamage;

        // Reset status criteria flags for touch-based damage components to safely frame tracking
        // states
        if (actor.contactDamage)
        {
            actor.contactDamage->active = false;
            actor.contactDamage->actorsHit.clear();
        }

        // Re-align close-combat melee configurations to safe defaults, avoiding unintended active
        // loops triggers upon instantiation
        if (actor.bite)
        {
            actor.bite->phase = BitePhase::Ready;
            actor.bite->phaseTimeRemaining = 0;
            actor.bite->actorsHit.clear();
        }

        // Establish ranged weapon mechanics parameters and initialize cooldown registers safely
        actor.rangedWeapon = definition.ranged;
        if (actor.rangedWeapon)
        {
            actor.rangedWeapon->phase = RangedPhase::Ready;
            actor.rangedWeapon->phaseTimeRemaining = 0;
            actor.rangedWeapon->lastFiredTimeSeconds = std::nullopt;
            actor.rangedWeapon->projectileSprite.textureId =
                textureId; // Ensure firing project instances draw out of matching texture sheets
        }

        // --- PHASE 5: COMPILE ANIMATOR BLOCKS AND ASSIGN DEFAULTS INTERFACE SPRITES ---
        if (!definition.animations.empty())
        {
            Animator animator;
            // Fetch structural texture animation key sequences directly out of global resources
            // database lists
            animator.animationSet = animationSet(animations, definition.animations);
            validateAnimationSet(animator.animationSet);

            // Extract the baseline frame layout index out of the global Idle state array list to
            // mount default images
            const auto& frame = clipFor(animator.animationSet, AnimationName::Idle).frames.front();
            actor.sprite = Sprite{textureId, frame};
            actor.sprite->anchor = definition.spriteAnchor;
            actor.animator = std::move(animator);
        }

        // Finalize structural operations through low-level post-construction state checking steps
        validateActor(actor);
        return actor;
    }

    // Dry-run checking interface helper verifying schema structures consistency without populating
    // active entities into maps
    void validateActorDefinition(
        const ActorDefinition& definition,
        const AnimationCatalog& animations)
    {
        // Leverage exact same orchestration pathways with dummy properties values to evaluate
        // blueprint stability loops
        composeActor(definition, animations, 0, {}, std::nullopt);
    }
}
