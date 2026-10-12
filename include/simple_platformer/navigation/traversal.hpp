/**
 * @file traversal.hpp
 * @brief Defines the movement mechanics and navigation profiles used across the game simulation.
 *
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#pragma once

namespace simple_platformer
{
    /**
     * @brief Defines the movement mechanics and traversal modes used for actor navigation.
     */
    enum class Traversal
    {
        Fly,  /**< Unconstrained movement through space, utilized by airborne actors. */
        Walk, /**< Standard ground-based movement along a horizontal surface. */
        Fall, /**< Downward movement influenced by gravity when unsupported. */
        Jump, /**< Upward vertical propulsion initiated from a surface. */
        Climb /**< Vertical or horizontal movement along climbable geometry like ladders or ropes.
               */
    };
}
