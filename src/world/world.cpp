/**
 * @file world.cpp
 * @brief Core state container and manager for the active gameplay simulation world.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#include "simple_platformer/world/world.hpp"

#include <algorithm>
#include <cstddef>
#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <glm/vec2.hpp>

#include "simple_platformer/actor/actor.hpp"
#include "simple_platformer/actor/actor_validation.hpp"
#include "simple_platformer/actor/actor_id.hpp"
#include "simple_platformer/combat/combat.hpp"
#include "simple_platformer/inventory/item.hpp"
#include "simple_platformer/navigation/platformer_connection_table.hpp"
#include "simple_platformer/math/aabb.hpp"
#include "simple_platformer/math/validation.hpp"
#include "simple_platformer/movement/platformer_movement.hpp"

namespace simple_platformer
{
    namespace
    {
        // Validates all internal attributes of a projectile to prevent simulation corruptions
        void validateProjectile(const simple_platformer::Projectile& projectile)
        {
            if (!isFinite(projectile.bounds.topLeft) || !isFinitePositive(projectile.bounds.size) ||
                !isFinite(projectile.velocity) || projectile.damage <= 0 ||
                !isFinitePositive(projectile.lifetimeRemaining) ||
                !isFinitePositive(projectile.sprite.region.size) ||
                (projectile.owner.has_value() && !simple_platformer::isValid(*projectile.owner)))
            {
                throw std::invalid_argument("Projectile data is invalid");
            }
        }

        // Validates projectile burst conditions, checking life metrics, valid causes, and
        // structural limits
        void validateProjectileBurst(const simple_platformer::ProjectileBurst& burst)
        {
            const bool hasValidCause =
                burst.cause == simple_platformer::ProjectileBurstCause::Impact ||
                burst.cause == simple_platformer::ProjectileBurstCause::LifetimeExpired;
            if (!hasValidCause || !isFinite(burst.center) || !isFinite(burst.direction) ||
                (burst.direction.x == 0.0F && burst.direction.y == 0.0F) ||
                !isFinitePositive(burst.sprite.region.size) || !isFinitePositive(burst.duration) ||
                !isFinitePositive(burst.lifetimeRemaining) ||
                burst.lifetimeRemaining > burst.duration)
            {
                throw std::invalid_argument("Projectile burst data is invalid");
            }
        }
    }

    // Constructor: Initializes the items collection while asserting item integrity and unique
    // registration IDs
    World::World(std::vector<ItemDefinition> items)
        : itemDefinitions(std::move(items))
    {
        for (std::size_t index = 0; index < itemDefinitions.size(); ++index)
        {
            validateItemDefinition(itemDefinitions[index]);
            for (std::size_t previous = 0; previous < index; ++previous)
            {
                if (itemDefinitions[previous].id == itemDefinitions[index].id)
                {
                    throw std::invalid_argument("Item IDs must be unique");
                }
            }
        }
    }

    // Asserves that a given timestamp falls properly between the start of the game and the current
    // timeline
    void World::requireWithinSimulationTime(const std::optional<double>& time, const char* what)
        const
    {
        if (time.has_value() &&
            (!std::isfinite(*time) || *time < 0.0 || *time > elapsedSimulationTimeSeconds))
        {
            throw std::invalid_argument(std::string(what) + " must be within simulation time");
        }
    }

    // Accessors for the pathfinding/connection grid table used by actors and entities
    PlatformerConnectionTable& World::platformerConnections()
    {
        return platformerConnectionTable;
    }

    const PlatformerConnectionTable& World::platformerConnections() const
    {
        return platformerConnectionTable;
    }

    // Returns total accumulated runtime simulation seconds
    double World::simulationTimeSeconds() const
    {
        return elapsedSimulationTimeSeconds;
    }

    // Queues a transient sound/noise event into the physics/AI notification pipeline
    void World::emitNoise(NoiseEvent event)
    {
        if (!isValid(event.source) || !isFinite(event.feet))
        {
            throw std::invalid_argument("Noise events require a source and finite feet");
        }
        pendingNoises.push_back(event);
    }

    // Flushes out and returns the list of collected sound notifications for AI processing
    std::vector<NoiseEvent> World::takeNoises()
    {
        std::vector<NoiseEvent> events = std::move(pendingNoises);
        pendingNoises.clear();
        return events;
    }

    // Calculates delta time passed since a past simulation timestamp
    std::optional<float> World::secondsSince(const std::optional<double>& timeSeconds) const
    {
        requireWithinSimulationTime(timeSeconds, "Stamp");
        if (!timeSeconds.has_value())
        {
            return std::nullopt;
        }
        return static_cast<float>(elapsedSimulationTimeSeconds - *timeSeconds);
    }

    // Steps forward the primary clock baseline of the global simulation domain
    void World::advanceSimulationTime(float deltaTime)
    {
        requireSeconds(deltaTime, "Simulation time step");
        elapsedSimulationTimeSeconds += deltaTime;
    }

    // Performs a safe identification look-up across the items database
    const ItemDefinition& World::itemDefinition(ItemId id) const
    {
        for (const ItemDefinition& definition : itemDefinitions)
        {
            if (definition.id == id)
            {
                return definition;
            }
        }
        throw std::invalid_argument("Unknown item ID");
    }

    // Registers a fully validated new entity into active simulation, auto-assigning a safe unique
    // ID
    ActorId World::addActor(Actor actor)
    {
        validateActor(actor);
        requireWithinSimulationTime(actor.lastDamageTimeSeconds, "Actor damage time");
        if (actor.rangedWeapon.has_value())
        {
            requireWithinSimulationTime(
                actor.rangedWeapon->lastFiredTimeSeconds, "Actor shot time");
        }
        if (nextActorId == std::numeric_limits<std::uint32_t>::max())
        {
            throw std::overflow_error("Actor IDs have been exhausted");
        }

        actor.id = {nextActorId};
        ++nextActorId;
        const ActorId id = actor.id;
        actorStorage.push_back(actor);
        return id;
    }

    // Erases an actor by ID, clearing active player links if it matches the current hero entity
    bool World::removeActor(ActorId id)
    {
        const auto actor = std::find_if(
            actorStorage.begin(),
            actorStorage.end(),
            [id](const Actor& candidate) { return candidate.id == id; });
        if (actor == actorStorage.end())
        {
            return false;
        }

        actorStorage.erase(actor);
        if (controlledPlayer == id)
        {
            controlledPlayer = {};
        }
        return true;
    }

    // Entity lookup interfaces (mutable/immutable versions) returning references or nullpointers
    Actor* World::findActor(ActorId id)
    {
        const auto actor = std::find_if(
            actorStorage.begin(),
            actorStorage.end(),
            [id](const Actor& candidate) { return candidate.id == id; });
        return actor == actorStorage.end() ? nullptr : &*actor;
    }

    const Actor* World::findActor(ActorId id) const
    {
        const auto actor = std::find_if(
            actorStorage.begin(),
            actorStorage.end(),
            [id](const Actor& candidate) { return candidate.id == id; });
        return actor == actorStorage.end() ? nullptr : &*actor;
    }

    // Exposes the underlying database vectors for active actors
    std::vector<Actor>& World::actors()
    {
        return actorStorage;
    }

    const std::vector<Actor>& World::actors() const
    {
        return actorStorage;
    }

    // Appends a new verified projectile node into the execution path
    void World::addProjectile(Projectile projectile)
    {
        validateProjectile(projectile);
        projectileStorage.push_back(projectile);
    }

    // Safely pops a projectile from vector slots without invalidating tracking states
    bool World::removeProjectile(std::size_t index)
    {
        if (index >= projectileStorage.size())
        {
            return false;
        }

        projectileStorage.erase(projectileStorage.begin() + static_cast<std::ptrdiff_t>(index));
        return true;
    }

    // Internal arrays exposure for moving projectiles
    std::vector<Projectile>& World::projectiles()
    {
        return projectileStorage;
    }

    const std::vector<Projectile>& World::projectiles() const
    {
        return projectileStorage;
    }

    // Injects a particle effect system configuration structure into world space
    void World::addProjectileBurst(ProjectileBurst burst)
    {
        validateProjectileBurst(burst);
        projectileBurstStorage.push_back(burst);
    }

    // Safely removes a burst index node when its lifetime constraints end
    bool World::removeProjectileBurst(std::size_t index)
    {
        if (index >= projectileBurstStorage.size())
        {
            return false;
        }

        projectileBurstStorage.erase(
            projectileBurstStorage.begin() + static_cast<std::ptrdiff_t>(index));
        return true;
    }

    // Collection reference maps for active bursts
    std::vector<ProjectileBurst>& World::projectileBursts()
    {
        return projectileBurstStorage;
    }

    const std::vector<ProjectileBurst>& World::projectileBursts() const
    {
        return projectileBurstStorage;
    }

    // Pairs an existing actor identity to the primary controller input pipeline and saves its home
    // point
    void World::setPlayer(ActorId id, glm::vec2 spawnFeet)
    {
        if (findActor(id) == nullptr || !isFinite(spawnFeet))
        {
            throw std::invalid_argument("The player must be an existing actor with a finite spawn");
        }

        controlledPlayer = id;
        controlledPlayerSpawnFeet = spawnFeet;
    }

    // Gets the registration tracking identity of the current hero character
    ActorId World::playerId() const
    {
        return controlledPlayer;
    }

    // Get the player
    glm::vec2 World::playerSpawnFeet() const
    {
        if (!isValid(controlledPlayer))
        {
            throw std::logic_error("The world has no player");
        }
        return controlledPlayerSpawnFeet;
    }

    // Restores the player entity's spatial coordinates, movement physics, states, and health
    // properties to default values upon death or stage resets
    void World::respawnPlayer()
    {
        // 1. Locate the controlled hero instance in the world entity database
        Actor* player = findActor(controlledPlayer);
        if (player == nullptr)
        {
            throw std::logic_error("The world has no player to respawn");
        }

        // 2. Relocate physical bounds to the registered spawn point and halt momentum completely
        moveFeetTo(player->body.bounds, controlledPlayerSpawnFeet);
        player->body.velocity = {0.0F, 0.0F};

        // 3. Purge queued controller inputs to prevent carrying over actions across spawn
        // boundaries
        player->intentions = {};

        // 4. Reset life state metrics and clear the historic invulnerability timestamp record
        player->life = LifeState::Alive;
        player->deathTimeRemaining = 0.0F;
        player->lastDamageTimeSeconds.reset();

        // 5. Fully restore vital stats if the actor utilizes standard health structures
        if (player->health.has_value())
        {
            player->health->current = player->health->maximum;
        }

        // 6. Refresh kinematic platforming behaviors to a clean, mid-air falling state
        if (player->platformerMovement.has_value())
        {
            player->platformerMovement->grounded = false;
            player->platformerMovement->coyoteRemaining =
                0.0F; // Disables walking off ledges jump privileges
            player->platformerMovement->jumpBufferRemaining =
                0.0F; // Drops pre-landing input queues
        }
    }

}
