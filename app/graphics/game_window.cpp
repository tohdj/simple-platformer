/**
 * @file        game_window.cpp
 * @brief       Declares the GameWindow lifecycle controller and window status structure
 *              built around the GLFW context environment.
 *
 * @author      Mohd Fadzuli
 * @author      Adapted by: Toh Da Jun
 * @copyright   Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#include "game_window.hpp"

#include <iostream>
#include <stdexcept>

#include <glad/glad.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <glm/vec2.hpp>

#include "simple_platformer/math/coordinates.hpp"

namespace simple_platformer
{
    // RAII Constructor: Initializes the global GLFW library state and assigns a logging error
    // callback
    GameWindow::GlfwLibrary::GlfwLibrary()
    {
        // Intercept internal framework runtime warnings and route them directly to standard error
        // output
        glfwSetErrorCallback([](int, const char* description)
                             { std::cerr << "GLFW: " << description << '\n'; });

        // Attempt to launch the root framework library instances early
        if (glfwInit() == GLFW_FALSE)
        {
            throw std::runtime_error("GLFW could not start");
        }
    }

    // RAII Destructor: Safely shuts down the global framework system allocations upon application
    // exit
    GameWindow::GlfwLibrary::~GlfwLibrary()
    {
        glfwTerminate();
    }

    // Main Constructor: Configures rendering targets, generates the OS window, and binds GLAD
    // pointers
    GameWindow::GameWindow(const char* title, glm::ivec2 size)
    {
        // Enforce a strict baseline requirements configuration for OpenGL 3.3 target devices
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        // Explicitly use the Core Profile to strip away outdated legacy immediate-mode functions
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        // Mandate forward compatibility to confirm strict driver behavior patterns (critical for
        // macOS support)
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

        // Request a desktop viewport panel handle allocation matching user defined dimensions
        window = glfwCreateWindow(size.x, size.y, title, nullptr, nullptr);
        if (window == nullptr)
        {
            throw std::runtime_error("GLFW could not create the game window");
        }

        // Lock window resizing configurations to prevent crashing or clipping lower than game
        // design minimums
        glfwSetWindowSizeLimits(
            window, InternalWidth, InternalHeight, GLFW_DONT_CARE, GLFW_DONT_CARE);

        // Map the current thread's active graphics command pipeline targets directly onto this
        // display target
        glfwMakeContextCurrent(window);

        // Lock execution swap commands to monitor refresh intervals (Enables V-Sync to prevent
        // screen tearing)
        glfwSwapInterval(1);

        // Dynamically resolve driver-specific function entry addresses for modern hardware shaders
        // execution
        if (gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)) == 0)
        {
            throw std::runtime_error("GLAD could not load OpenGL");
        }
    }

    // Clean up local system resources by destroying the application context handle safely
    GameWindow::~GameWindow()
    {
        glfwDestroyWindow(window);
    }

    // Exposes the raw underlying window pointer interface used directly by downstream backends like
    // ImGui
    GLFWwindow* GameWindow::handle() const
    {
        return window;
    }

    // Returns whether the OS interface container or window control items have registered a closing
    // execution query
    bool GameWindow::shouldClose() const
    {
        return glfwWindowShouldClose(window) == GLFW_TRUE;
    }

    // Samples snapshot structural values (Window sizing bounds vs explicit pixels density and
    // cursor locations)
    WindowReading GameWindow::read() const
    {
        WindowReading reading;
        double cursorX = 0.0;
        double cursorY = 0.0;

        // Fetch virtual screen layout coordinates spacing configurations
        glfwGetWindowSize(window, &reading.size.x, &reading.size.y);
        // Fetch explicit physical backbuffer resolution metrics (critical to map High-DPI/Retina
        // screens cleanly)
        glfwGetFramebufferSize(window, &reading.framebufferSize.x, &reading.framebufferSize.y);
        // Fetch relative spatial coordinates tracking current location bounds of pointing hardware
        // devices
        glfwGetCursorPos(window, &cursorX, &cursorY);

        reading.cursor = {static_cast<float>(cursorX), static_cast<float>(cursorY)};
        return reading;
    }

    // Commits the finalized frame draw commands onto the screen by swapping back and front graphics
    // buffers
    void GameWindow::present() const
    {
        glfwSwapBuffers(window);
    }
}
