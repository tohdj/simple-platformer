/**
 * @file        AnimationCatalog.cpp
 * @brief       Declares the AnimationCatalog type alias and functions for loading, 
 *              parsing, and validating character and effect animation sets.
 *
 * @author      Mohd Fadzuli
 * @author      Adapted by: Toh Da Jun
 * @copyright   Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#include "animation_catalog.hpp"
#include "content_diagnostics.hpp"
#include "content_json.hpp"
#include "content_validation.hpp"
#include <array>
#include <cstddef>
#include <filesystem>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include <nlohmann/json.hpp>
#include "simple_platformer/math/validation.hpp"
#include "simple_platformer/render/animation.hpp"
#include "simple_platformer/render/sprite.hpp"

namespace simple_platformer
{
    namespace
    {
        // Mapping node pairing text identifiers found in JSON datasets to internal engine animation
        // enum tokens
        struct ClipEntry
        {
            std::string_view name;
            AnimationName type;
        };

        // Static immutable compilation database table containing all required character movement
        // clip sequences
        constexpr std::array<ClipEntry, 6> Clips = {
            {{"idle", AnimationName::Idle},     // Static rest stance
             {"move", AnimationName::Move},     // Lateral ground locomotion
             {"jump", AnimationName::Jump},     // Vertical upward kinetic trace
             {"fall", AnimationName::Fall},     // Gravitational aerial transition drop
             {"attack", AnimationName::Attack}, // Triggering aggressive action frames
             {"death", AnimationName::Death}}}; // Lifecycle termination sequence

        // The clip's field name in animations.json, as in "idle".
        std::string_view clipName(AnimationName name)
        {
            // Query database rows to resolve the localized naming string matching the internal enum
            // state configuration
            for (const ClipEntry& entry : Clips)
            {
                if (entry.type == name)
                {
                    return entry.name;
                }
            }
            throw std::logic_error("An animation clip has no catalog name");
        }

        // Extracts a sequential array containing the string keys representing the standard
        // blueprint animations schema
        std::vector<std::string_view> clipNames()
        {
            std::vector<std::string_view> names;
            names.reserve(Clips.size()); // Pre-allocate vector nodes storage bounds to bypass
                                         // fragmentation footprint triggers
            for (const ClipEntry& entry : Clips)
            {
                names.push_back(entry.name);
            }
            return names;
        }
    }

    // Runs a comprehensive verification sweep to assert that an animation set satisfies schema
    // requirements and texture invariants
    void validateAnimationSet(const AnimationSet& set)
    {
        std::set<AnimationName> names;

        // --- STAGE 1: CHRONOLOGICAL ENTRY ITERATION AND FIELD BOUNDS TESTING ---
        for (const auto& clip : set.clips)
        {
            std::string name;
            for (const ClipEntry& entry : Clips)
            {
                if (clip.name == entry.type)
                {
                    name = entry.name;
                    break;
                }
            }
            if (name.empty())
            {
                throw std::invalid_argument("unknown animation clip");
            }
            // Enforce identity uniqueness: flag duplicates trying to parse the same category action
            // sequence twice
            if (!names.insert(clip.name).second)
            {
                failJson({}, name, "duplicate animation clip");
            }
            if (clip.frames.empty())
            {
                failJson({}, fieldPath(name, "frames"), "expected at least one frame");
            }
            // Confirm the frame timing intervals evaluate to valid, positive, non-infinite time
            // intervals
            if (!isFinitePositive(clip.frameDuration))
            {
                failJson({}, fieldPath(name, "frameDuration"), "expected a positive finite number");
            }

            // --- STAGE 2: MULTI-FRAME COHESION AND RECTANGULAR GEOMETRY LOCKING PASSES ---
            for (std::size_t index = 0; index < clip.frames.size(); ++index)
            {
                try
                {
                    const auto& frame = clip.frames[index];
                    Sprite sprite;
                    sprite.region = frame;
                    validateContentSprite(sprite); // Execute low-level coordinates bounds checks

                    // Critical Invariant: Every text frame layout inside a single set block must
                    // share exact pixel sizing ratios. This prevents the character's layout shape
                    // from jittering or shifting relative to physics bounds during flips.
                    if (frame.size != set.clips.front().frames.front().size)
                    {
                        throw std::invalid_argument("all frames in a set must use the same size");
                    }
                }
                catch (const std::invalid_argument& error)
                {
                    failJson({}, indexPath(fieldPath(name, "frames"), index), error.what());
                }
            }
        }

        // --- STAGE 3: REQUIREMENT CHECKS - Guarantee completeness of standard entity actions ---
        for (const ClipEntry& entry : Clips)
        {
            // Trigger failure arguments if any baseline action module drops off from the
            // deserialised tracking structure
            if (names.count(entry.type) == 0)
            {
                throw std::invalid_argument(std::string(entry.name) + ": required clip is missing");
            }
        }
    }

    // Runs an integrity pass across the animation catalog to intercept empty names and ensure every
    // animation set is valid
    void validateAnimationCatalog(const AnimationCatalog& catalog)
    {
        for (const auto& entry : catalog)
        {
            try
            {
                // Ensure the map dictionary look-up string key identifier isn't blank
                if (entry.first.empty())
                {
                    throw std::invalid_argument("animation set name cannot be empty");
                }
                validateAnimationSet(entry.second);
            }
            catch (const std::invalid_argument& error)
            {
                // Format a nested JSON diagnostic path trace specifying the broken animation key
                // node context
                failJson({}, fieldPath("animations", entry.first), error.what());
            }
        }
    }

    // Parses a raw text config stream down into an operational binary AnimationCatalog database map
    AnimationCatalog parseAnimationCatalog(std::string_view text, std::string_view sourceName)
    {
        // 1. Initialize structural JSON token parsing out of the text stream
        const auto root = parseContentRoot(text, sourceName);
        checkJsonFields(root, {"animations"}, sourceName, "root");
        const auto& definitions = requiredJsonMember(root, "animations", sourceName, "root");
        checkJsonObject(definitions, sourceName, "animations");

        AnimationCatalog catalog;

        // 2. Loop through all high-level animation sets (e.g., "player", "slime_enemy")
        for (const auto& entry : definitions.items())
        {
            const std::string setPath = fieldPath("animations", entry.key());
            // Enforce that only valid clip names (idle, move, etc.) are declared inside this
            // character set
            checkJsonFields(entry.value(), clipNames(), sourceName, setPath);
            AnimationSet set;

            // 3. Nested loop: Extract each mandatory animation type out of the current set
            // definition
            for (const ClipEntry& definition : Clips)
            {
                const std::string name(definition.name);
                const std::string clipPath = fieldPath(setPath, name);
                const auto& value = requiredJsonMember(entry.value(), name, sourceName, setPath);

                // Enforce structural schema requirements for the specific animation clip block
                checkJsonFields(
                    value, {"frames", "frameDuration", "looping"}, sourceName, clipPath);

                AnimationClip clip;
                clip.name = definition.type;
                clip.frameDuration = readNumber(value, "frameDuration", sourceName, clipPath);
                clip.looping = readBoolean(value, "looping", sourceName, clipPath);

                const auto& frames = requiredJsonMember(value, "frames", sourceName, clipPath);
                if (!frames.is_array())
                {
                    failJson(sourceName, fieldPath(clipPath, "frames"), "expected an array");
                }

                // 4. Fine-grain extraction loop: Pull texture coordinate regions out of the frames
                // array
                for (std::size_t frame = 0; frame < frames.size(); ++frame)
                {
                    const std::string framePath = indexPath(fieldPath(clipPath, "frames"), frame);
                    checkJsonFields(frames[frame], {"position", "size"}, sourceName, framePath);

                    // Extract position and size vectors to establish the 2D bounding layout region
                    // of the sprite frame
                    clip.frames.push_back(jsonSpriteRegion(frames[frame], sourceName, framePath));
                }
                set.clips.push_back(clip);
            }
            catalog.emplace(entry.key(), set);
        }

        // 5. Conclude compilation by executing a post-construction data integrity validation sweep
        validateInFile(sourceName, [&] { validateAnimationCatalog(catalog); });
        return catalog;
    }

    // Handles file text reading disk input-output queries and streams strings downstream to target
    // text compiling nodes
    AnimationCatalog loadAnimationCatalog(const std::filesystem::path& path)
    {
        return parseAnimationCatalog(loadContentText(path), path.string());
    }

    // Safe bounds-checked search utility extracting animation data references matching a query
    // token name
    const AnimationSet& animationSet(const AnimationCatalog& catalog, const std::string& name)
    {
        const auto found = catalog.find(name);
        if (found == catalog.end())
        {
            throw std::invalid_argument("unknown animation set '" + name + "'");
        }
        return found->second;
    }

    // Validates clip metrics boundary domains to verify every animation frame resides completely
    // within physical atlas sizes
    void validateAnimationAtlasRegions(
        const AnimationCatalog& catalog,
        glm::ivec2 atlasSize,
        std::string_view sourceName)
    {
        // Recursively unpack the catalog hierarchy down to individual 2D frame regions to detect
        // coordinate overflows
        for (const auto& [name, set] : catalog)
        {
            for (const AnimationClip& clip : set.clips)
            {
                const std::string frames = fieldPath(
                    fieldPath(fieldPath("animations", name), clipName(clip.name)), "frames");

                for (std::size_t index = 0; index < clip.frames.size(); ++index)
                {
                    // Assert that the texture sub-rect fits completely inside physical graphics
                    // limits
                    requireInAtlas(
                        clip.frames[index], atlasSize, sourceName, indexPath(frames, index));
                }
            }
        }
    }
}
