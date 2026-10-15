/**
 * @file        npc_names.hpp
 * @brief       Declares diagnostic string mapping functions to retrieve human-readable
 *              names for NPC states and tactical behaviors.
 *
 * @author      Mohd Fadzuli
 * @author      Adapted by: Toh Da Jun
 * @copyright   Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#pragma once

namespace simple_platformer
{
    enum class NpcState;
    enum class NpcTactic;

    // The names the overlay prints for an NPC's state and tactic.
    const char* nameOf(NpcState state);
    const char* nameOf(NpcTactic tactic);
}
