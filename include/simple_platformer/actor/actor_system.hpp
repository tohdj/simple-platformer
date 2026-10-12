#pragma once

namespace simple_platformer
{
    class TileMap;
    class World;

    /**
     * @brief Updates the positions and velocities of all active actors in the world.
     *
     * This function processes actor physics and environmental interactions by:
     * 1. Moving actors based on their current velocities and elapsed time.
     * 2. Resolving collisions and boundary constraints against the static tile map geometry.
     * 3. Updating the internal movement states of the actors (e.g., grounding flags, velocity
     * dampening).
     *
     * @param map The static environment geometry used for collision detection.
     * @param world The game world containing the mutable actors to be moved.
     * @param deltaTime The elapsed time since the last frame in seconds.
     */
    void updateActorMovement(const TileMap& map, World& world, float deltaTime);
}
