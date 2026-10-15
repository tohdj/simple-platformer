/**
 * @file        imgui_session.hpp
 * @brief       Declares the ImGuiSession context wrapper class for managing the
 *              lifecycle, input backends, and frame boundaries of Dear ImGui.
 *
 * @author      Mohd Fadzuli
 * @author      Adapted by: Toh Da Jun
 * @copyright   Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#pragma once

struct GLFWwindow;

namespace simple_platformer
{
    // Scopes the lifecycle of the ImGui context, binding its GLFW input systems
    // and OpenGL 3 driver commands for the duration of the object's lifetime.
    class ImGuiSession
    {
    public:
        // Allocates the global ImGui state context and hooks up the window callback routines.
        explicit ImGuiSession(GLFWwindow* window);

        // Unbinds the window tracking hooks and safely destroys the working ImGui context.
        ~ImGuiSession();

        // Enforce unique window hook sessions by preventing duplicate context reference coping.
        ImGuiSession(const ImGuiSession&) = delete;
        ImGuiSession& operator=(const ImGuiSession&) = delete;

        // Cleans up internal input buffers and flags the start of a fresh UI command list capture.
        void beginFrame() const;

        // Finalizes collected command queues and renders the visual layers on top of the active
        // frame.
        void render() const;
    };
}
