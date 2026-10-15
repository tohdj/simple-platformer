/**
 * @file        hud_catalog.cpp
 * @brief       Declares the HudIcons structure and data pipeline functions for loading, 
 *              parsing, and validating Heads-Up Display UI graphic sprite regions.
 *
 * @author      Mohd Fadzuli
 * @author      Adapted by: Toh Da Jun
 * @copyright   Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#include "hud_catalog.hpp"
#include "content_diagnostics.hpp"
#include "content_json.hpp"
#include "content_validation.hpp"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <string_view>
#include "simple_platformer/math/validation.hpp"
#include "simple_platformer/render/sprite.hpp"

namespace simple_platformer
{
    namespace
    {
        // Validates an isolated UI icon layout context profile to check if coordinates are
        // non-negative and sizes are strictly positive
        void validateHudIcon(const SpriteRegion& region, std::string_view name)
        {
            if (!isFiniteNonNegative(region.position) || !isFinitePositive(region.size))
            {
                // Generate detailed failure exceptions specifying the erroneous sub-node key
                // variable parameters
                failJson(
                    {}, name, "expected a finite, non-negative atlas position and a positive size");
            }
        }

        // Extracts, formats, and checks structural position and sizing sub-fields out of individual
        // JSON sub-objects
        SpriteRegion jsonHudIcon(
            const nlohmann::json& root,
            std::string_view name,
            std::string_view sourceName)
        {
            const auto& value = requiredJsonMember(root, name, sourceName, "root");
            // Enforce explicit structural requirements checks parameters for the target UI
            // configuration nodes
            checkJsonFields(value, {"position", "size"}, sourceName, name);
            return jsonSpriteRegion(value, sourceName, name);
        }
    }

    // High-level coordinate matrix integrity check block verifying all standard runtime HUD
    // interface configuration icons profiles
    void validateHudIcons(const HudIcons& icons)
    {
        validateHudIcon(
            icons.fullHeart,
            "fullHeart"); // Dynamic health pool active layer indicator graphics block
        validateHudIcon(
            icons.emptyHeart,
            "emptyHeart"); // Dynamic health pool baseline backdrop background graphics block
        validateHudIcon(
            icons.bag, "bag"); // Persistent inventory menu toggle anchor button graphics block
    }

    // Parses a raw string text data block structure down into a validated binary interface hud
    // structural profile struct
    HudIcons parseHudIcons(std::string_view text, std::string_view sourceName)
    {
        // 1. Initialize structural JSON token parsing layouts out of the input text streams
        const auto root = parseContentRoot(text, sourceName);
        checkJsonFields(root, {"fullHeart", "emptyHeart", "bag"}, sourceName, "root");

        HudIcons icons;

        // 2. Map structural values into flat memory properties containers
        icons.fullHeart = jsonHudIcon(root, "fullHeart", sourceName);
        icons.emptyHeart = jsonHudIcon(root, "emptyHeart", sourceName);
        icons.bag = jsonHudIcon(root, "bag", sourceName);

        // Conclude setup steps by applying post-construction validation data verification loops
        validateInFile(sourceName, [&] { validateHudIcons(icons); });
        return icons;
    }

    // Handles drive reading IO operations and directs content text directly downward to text
    // parsing units
    HudIcons loadHudIcons(const std::filesystem::path& path)
    {
        return parseHudIcons(loadContentText(path), path.string());
    }

    // Assures that HUD element sub-rect layout settings stay completely enclosed within the limits
    // of the texture atlas sheet dimensions
    void validateHudAtlasRegions(
        const HudIcons& icons,
        glm::ivec2 atlasSize,
        std::string_view sourceName)
    {
        // Enforce boundary verification constants rules to block sprite clipping errors early
        requireInAtlas(icons.fullHeart, atlasSize, sourceName, "fullHeart");
        requireInAtlas(icons.emptyHeart, atlasSize, sourceName, "emptyHeart");
        requireInAtlas(icons.bag, atlasSize, sourceName, "bag");
    }
}
