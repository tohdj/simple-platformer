#pragma once

namespace simple_platformer
{
    class World;
    class WorldRequests;

    /// The duration (in seconds) that an actor remains in a death state
    /// (e.g., for death animations or delay) before being cleaned up or respawned.
    constexpr float ActorDeathSeconds = 0.4F;

    /**
     * @brief Updates the lifecycle, damage states, and death timelines of all actors.
     *
     * This function executes the following sequential logic during the frame update:
     * 1. Applies any pending damage to actors.
     * 2. Advances the death timers *only* for actors that were already dead before this frame.
     * 3. Handles expired death states: respawns the player or queues NPCs for removal.
     *
     * @param world The current state of the game world containing the actors.
     * @param requests The request queue used to submit world modifications (like spawning or
     * removal).
     * @param deltaTime The elapsed time since the last frame in seconds.
     */
    void updateActorLifecycle(World& world, WorldRequests& requests, float deltaTime);
}
