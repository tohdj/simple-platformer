/**
 * @file item.cpp
 * @brief Blueprint definitions, usage effect categories, and inventory allocation stacks for items.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#include "simple_platformer/inventory/item.hpp"

#include <stdexcept>

namespace simple_platformer
{
    // Runs debug asset assertions to ensure item properties fall within valid boundaries.
    // Verifies properties such as ensuring maximumStack is greater than 0, strings are not empty,
    // and numbers match logic rules.
    void validateItemDefinition(const ItemDefinition& definition)
    {
        // Assert that registration IDs, display labels, and capacity stack caps remain inside
        // positive non-zero boundaries Additionally verify that recovery items provide a numeric
        // benefit, while standard passive elements maintain clean zero metrics
        if (definition.id <= 0 || definition.name.empty() || definition.maximumStack <= 0 ||
            (definition.effect == ItemEffect::Heal && definition.effectAmount <= 0) ||
            (definition.effect == ItemEffect::None && definition.effectAmount != 0))
        {
            throw std::invalid_argument(
                "Item definitions require an ID, name and valid stack/effect");
        }
    }
}
