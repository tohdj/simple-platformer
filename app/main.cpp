/**
 * @file        main.cpp
 * @brief       The primary entry point for the application, containing the global
 *              main function and top-level error handling blocks.
 *
 * @author      Mohd Fadzuli
 * @author      Adapted by: Toh Da Jun
 * @copyright   Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#include <cstdlib>
#include <exception>
#include <iostream>

#include "application.hpp"

// The main entry point of the entire application program execution
int main()
{
    try
    {
        // Hands off execution control to the platformer's primary setup and update loop engine
        return simple_platformer::runApplication();
    }
    // Top-level global catch-all safety block to intercept unhandled application errors safely
    catch (const std::exception& error)
    {
        // Log the detailed runtime diagnostic error string directly out to the standard error
        // console stream
        std::cerr << "Simple Platformer could not start: " << error.what() << '\n';

        // Return a standard macro error code failure signal back to the operating system
        // environment
        return EXIT_FAILURE;
    }
}
