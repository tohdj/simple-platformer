/**
 * @file level_exit.cpp
 * @brief Logic, structure validation, and unlock mechanics for level transitions and stage exit
 * volumes.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#include "simple_platformer/world/level_exit.hpp"

#include <optional>
#include <stdexcept>

#include "simple_platformer/actor/actor.hpp"
#include "simple_platformer/inventory/inventory.hpp"
#include "simple_platformer/math/aabb.hpp"
#include "simple_platformer/math/validation.hpp"
#include "simple_platformer/world/world.hpp"

namespace simple_platformer
{
    // Asserts that a LevelExit structure possesses finite dimensions, valid counts, and reachable
    // stage IDs
    void validateLevelExit(const LevelExit& exit)
    {
        if (!simple_platformer::isFinite(exit.bounds.topLeft) ||
            !simple_platformer::isFinite(exit.bounds.size) || exit.bounds.size.x <= 0.0F ||
            exit.bounds.size.y <= 0.0F)
        {
            throw std::invalid_argument("Level exits require finite positive-sized bounds");
        }

        // Ensure standard requirement counters (e.g., keys, stars) ask for realistic quantities
        if (exit.requirement.has_value())
        {
            if (exit.requirement->quantity <= 0)
            {
                throw std::invalid_argument("Exit requirements must be positive");
            }
        }

        // Assert that the indexing index pointing to the subsequent map asset stays in positive
        // bounds
        if (exit.nextLevel.has_value() && *exit.nextLevel < 0)
        {
            throw std::invalid_argument("Level IDs must be non-negative");
        }
    }

    // Assigns or overwrites the active exit portal zone setup parameters for the current world map
    // simulation
    void World::setExit(LevelExit exit)
    {
        validateLevelExit(exit);
        if (exit.requirement)
        {
            itemDefinition(exit.requirement
                               ->item); // Assert database record availability for the gate key item
        }

        // Validate internal historical simulation timeline markers for sanity
        requireWithinSimulationTime(exit.lastLockedTouchTimeSeconds, "Exit touch time");
        requireWithinSimulationTime(exit.openedTimeSeconds, "Exit opening time");

        levelExit = exit;
        completed = false; // Reset level progression state back to pending completion status
    }

    // Accessors for reading or modifying the currently loaded level portal parameters block
    const std::optional<LevelExit>& World::exit() const
    {
        return levelExit;
    }

    std::optional<LevelExit>& World::exit()
    {
        return levelExit;
    }

    // Returns whether the active stage setup loop has completed all terminal conditions
    bool World::levelComplete() const
    {
        return completed;
    }

    // Sets flag statuses forcing the main engine loops to advance toward the summary/next stage
    // screen
    void World::completeLevel()
    {
        completed = true;
    }

    // Compares player item registry balances to confirm they meet minimum gate access totals
    bool exitUnlocked(const LevelExit& exit, const Actor& actor)
    {
        return !exit.requirement.has_value() ||
               (actor.inventory.has_value() &&
                actor.inventory->count(exit.requirement->item) >= exit.requirement->quantity);
    }

    // Returns true if a zone exit loop is actively playing out its opening sequence/animations
    bool exitOpening(const World& world)
    {
        const auto& levelExit = world.exit();
        return levelExit.has_value() && levelExit->openedTimeSeconds.has_value() &&
               !world.levelComplete();
    }

    // Evaluates spatial boundaries overlap logic and manages transitions through item consumption
    // and gate delay loops
    void updateLevelExit(World& world)
    {
        Actor* player = world.findActor(world.playerId());
        std::optional<LevelExit>& levelExit = world.exit();

        // Early escape: Skip checks if level is over, missing an exit point, or if the user is
        // missing/dead
        if (world.levelComplete() || !levelExit.has_value() || player == nullptr ||
            player->life != LifeState::Alive)
        {
            return;
        }

        LevelExit& exit = levelExit.value();
        const std::optional<float> sinceOpened = world.secondsSince(exit.openedTimeSeconds);

        // 1. Process active opening delay countdown metrics (holds state to let screen fade-outs
        // play cleanly)
        if (sinceOpened.has_value())
        {
            if (*sinceOpened >= ExitOpenSeconds)
            {
                world.completeLevel(); // Complete the phase entirely after the designated delay
                                       // window
            }
            return;
        }

        // 2. Perform bounding intersection tests between the primary character avatar and the
        // trigger node zone
        const double now = world.simulationTimeSeconds();
        if (!overlaps(player->body.bounds, exit.bounds))
        {
            return;
        }

        // 3. Handle rejection callbacks if requirements check blocks key access tokens
        if (!exitUnlocked(exit, *player))
        {
            exit.lastLockedTouchTimeSeconds =
                now; // Logs the trigger event timestamp for lock vibration feedback animations
            return;
        }

        // 4. Consume key items from user inventory slots if the exit configuration requires burning
        // resources
        if (exit.consumeItem && exit.requirement.has_value() && player->inventory.has_value())
        {
            player->inventory->remove(exit.requirement->item, exit.requirement->quantity);
        }

        // 5. Save activation stamp records to initialize step countdown cycles
        exit.openedTimeSeconds = now;
    }
}
