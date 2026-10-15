/**
 * @file        interface_ui.cpp
 * @brief       Declares user interface rendering functions and request structures
 *              for drawing HUD elements and managing inventory overlay interactions.
 *
 * @author      Mohd Fadzuli
 * @author      Adapted by: Toh Da Jun
 * @copyright   Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#include "interface_ui.hpp"

#include <cstddef>
#include <cstdio>
#include <optional>

#include <glm/vec2.hpp>
#include <imgui.h>

#include "content/hud_catalog.hpp"
#include "game/game.hpp"
#include "graphics/display_viewport.hpp"
#include "graphics/sprite_renderer.hpp"
#include "simple_platformer/actor/actor.hpp"
#include "simple_platformer/inventory/inventory.hpp"
#include "simple_platformer/inventory/item.hpp"
#include "simple_platformer/math/coordinates.hpp"
#include "simple_platformer/render/sprite.hpp"
#include "ui/hud_draw.hpp"
#include "ui/hud_layout.hpp"
#include "ui/inventory_layout.hpp"

namespace simple_platformer
{
    namespace
    {
        // Renders the health heart indicators along the top-left of the viewport display
        void drawHealthHud(
            const Health& health,
            const HudIcons& icons,
            const Texture& atlas,
            const WindowViewport& viewport)
        {
            // Direct background draw list bypasses ImGui window structures for low-overhead drawing
            ImDrawList* drawList = ImGui::GetBackgroundDrawList();

            // Scaled dimensional bounds mapped according to device display densities
            const ImVec2 size = {HudIconSize * viewport.scale.x, HudIconSize * viewport.scale.y};
            ImVec2 position = {
                viewport.topLeft.x + HudMargin * viewport.scale.x,
                viewport.topLeft.y + HudMargin * viewport.scale.y};

            // Loop up to max structural thresholds and fill full vs empty hearts sequentially
            for (int heart = 0; heart < health.maximum; ++heart)
            {
                drawAtlasRegion(
                    *drawList,
                    atlas,
                    heart < health.current ? icons.fullHeart : icons.emptyHeart,
                    position,
                    {position.x + size.x, position.y + size.y});

                // Shift drawing anchor forward horizontally for the next icon step
                position.x += (HudIconSize + HudGap) * viewport.scale.x;
            }
        }

        // Renders the persistent bottom-left hotbar toggle icon for accessing inventory controls
        bool drawInventoryButton(
            const Texture& atlas,
            const SpriteRegion& bagIcon,
            const WindowViewport& viewport)
        {
            const ImVec2 size{HudIconSize * viewport.scale.x, HudIconSize * viewport.scale.y};

            // Positions the controller window exactly at the lower-left aspect relative to internal
            // height limits
            const ImVec2 position{
                viewport.topLeft.x + HudMargin * viewport.scale.x,
                viewport.topLeft.y +
                    (static_cast<float>(InternalHeight) - HudMargin - HudIconSize) *
                        viewport.scale.y};

            ImGui::SetNextWindowPos(position, ImGuiCond_Always);
            ImGui::SetNextWindowSize(size, ImGuiCond_Always);

            // Tighten window frame padding strictly to maximize active click thresholds
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0.0F, 0.0F});
            ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, {0.0F, 0.0F});

            bool clicked = false;
            if (ImGui::Begin(
                    "Inventory bag##hud",
                    nullptr,
                    ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground |
                        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                        ImGuiWindowFlags_NoNav))
            {
                ImGui::InvisibleButton("Open inventory", size);
                const bool hovered = ImGui::IsItemHovered();

                // Act on press, before gameplay consumes the same mouse-button edge.
                clicked = hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left);

                drawAtlasRegion(
                    *ImGui::GetWindowDrawList(),
                    atlas,
                    bagIcon,
                    position,
                    {position.x + size.x, position.y + size.y});
            }
            ImGui::End();
            ImGui::PopStyleVar(2); // Balance padding layout structures
            return clicked;
        }

        // Projects a floating contextual hint icon above the level portal door if key demands
        // aren't satisfied
        void drawLockedExitHint(
            const Game& game,
            const Texture& atlas,
            const WindowViewport& viewport)
        {
            const std::optional<Sprite> icon = game.lockedExitHintIcon();
            const std::optional<glm::vec2> doorTopCenter = game.levelExitScreenPosition();
            if (!icon.has_value() || !doorTopCenter.has_value())
            {
                return;
            }

            // Convert world-space gate coordinate metrics safely down into current local pixel
            // regions
            const ImVec2 topLeft = {
                viewport.topLeft.x +
                    (doorTopCenter->x - icon->region.size.x * 0.5F) * viewport.scale.x,
                viewport.topLeft.y +
                    (doorTopCenter->y - GapAboveDoor - icon->region.size.y) * viewport.scale.y};
            const ImVec2 bottomRight = {
                topLeft.x + icon->region.size.x * viewport.scale.x,
                topLeft.y + icon->region.size.y * viewport.scale.y};

            drawAtlasRegion(
                *ImGui::GetBackgroundDrawList(), atlas, icon->region, topLeft, bottomRight);
        }

        // Renders text directly onto the viewport showing restart commands upon completing a scene
        void drawLevelCompletion(const Game& game, const WindowViewport& viewport)
        {
            if (!game.complete())
            {
                return;
            }
            const std::optional<glm::vec2> exitPosition = game.levelExitScreenPosition();
            if (!exitPosition.has_value())
            {
                return;
            }

            const ImVec2 doorTopCenter = {
                viewport.topLeft.x + exitPosition->x * viewport.scale.x,
                viewport.topLeft.y + exitPosition->y * viewport.scale.y};
            const float lineHeight = ImGui::GetTextLineHeight();

            // Adjust label alignments slightly above standard architectural assets on display
            const float firstLineY =
                doorTopCenter.y - lineHeight * 3.0F - GapAboveDoor * viewport.scale.y;

            ImDrawList* drawList = ImGui::GetForegroundDrawList();
            drawCenteredText(*drawList, doorTopCenter.x, firstLineY, "Completed");
            drawCenteredText(*drawList, doorTopCenter.x, firstLineY + lineHeight, "Press R to");
            drawCenteredText(*drawList, doorTopCenter.x, firstLineY + lineHeight * 2.0F, "restart");
        }

        // Inventory grid layout, in internal pixels before the viewport scales them.
        constexpr float SlotSize = 20.0F;
        constexpr float IconPadding = 2.0F;
        constexpr float GridPadding = 2.0F;

        // Custom palette color maps for building interactive slot frame components
        constexpr ImU32 SlotColour = IM_COL32(40, 44, 52, 220);
        constexpr ImU32 HoveredSlotColour = IM_COL32(72, 76, 84, 240);
        constexpr ImU32 SlotBorderColour = IM_COL32(150, 156, 168, 220);

        // Draws one slot and returns whether its consumable was clicked.
        bool drawInventorySlot(
            const Game& game,
            const Texture& atlas,
            const std::optional<ItemStack>& slot,
            float slotSize,
            float iconPadding)
        {
            const bool clicked = ImGui::InvisibleButton("slot", {slotSize, slotSize});
            const ImVec2 slotMinimum = ImGui::GetItemRectMin();
            const ImVec2 slotMaximum = ImGui::GetItemRectMax();
            ImDrawList* drawList = ImGui::GetWindowDrawList();

            // Alter backdrop color dynamically based on focus context
            const ImU32 background = ImGui::IsItemHovered() ? HoveredSlotColour : SlotColour;
            drawList->AddRectFilled(slotMinimum, slotMaximum, background);
            drawList->AddRect(slotMinimum, slotMaximum, SlotBorderColour, 0.0F, 0, 1.0F);

            if (!slot.has_value())
            {
                return false; // Slot empty; drop processing early
            }

            // Extract item metadata parameters out of the core definitions catalog
            const ItemDefinition& item = game.itemDefinition(slot->item);
            const ImVec2 iconMinimum = {slotMinimum.x + iconPadding, slotMinimum.y + iconPadding};
            const ImVec2 iconMaximum = {slotMaximum.x - iconPadding, slotMaximum.y - iconPadding};
            drawAtlasRegion(*drawList, atlas, item.icon.region, iconMinimum, iconMaximum);

            // Overlay the item quantity counter label inside the bottom edge of the frame context
            char count[16];
            std::snprintf(count, sizeof(count), "%d", slot->quantity);
            drawShadowedText(*drawList, iconMinimum, HudTextColour, count);

            // Only capture activations if the underlying item has functional properties
            return clicked && item.effect != ItemEffect::None;
        }

        // Orchestrates the multi-column layout grid wrapper matrix window for item navigation
        std::optional<std::size_t> drawInventory(
            const Game& game,
            const Texture& atlas,
            const WindowViewport& viewport)
        {
            const auto& slots = game.playerInventory().slots();
            const InventoryGridLayout layout = makeInventoryGridLayout(slots.size());

            // Normalize layout boundaries dynamically alongside scale shifts
            const float slotSize = SlotSize * viewport.scale.x;
            // Scale the icon's internal padding according to the viewport's horizontal scale factor
            const float iconPadding = IconPadding * viewport.scale.x;
            // Scale the window grid's outer and structural padding based on the horizontal layout scale
            const float windowPadding = GridPadding * viewport.scale.x;

            // Calculate the total required window size based on item slots, column/row sizes, and
            // padding gaps
            const ImVec2 windowSize = {
                slotSize * static_cast<float>(layout.columns) +
                    windowPadding * static_cast<float>(layout.columns + 1),
                slotSize * static_cast<float>(layout.rows) +
                    windowPadding * static_cast<float>(layout.rows + 1)};

            // Compute the absolute bottom-left position anchor for the window context within the
            // application viewport
            const ImVec2 inventoryBottomLeft = {
                viewport.topLeft.x + HudMargin * viewport.scale.x,
                viewport.topLeft.y +
                    (static_cast<float>(InternalHeight) - HudMargin - HudIconSize - HudGap) *
                        viewport.scale.y};

            // Optional container to store the index of the inventory slot selected by the player
            // this frame
            std::optional<std::size_t> slotToUse;

            // Enforce the computed layout position on the next window initialization using a
            // bottom-left pivot point (0,1)
            ImGui::SetNextWindowPos(inventoryBottomLeft, ImGuiCond_Always, {0.0F, 1.0F});
            // Enforce the calculated bounding size restrictions on the next window instance pass
            ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);

            // Push layout variables to override the internal window border space and inter-item
            // cell offsets
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {windowPadding, windowPadding});
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {windowPadding, windowPadding});

            // Initialize the flat background grid surface panel container
            if (ImGui::Begin(
                    "Inventory##grid",
                    nullptr,
                    ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                        ImGuiWindowFlags_NoSavedSettings))
            {
                // Iterate through every active data stack currently allocated inside the player
                // inventory matrix
                for (std::size_t index = 0; index < slots.size(); ++index)
                {
                    // Scope individual slot loop loops to prevent unique ID collision patterns
                    // across the engine pipeline
                    ImGui::PushID(static_cast<int>(index));

                    // Render the individual grid slot box element; registers user click actions if
                    // interactive effects exist
                    if (drawInventorySlot(game, atlas, slots[index], slotSize, iconPadding))
                    {
                        slotToUse =
                            index; // Cache selected item tracker indices for processing passes
                    }

                    // Push the next item onto the same visual horizontal row unless a layout column
                    // boundary is broken
                    if ((index + 1) % layout.columns != 0 && index + 1 < slots.size())
                    {
                        ImGui::SameLine();
                    }
                    ImGui::PopID(); // Clean up unique token identifier scope rules
                }
            }
            ImGui::End();          // Finalize current window draw batch state context
            ImGui::PopStyleVar(2); // Reset the window layout and spacing modifications
            return slotToUse; // Pass the active interaction response back up the tracking chain
        }
    }

    // Main layout execution junction handling structural hud configurations across execution loops
    InterfaceRequests drawInterface(
        const Game& game,
        const Texture& atlas,
        const std::optional<WindowViewport>& viewport,
        bool inventoryOpen)
    {
        InterfaceRequests requests;
        // Terminate calculations early if the window environment is missing or lacks structural
        // attributes
        if (!viewport.has_value())
        {
            return requests;
        }

        // 1. Render the top-left player health bar overlay configurations
        drawHealthHud(game.playerHealth(), game.hudIcons(), atlas, *viewport);

        // 2. Render normal play state menu bars and map layout trackers while gameplay is active
        if (!game.complete())
        {
            // Update active state triggers for interaction overlays
            requests.toggleInventory = drawInventoryButton(atlas, game.hudIcons().bag, *viewport);
            drawLockedExitHint(game, atlas, *viewport);
        }

        // 3. Process game victory state screen alerts or handle modal submenus when active
        drawLevelCompletion(game, *viewport);
        if (inventoryOpen && !game.complete())
        {
            requests.useInventorySlot = drawInventory(game, atlas, *viewport);
        }

        return requests;
    }
}
