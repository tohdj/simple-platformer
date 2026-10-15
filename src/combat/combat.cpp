/**
 * @file combat.cpp
 * @brief Component declarations, attack lifecycles, and tracking structures for weapon and contact
 * combat.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#include "simple_platformer/combat/combat.hpp"

namespace simple_platformer
{
    // Evaluates whether two teams are mutually hostile towards each other.
    bool areOpponents(Team first, Team second)
    {
        return (first == Team::Player && second == Team::Enemy) ||
               (first == Team::Enemy && second == Team::Player);
    }
}
