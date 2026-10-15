/**
 * @file        inventory_layout.cpp
 * @brief       Declares the InventoryGridLayout structure and layout helper functions
 *              for structuring the inventory user interface slots into column grids.
 *
 * @author      Mohd Fadzuli
 * @author      Adapted by: Toh Da Jun
 * @copyright   Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#include "inventory_layout.hpp"

#include <algorithm>
#include <cstddef>

namespace simple_platformer
{
    InventoryGridLayout makeInventoryGridLayout(std::size_t slotCount)
    {
        constexpr std::size_t MaximumColumns = 3;
        const std::size_t columns = std::min(slotCount, MaximumColumns);
        if (columns == 0)
        {
            return {};
        }

        const std::size_t rows = (slotCount + columns - 1) / columns;
        return {columns, rows};
    }
}
