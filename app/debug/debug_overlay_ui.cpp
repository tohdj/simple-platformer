/**
 * @file        debug_overlay_ui.cpp
 * @brief       Declares high-level rendering functions for drawing real-time game state
 *              overlays and world-space diagnostic layers.
 *
 * @author      Mohd Fadzuli
 * @author      Adapted by: Toh Da Jun
 * @copyright   Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#include "debug_overlay_ui.hpp"

#include "debug_draw.hpp"
#include "debug_overlay.hpp"
#include "debug_ui_layout.hpp"
#include "navigation_debug.hpp"
#include "navigation_debug_ui.hpp"
#include "npc_names.hpp"
#include "graphics/display_viewport.hpp"

#include <cstddef>
#include <cstdio>
#include <optional>
#include <string>

#include <imgui.h>

#include "simple_platformer/math/aabb.hpp"
#include "simple_platformer/navigation/traversal.hpp"
#include "simple_platformer/render/animation.hpp"
#include "ui/hud_draw.hpp"

namespace simple_platformer
{
    namespace
    {
        // Vertical spacing offset constant in pixels used to separate multiple text rows above
        // entities
        constexpr float ActorTextGap = 4.0F;

        // Translates an abstract animation enum token into a human-readable string description
        const char* nameOf(AnimationName animation)
        {
            switch (animation)
            {
            case AnimationName::Idle:
                return "Idle"; // Passive rest stance loops
            case AnimationName::Move:
                return "Move"; // Ground running or movement transitions
            case AnimationName::Jump:
                return "Jump"; // Vertical upward acceleration phase
            case AnimationName::Fall:
                return "Fall"; // Downward velocity aerial phase
            case AnimationName::Attack:
                return "Attack"; // Triggering combat hitbox interactions
            case AnimationName::Death:
                return "Death"; // Lifecycle termination phase
            }

            return "Unknown"; // Safe fallback tracker value placeholder
        }

        // Translates an abstract navigation traversal enum token into a readable string description
        const char* nameOf(Traversal traversal)
        {
            switch (traversal)
            {
            case Traversal::Fly:
                return "Fly"; // Multi-directional free flying movement arcs
            case Traversal::Walk:
                return "Walk"; // Solid horizontal floor walking navigation
            case Traversal::Fall:
                return "Fall"; // Gravity-assisted vertical downward drops
            case Traversal::Jump:
                return "Jump"; // Path-calculated parabolic vertical leaps
            case Traversal::Climb:
                return "Climb"; // Vertical wall or ladder climbing anchor traces
            }

            return "Unknown"; // Safe fallback tracker value placeholder
        }

        // Generates a base descriptive identity classification string for an entity context
        std::string labelFor(const ActorDebugInfo& actor)
        {
            switch (actor.kind)
            {
            case ActorDebugKind::Player:
                return "Player"; // Isolated human controller instance tag
            case ActorDebugKind::Npc:
                return "NPC " +
                       std::to_string(actor.id.value); // Appends automated entity tracker indices
            case ActorDebugKind::Actor:
                return "Actor " +
                       std::to_string(actor.id.value); // Generic dynamic interactive world entity
            }

            return "Actor"; // Structural identification type fallback
        }

        // Combines classification labels and asset description metadata tags for explicit entity
        // logs
        std::string actorTextLabelFor(const ActorDebugInfo& actor)
        {
            std::string label = labelFor(actor);
            // If the entity points to a valid entry inside catalog template arrays, append the
            // blueprint name
            if (actor.definitionName.has_value())
            {
                return label + " (" + *actor.definitionName + ")";
            }
            return label;
        }

        // Evaluates pathfinding attributes to return the target drawing color palette (packed
        // 32-bit RGBA)
        ImU32 pathColour(const PathConnectionDebugInfo& connection)
        {
            // Dim colors out significantly using faded color maps if the segment has already been
            // completed
            if (connection.completed)
            {
                return CompletedPathColour;
            }

            // Route distinctive trace colors depending on the active spatial mobility mechanics
            switch (connection.traversal)
            {
            case Traversal::Fly:
                return FlyingPathColour;
            case Traversal::Walk:
                return WalkingPathColour;
            case Traversal::Fall:
                return FallingPathColour;
            case Traversal::Jump:
                return JumpingPathColour;
            case Traversal::Climb:
                return WalkingPathColour;
            }

            return UnknownPathColour;
        }

        // Draws dynamic navigation node sequences, jump splines, numerical labels, and guidance
        // vectors on screen
        void drawPathConnection(
            ImDrawList& drawList,
            const PathConnectionDebugInfo& connection,
            std::size_t pointNumber,
            const ActorDebugInfo& actor,
            const DebugOverlay& scene,
            const WindowViewport& viewport)
        {
            // Project absolute world vectors down into scaled native UI screen positions
            const ImVec2 from = screenPosition(connection.fromFeet, scene.cameraBounds, viewport);
            const ImVec2 to = screenPosition(connection.toFeet, scene.cameraBounds, viewport);
            const ImU32 colour = pathColour(connection);

            // Highlight the immediate targeted waypoint path segment by inflating line thickness
            // metrics
            const float thickness = connection.next ? 3.0F : 2.0F;

            // --- STAGE 1: TRACE WAYPOINT PATH CONNECTIONS ---
            if (connection.sampledFeet.size() >= 2)
            {
                // Multi-segment trace: connect successive ticks to accurately plot micro-airborne
                // parabolas
                for (std::size_t sampleIndex = 1; sampleIndex < connection.sampledFeet.size();
                     ++sampleIndex)
                {
                    drawList.AddLine(
                        screenPosition(
                            connection.sampledFeet[sampleIndex - 1], scene.cameraBounds, viewport),
                        screenPosition(
                            connection.sampledFeet[sampleIndex], scene.cameraBounds, viewport),
                        colour,
                        thickness);
                }
            }
            else
            {
                // Linear fallback trace: draw a direct straight line segment if custom tick loops
                // are empty
                drawList.AddLine(from, to, colour, thickness);
            }

            // Place a solid anchor circle dot indicating the physical location vertex node node
            // bounds
            drawList.AddCircleFilled(to, connection.next ? 4.0F : 3.0F, colour);

            // --- STAGE 2: DRAW CHRONOLOGICAL NODE ROUTE TEXT MARKERS ---
            char pointLabel[16]{};
            std::snprintf(pointLabel, sizeof(pointLabel), "%zu", pointNumber);
            const ImVec2 pointLabelSize = ImGui::CalcTextSize(pointLabel);
            // Center the numerical text label cleanly right over the node anchor point dot
            drawShadowedText(
                drawList,
                {to.x - pointLabelSize.x * 0.5F,
                 to.y - pointLabelSize.y - (connection.next ? 6.0F : 5.0F)},
                colour,
                pointLabel);

            // --- STAGE 3: INTERACTION LOGIC FOR ACTIVE CURRENT SEGMENTS ---
            if (connection.next)
            {
                // Locate the optimal mid-point coordinate along the arc to mount descriptive
                // floating text tags
                const glm::vec2 labelWorldPosition =
                    connection.sampledFeet.empty()
                        ? (connection.fromFeet + connection.toFeet) * 0.5F
                        : connection.sampledFeet[connection.sampledFeet.size() / 2];
                const ImVec2 labelPosition =
                    screenPosition(labelWorldPosition, scene.cameraBounds, viewport);

                const char* traversalName = nameOf(connection.traversal);
                drawShadowedText(drawList, labelPosition, colour, traversalName);

                // Draw a contextual vector guide line connecting the entity directly to its current
                // active destination node
                drawList.AddLine(
                    screenPosition(feetOf(actor.collider), scene.cameraBounds, viewport),
                    to,
                    NextPathGuideColour);
            }
        }

        // Iterates through an actor's pathfinding route and visually reconstructs the navigation
        // node segments and target goal
        void drawActorPath(
            ImDrawList& drawList,
            const ActorDebugInfo& actor,
            const DebugOverlay& scene,
            const WindowViewport& viewport)
        {
            // Abort if the target actor has no active pathfinding or route state calculated
            if (!actor.pathFollower.has_value())
            {
                return;
            }

            // --- STAGE 1: TRACE INDIVIDUAL WAYPOINT CONNECTIONS ---
            const PathFollowerDebugInfo& follower = actor.pathFollower.value();
            for (std::size_t index = 0; index < follower.connections.size(); ++index)
            {
                const PathConnectionDebugInfo& connection = follower.connections[index];
                // Pass 1-based indexing (index + 1) to draw steps for human-readable overlay node
                // labeling
                drawPathConnection(drawList, connection, index + 1, actor, scene, viewport);
            }

            // --- STAGE 2: RENDER ULTIMATE ROUTE DESTINATION MARKER ---
            if (follower.goalFeet.has_value())
            {
                constexpr float GoalRadius = 6.0F;
                // Translate the absolute world space coordinate vector into screen pixels
                const ImVec2 goal =
                    screenPosition(follower.goalFeet.value(), scene.cameraBounds, viewport);

                // Overlay a distinct double-pixel thickness wireframe circle targeting the
                // destination coordinates
                drawList.AddCircle(goal, GoalRadius, PathGoalColour, 16, 2.0F);

                // Align the "Goal" text label directly adjacent to the circle element with a
                // half-line vertical center offset
                drawShadowedText(
                    drawList,
                    {goal.x + GoalRadius + 2.0F, goal.y - ImGui::GetTextLineHeight() * 0.5F},
                    PathGoalColour,
                    "Goal");
            }
        }

        // Renders visual overlays displaying an NPC's vision notice radius, active alert
        // line-of-sight, and memory persistence
        void drawActorSensor(
            ImDrawList& drawList,
            const ActorDebugInfo& actor,
            const DebugOverlay& scene,
            const WindowViewport& viewport)
        {
            // Drop processing if the actor contains no simulated sensory perception module fields
            if (!actor.sensor.has_value())
            {
                return;
            }

            const SensorDebugInfo& sensor = actor.sensor.value();
            // Project the eye vector vantage point origin out into screen coordinates
            const ImVec2 observer =
                screenPosition(sensor.observerCenter, scene.cameraBounds, viewport);

            // --- STAGE 1: DRAW FIELD OF VIEW DETECTION RADIUS ---
            // Notice detection distance is multiplied by the viewport horizontal scale factor to
            // preserve correct spatial ratios
            drawList.AddCircle(
                observer, sensor.noticeDistance * viewport.scale.x, SensorRangeColour, 48, 1.0F);

            // --- STAGE 2: DRAW LIVE TARGETING VECTOR LINE ---
            if (sensor.visibleTargetCenter.has_value())
            {
                // Draw a direct wire vector line straight from the NPC eye center to the visible
                // target center
                drawList.AddLine(
                    observer,
                    screenPosition(
                        sensor.visibleTargetCenter.value(), scene.cameraBounds, viewport),
                    VisibleTargetColour,
                    2.0F);
            }

            // --- STAGE 3: DRAW CACHED TARGET MEMORY RECURSION MARKERS ---
            if (sensor.rememberedTargetFeet.has_value())
            {
                constexpr float MarkerRadius = 4.0F;
                const ImVec2 remembered = screenPosition(
                    sensor.rememberedTargetFeet.value(), scene.cameraBounds, viewport);

                // Connect origin eye center to last known target position metrics via a thin
                // tracker vector line
                drawList.AddLine(observer, remembered, RememberedTargetColour, 1.5F);

                // Assemble an "X" coordinate cross box layout indicating historical search target
                // spots
                drawList.AddLine(
                    {remembered.x - MarkerRadius, remembered.y - MarkerRadius},
                    {remembered.x + MarkerRadius, remembered.y + MarkerRadius},
                    RememberedTargetColour,
                    2.0F);
                drawList.AddLine(
                    {remembered.x - MarkerRadius, remembered.y + MarkerRadius},
                    {remembered.x + MarkerRadius, remembered.y - MarkerRadius},
                    RememberedTargetColour,
                    2.0F);

                // Append remaining target visibility persistence countdown value alongside the
                // layout cross indicators
                char memoryLabel[32]{};
                std::snprintf(memoryLabel, sizeof(memoryLabel), "%.2fs", sensor.memoryRemaining);
                drawShadowedText(
                    drawList,
                    {remembered.x + MarkerRadius + 2.0F, remembered.y - MarkerRadius},
                    RememberedTargetColour,
                    memoryLabel);
            }
        }

        // Renders automated linear patrol paths, endpoint nodes, and highlights active destination
        // nodes
        void drawActorPatrol(
            ImDrawList& drawList,
            const ActorDebugInfo& actor,
            const DebugOverlay& scene,
            const WindowViewport& viewport)
        {
            // Exit early if the target entity is missing a patrol layout configuration itinerary
            if (!actor.patrol.has_value())
            {
                return;
            }

            constexpr float PointRadius = 4.0F;
            constexpr float ActivePointRadius = 7.0F;
            const PatrolDebugInfo& patrol = actor.patrol.value();

            // Translate absolute world coordinate milestone variables into screen pixels
            const ImVec2 first = screenPosition(patrol.firstFeet, scene.cameraBounds, viewport);
            const ImVec2 second = screenPosition(patrol.secondFeet, scene.cameraBounds, viewport);

            // Draw a solid travel line segment bridging the complete route loop boundary parameters
            drawList.AddLine(first, second, PatrolRouteColour, 2.0F);

            // Place solid dot markers identifying static node landmark endpoints
            drawList.AddCircleFilled(first, PointRadius, PatrolPointColour);
            drawList.AddCircleFilled(second, PointRadius, PatrolPointColour);

            // Ring highlight the specific node index currently target tracked by the state logic
            // loops
            drawList.AddCircle(
                patrol.headingToSecond ? second : first,
                ActivePointRadius,
                ActivePatrolPointColour,
                16,
                2.0F);

            // Nested helper lambda function to construct stacked identification text labels above
            // patrol milestones
            const auto drawPatrolLabel =
                [&drawList, &actor, ActivePointRadius](ImVec2 point, const char* pointName)
            {
                char actorLabel[32]{};
                std::snprintf(actorLabel, sizeof(actorLabel), "NPC %u", actor.id.value);
                const float lineHeight = ImGui::GetTextLineHeight();

                // Float text rows safely clear of the outer active ring highlight dot graphic
                // structures
                const float top = point.y - ActivePointRadius - 2.0F - lineHeight * 2.0F;
                const ImVec2 actorLabelSize = ImGui::CalcTextSize(actorLabel);
                const ImVec2 pointLabelSize = ImGui::CalcTextSize(pointName);

                // Horizontally center align the font assets cleanly using the calculated pixel size
                // offsets
                drawShadowedText(
                    drawList,
                    {point.x - actorLabelSize.x * 0.5F, top},
                    PatrolPointColour,
                    actorLabel);
                drawShadowedText(
                    drawList,
                    {point.x - pointLabelSize.x * 0.5F, top + lineHeight},
                    PatrolPointColour,
                    pointName);
            };

            // Generate visual descriptions for both milestone nodes
            drawPatrolLabel(first, "P1");
            drawPatrolLabel(second, "P2");
        }

        // Projects real-time entity identification labels (Type, Animation State, AI Behavior
        // State) above entity bounds
        void drawActorWorldLabel(
            ImDrawList& drawList,
            const ActorDebugInfo& actor,
            const DebugOverlay& scene,
            const WindowViewport& viewport)
        {
            // Choose the upper-left coordinate vector matching either the visual sprite canvas or
            // the core collision box
            const glm::vec2 labelWorldPosition =
                actor.sprite.has_value() ? actor.sprite->bounds.topLeft : actor.collider.topLeft;
            ImVec2 labelPosition = screenPosition(labelWorldPosition, scene.cameraBounds, viewport);
            const float lineHeight = ImGui::GetTextLineHeight();

            // 1. Render primary entity classification tag (e.g., "Player" or "NPC [ID]")
            const std::string actorLabel = labelFor(actor);
            drawShadowedText(drawList, labelPosition, WorldLabelColour, actorLabel.c_str());

            // 2. Stack active script animation descriptors down onto the consecutive row slots
            if (actor.animation.has_value())
            {
                labelPosition.y += lineHeight;
                drawShadowedText(
                    drawList, labelPosition, WorldLabelColour, nameOf(actor.animation.value()));
            }

            // 3. Stack finite state engine tracking tokens onto the final row slots
            if (actor.npcState.has_value())
            {
                labelPosition.y += lineHeight;
                drawShadowedText(
                    drawList, labelPosition, WorldLabelColour, nameOf(actor.npcState.value()));
            }
        }

        // Outlines active projectile bounds fields and appends ownership identifiers alongside
        // destruction cooldowns
        void drawProjectile(
            ImDrawList& drawList,
            const ProjectileDebugInfo& projectile,
            const DebugOverlay& scene,
            const WindowViewport& viewport)
        {
            // Frame the physical spatial coordinates using standard wireframe rectangular
            // primitives
            drawWorldBounds(
                drawList, projectile.bounds, scene.cameraBounds, viewport, ProjectileColour);

            // Compute label layouts slightly below the lower horizontal border frame boundary
            ImVec2 labelPosition =
                screenPosition(projectile.bounds.topLeft, scene.cameraBounds, viewport);
            labelPosition.y += projectile.bounds.size.y * viewport.scale.y + 2.0F;

            char label[64]{};
            // Format a multi-line string containing the tracking criteria: [Owner ID] \n [Remaining
            // Lifespan Ticks]
            if (projectile.owner.has_value())
            {
                std::snprintf(
                    label,
                    sizeof(label),
                    "%u\n%.2f",
                    projectile.owner->value,
                    projectile.lifetimeRemaining);
            }
            else
            {
                std::snprintf(label, sizeof(label), "none\n%.2f", projectile.lifetimeRemaining);
            }
            drawShadowedText(drawList, labelPosition, ProjectileColour, label);
        }

        // Outlines the breakable cell under the cursor and names the key that breaks it.
        void drawBreakableCellHint(
            ImDrawList& drawList,
            const Aabb& cell,
            const DebugOverlay& scene,
            const WindowViewport& viewport)
        {
            // Render a crisp selection box wrapper targeting the explicit cell coordinate
            // boundaries
            drawWorldBounds(drawList, cell, scene.cameraBounds, viewport, WorldLabelColour);

            // Extrapolate a text positioning anchor floating safely one line height threshold
            // directly above the tile
            const ImVec2 above = screenPosition(cell.topLeft, scene.cameraBounds, viewport);
            drawShadowedText(
                drawList,
                {above.x, above.y - ImGui::GetTextLineHeight()},
                WorldLabelColour,
                "B to break"); // Notifies developer that debug cheat input key codes are active
        }

        // Formats detailed text blocks inside the developer sidebar overlay layout lists tracking
        // selected entities
        void drawActorText(ImDrawList& drawList, const ActorDebugInfo& actor, ImVec2& position)
        {
            constexpr float Indentation = 12.0F; // Text margin indentation alignment constant

            // 1. Output the main header label string containing the blueprint tracking identifier
            const std::string label = actorTextLabelFor(actor);
            drawTextLine(drawList, position, label.c_str(), TextHeadingColour);

            // 2. Log absolute world pixel space location metrics format parameters
            char text[96]{};
            std::snprintf(
                text,
                sizeof(text),
                "pos:    %.1f, %.1f",
                actor.collider.topLeft.x,
                actor.collider.topLeft.y);
            drawTextLine(drawList, position, text, TextDetailColour, Indentation);

            // 3. Log asset indices alongside original texture sheet location pixel boundaries
            if (actor.sprite.has_value())
            {
                std::snprintf(
                    text,
                    sizeof(text),
                    "frame:  %zu (%.0f, %.0f)",
                    actor.sprite->atlasFrame,
                    actor.sprite->atlasPosition.x,
                    actor.sprite->atlasPosition.y);
                drawTextLine(drawList, position, text, TextDetailColour, Indentation);
            }

            // 4. Log behavioral personality traits assigned to the targeting artificial
            // intelligence loops
            if (actor.npcTactic.has_value())
            {
                std::snprintf(text, sizeof(text), "tactic: %s", nameOf(actor.npcTactic.value()));
                drawTextLine(drawList, position, text, TextDetailColour, Indentation);
            }

            // Append a small padding gap spacer block before formatting the consecutive panel data
            // entries
            position.y += ActorTextGap;
        }

        // Actor and navigation text share a transparent, full-height panel at the right.
        // Its custom-drawn lines reserve matching ImGui content height, so the mouse wheel
        // can scroll a long list without showing a scrollbar.
        void drawDebugTextPanel(const DebugOverlay& scene)
        {
            // Evaluate tracking conditions to verify whether any readable telemetry content needs
            // to be rendered this frame
            const bool hasActorText = !scene.actors.empty();
            const bool hasNavigationText = scene.navigationConnections.has_value();
            if (!hasActorText && !hasNavigationText)
            {
                return; // Discard panel generation layout passes early if there is no text
                        // information to show
            }

            // 1. Fetch desktop viewport metrics and compute exact horizontal sizing allocations
            const ImGuiViewport* mainViewport = ImGui::GetMainViewport();
            const ImVec2 padding = ImGui::GetStyle().WindowPadding;
            const float windowWidth = DebugTextContentWidth + 2.0F * padding.x;

            // Lock the window anchor position directly flush against the upper-right edge of the
            // engine workspace area
            ImGui::SetNextWindowPos(
                {mainViewport->WorkPos.x + mainViewport->WorkSize.x - windowWidth,
                 mainViewport->WorkPos.y},
                ImGuiCond_Always);
            // Mandate a full-height structural panel configuration span matching the workspace
            // vertical dimensions
            ImGui::SetNextWindowSize({windowWidth, mainViewport->WorkSize.y}, ImGuiCond_Always);

            // 2. Initialize a borderless, transparent, non-scrollable overlay container interface
            if (ImGui::Begin(
                    "Debug text##overlay",
                    nullptr,
                    ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground |
                        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                        ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoScrollbar))
            {
                ImDrawList* drawList = ImGui::GetWindowDrawList();
                ImVec2 position = ImGui::GetCursorScreenPos(); // Cache the starting top anchor for
                                                               // custom draw tracking coordinates
                const float contentTop = position.y;

                // --- STAGE 3: EXECUTE CUSTOM TEXT RENDER PASSED LOGS ---
                // Sequentially write telemetry reports for all dynamic world objects detected this
                // frame loop step
                for (const ActorDebugInfo& actor : scene.actors)
                {
                    drawActorText(
                        *drawList, actor, position); // Increments position.y mutably under the hood
                }

                // Write out AI graph routing index configurations at the base of our text chain
                // loops
                if (hasNavigationText)
                {
                    drawNavigationProfile(*drawList, *scene.navigationConnections, position);
                }

                // --- STAGE 4: IMGUI LAYOUT CANVAS SCROLLING OVERRIDE HACK ---
                // We inject a virtual bounding placeholder box matching the exact size of our
                // low-level vector text drops. This tricks the core ImGui engine layout system into
                // capturing standard hardware input mouse wheel event deltas, allowing natural
                // panel scrolling mechanics to operate seamlessly without forcing ugly UI scrollbar
                // widgets onto the screen.
                ImGui::Dummy({DebugTextContentWidth, position.y - contentTop});
            }
            ImGui::End(); // Finalize current window draw state context references safely
        }
    }

    /**
     * Draw the debug information as an overlay on the viewport
     */
    void drawDebugOverlay(const DebugOverlay& scene, const std::optional<WindowViewport>& viewport)
    {
        // Route overlay metrics straight to the background pass layer to map directly over hardware
        // game pixels
        ImDrawList* drawList = ImGui::GetBackgroundDrawList();

        // --- STAGE 1: RENDER SYSTEM METRICS AND ACTIVE CAMERA TRACKING BOXES ---
        if (viewport.has_value())
        {
            // Frame the absolute perimeter borders representing what the structural camera lens
            // captures
            drawWorldBounds(
                *drawList, scene.cameraBounds, scene.cameraBounds, *viewport, CameraBoundsColour);

            // Draw out the inner framing dead-zone box inside which the camera will not move
            // tracking players
            drawWorldBounds(
                *drawList,
                scene.cameraDeadZone,
                scene.cameraBounds,
                *viewport,
                CameraDeadZoneColour);

            // Apply text marker identification strings directly above the dead-zone upper corner
            drawShadowedText(
                *drawList,
                screenPosition(scene.cameraDeadZone.topLeft, scene.cameraBounds, *viewport),
                CameraDeadZoneColour,
                "camera dead zone");

            // Draw real-time calculated diagnostics framerate integers onto screen grids margins
            // slots
            drawText(
                *drawList, 50, 100, std::to_string(scene.framerate).c_str(), TextHeadingColour);
        }

        // --- STAGE 2: ITERATE LOOPS AND MAP COMPREHENSIVE ON-SCREEN ENTITY PROPERTY LAYERS ---
        for (const ActorDebugInfo& actor : scene.actors)
        {
            // Terminate processing passes safely if layout coordinates lack explicit viewport
            // scales parameters
            if (!viewport.has_value())
            {
                continue;
            }

            // Unpack complex spatial sub-components step-by-step (patrol trails, sensory fields,
            // path nodes)
            drawActorPatrol(*drawList, actor, scene, *viewport);
            drawActorSensor(*drawList, actor, scene, *viewport);
            drawActorPath(*drawList, actor, scene, *viewport);

            // If immediate close-combat actions are actively processing, draw the physics attack
            // hitbox boundary box
            if (actor.biteHitbox.has_value())
            {
                drawWorldBounds(
                    *drawList,
                    actor.biteHitbox.value(),
                    scene.cameraBounds,
                    *viewport,
                    BiteHitboxColour);
            }

            // Outline visual texture region margins blocks to confirm alignment calibrations match
            // physics blocks
            if (actor.sprite.has_value())
            {
                drawWorldBounds(
                    *drawList,
                    actor.sprite->bounds,
                    scene.cameraBounds,
                    *viewport,
                    SpriteBoundsColour);
            }

            // Print status descriptions strings directly over top of actor models locations
            drawActorWorldLabel(*drawList, actor, scene, *viewport);

            // Highlight core physics collision framework blocks with distinct wired outlines
            drawWorldBounds(
                *drawList, actor.collider, scene.cameraBounds, *viewport, ColliderBoundsColour);
        }

        // --- STAGE 3: RENDERING LOGIC SECTIONS FOR LOOSE ASSETS AND COMPONENT GRAPH LAYOUTS ---
        if (viewport.has_value())
        {
            // Map out trajectory trails and lifetime parameters tracking active projectile items
            for (const ProjectileDebugInfo& projectile : scene.projectiles)
            {
                drawProjectile(*drawList, projectile, scene, *viewport);
            }

            // Trace the massive pathfinding grid connection nodes layout networks to evaluate graph
            // pathways
            if (scene.navigationConnections.has_value())
            {
                drawNavigationConnections(
                    *drawList,
                    scene.navigationConnections.value_or(NavigationConnectionsDebugInfo{}),
                    scene.cameraBounds,
                    *viewport);
            }

            // Map visual highlight selections frames isolating dynamic destructible cells targeted
            // under cursors
            if (scene.breakableCellUnderCursor.has_value())
            {
                drawBreakableCellHint(
                    *drawList, scene.breakableCellUnderCursor.value_or(Aabb{}), scene, *viewport);
            }

            // Outline loose collectible inventory drop elements and print matching definition names
            // descriptions
            for (const PickupDebugInfo& pickup : scene.pickups)
            {
                drawWorldBounds(
                    *drawList, pickup.bounds, scene.cameraBounds, *viewport, PickupColour);
                drawShadowedText(
                    *drawList,
                    screenPosition(pickup.bounds.topLeft, scene.cameraBounds, *viewport),
                    PickupColour,
                    pickup.itemName.c_str());
            }
        }

        // --- STAGE 4: PROCESS THE RIGHT-ALIGNED TEXT DATA PANEL VIEWPORT SIDEBARS ---
        drawDebugTextPanel(scene);
    }
}
