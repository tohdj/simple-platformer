/**
 * @file        actor_validation.hpp
 * @brief       Provides validation signatures ensuring structurally complete and safe
 * initialization parameters for new actors.
 * @author Mohd Fadzuli
 * @author Adapted by: Toh Da Jun
 * @copyright   Copyright (c) 2026 Simple Platformer. All rights reserved.
 */

#pragma once

namespace simple_platformer
{
    struct Actor;
    // Validates a newly composed actor before World assigns its identity.
    void validateActor(const Actor& actor);
}
