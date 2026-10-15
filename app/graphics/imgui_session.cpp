/**
 * @file        imgui_session.cpp
 * @brief       Declares the ImGuiSession context wrapper class for managing the
 *              lifecycle, input backends, and frame boundaries of Dear ImGui.
 *
 * @author      Mohd Fadzuli
 * @author      Adapted by: Toh Da Jun
 * @copyright   Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#include "imgui_session.hpp"

#include <stdexcept>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

namespace simple_platformer
{
    namespace
    {
        // Internal clean-up helper function to free the underlying ImGui core instance memory
        void destroyContexts()
        {
            ImGui::DestroyContext();
        }
    }

    // Constructor: Sets up the ImGui runtime context and binds it to the window and OpenGL backends
    ImGuiSession::ImGuiSession(GLFWwindow* window)
    {
        // Validates structure sizes to confirm the linked header matches the compilation binary
        IMGUI_CHECKVERSION();
        // Creates and sets the active global context configuration state instance
        ImGui::CreateContext();
        // Applies the standard default dark interface color scheme palette configuration
        ImGui::StyleColorsDark();

        // Bind and initialise the input event hooks directly onto our physical GLFW viewport
        // framework
        if (!ImGui_ImplGlfw_InitForOpenGL(window, true))
        {
            destroyContexts(); // Clean up core allocations early upon initialization failures
            throw std::runtime_error("ImGui could not start its GLFW backend");
        }
        // Link the drawing pipelines to OpenGL 3.3 programmable pipeline shaders
        if (!ImGui_ImplOpenGL3_Init("#version 330 core"))
        {
            ImGui_ImplGlfw_Shutdown(); // Clean up preceding successfully initialized platform
                                       // dependencies
            destroyContexts();
            throw std::runtime_error("ImGui could not start its OpenGL backend");
        }
    }

    // Destructor: Safely deallocates graphics hooks, input hooks, and core contexts in reverse
    // lifecycle order
    ImGuiSession::~ImGuiSession()
    {
        ImGui_ImplOpenGL3_Shutdown(); // Terminate OpenGL textures and shader buffers
        ImGui_ImplGlfw_Shutdown();    // Sever GLFW window callback listener assignments
        destroyContexts();            // Destroys the main internal context layout allocations
    }

    // Prepares internal command structures and starts tracking frame state timing definitions
    void ImGuiSession::beginFrame() const
    {
        ImGui_ImplOpenGL3_NewFrame(); // Pull updated device rendering bounds transformations
        ImGui_ImplGlfw_NewFrame();    // Fetch current mouse coordinates, keyboard inputs, and
                                      // scrolling deltas
        ImGui::NewFrame(); // Signals ImGui that widget definitions are ready to be parsed
    }

    // Compiles UI layout declarations down into hardware draw command arrays and sends them to the
    // GPU
    void ImGuiSession::render() const
    {
        ImGui::Render(); // Finalizes the vertex and text index array buffers for this frame
        // Submit the finalized drawing structure vectors directly over to the active OpenGL
        // rendering state
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
}
