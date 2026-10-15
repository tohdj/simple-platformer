/**
 * @file        inventory_layout.hpp
 * @brief       Declares the InventoryGridLayout structure and layout helper functions
 *              for structuring the inventory user interface slots into column grids.
 *
 * @author      Mohd Fadzuli
 * @author      Adapted by: Toh Da Jun
 * @copyright   Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#pragma once

#include <cstddef>

namespace simple_platformer
{
    struct InventoryGridLayout
    {
        std::size_t columns = 0;
        std::size_t rows = 0;
    };

    // Inventory slots use up to three columns and add rows as capacity grows.
    InventoryGridLayout makeInventoryGridLayout(std::size_t slotCount);
}
