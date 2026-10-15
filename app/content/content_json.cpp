/**
 * @file        content_json.cpp
 * @brief       Declares strict JSON validation and type extraction functions for
 *              loading engine data types, shapes, and sprite elements safely.
 *
 * @author      Mohd Fadzuli
 * @author      Adapted by: Toh Da Jun
 * @copyright   Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#include "content_json.hpp"
#include "content_diagnostics.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include <nlohmann/json.hpp>
#include <glm/vec2.hpp>
#include "simple_platformer/render/sprite.hpp"

namespace simple_platformer
{
    // Asserts that a parsed JSON value structure represents a valid map dictionary object node
    void checkJsonObject(
        const nlohmann::json& value,
        std::string_view sourceName,
        std::string_view path)
    {
        if (!value.is_object())
        {
            failJson(sourceName, path, "expected an object");
        }
    }

    // Scans key names inside a dictionary block to intercept and block illegal or unknown schema
    // keys parameters
    void checkJsonFields(
        const nlohmann::json& value,
        const std::vector<std::string_view>& allowed,
        std::string_view sourceName,
        std::string_view path)
    {
        checkJsonObject(
            value, sourceName, path); // Enforce object structural requirements baseline criteria

        // Loop and parse every key element present inside the active node data block
        for (const auto& entry : value.items())
        {
            // Trigger validation failures if the current token key name is not found in the allowed
            // catalog listings
            if (std::find(allowed.begin(), allowed.end(), entry.key()) == allowed.end())
            {
                failJson(sourceName, path, "unknown field '" + entry.key() + "'");
            }
        }
    }

    // Safely reads a pointer address targeting a member node key, returning a null pointer if the
    // key is missing
    const nlohmann::json* optionalJsonMember(
        const nlohmann::json& object,
        std::string_view key,
        std::string_view sourceName,
        std::string_view path)
    {
        checkJsonObject(object, sourceName, path);
        const auto found = object.find(std::string(key));
        return found == object.end() ? nullptr : &*found;
    }

    // Extracts a core required field element, deliberately throwing a validation argument error
    // path if the key drops off
    const nlohmann::json& requiredJsonMember(
        const nlohmann::json& object,
        std::string_view key,
        std::string_view sourceName,
        std::string_view path)
    {
        const nlohmann::json* found = optionalJsonMember(object, key, sourceName, path);
        if (found == nullptr)
        {
            failJson(sourceName, path, "missing '" + std::string(key) + "'");
        }
        return *found;
    }

    // Extracts numeric values parameters and enforces strict 32-bit hardware boundary limits tests
    // configurations
    int jsonInteger(const nlohmann::json& value, std::string_view sourceName, std::string_view path)
    {
        if (!value.is_number_integer())
        {
            failJson(sourceName, path, "expected an integer");
        }
        // Catch variables that overflow beyond native machine limits thresholds configurations
        if (value < std::numeric_limits<int>::min() || value > std::numeric_limits<int>::max())
        {
            failJson(sourceName, path, "integer is outside the supported range");
        }
        return value.get<int>();
    }

    // Extracts raw floating-point metrics numbers and intercepts illegal mathematical
    // representations (NaN/Infinity)
    float jsonNumber(
        const nlohmann::json& value,
        std::string_view sourceName,
        std::string_view path)
    {
        if (!value.is_number())
        {
            failJson(sourceName, path, "expected a number");
        }
        const float result = value.get<float>();
        // Intercept corrupting non-finite anomalies paths before they contaminate core transform
        // calculations vectors
        if (!std::isfinite(result))
        {
            failJson(sourceName, path, "number must be finite");
        }
        return result;
    }

    // Extracts a primitive boolean flag and validates that the underlying token type is strictly
    // true or false
    bool jsonBoolean(
        const nlohmann::json& value,
        std::string_view sourceName,
        std::string_view path)
    {
        if (!value.is_boolean())
        {
            failJson(sourceName, path, "expected true or false");
        }
        return value.get<bool>();
    }

    // Extracts a raw string block and guarantees that the underlying JSON data value type
    // represents a string literal
    std::string jsonText(
        const nlohmann::json& value,
        std::string_view sourceName,
        std::string_view path)
    {
        if (!value.is_string())
        {
            failJson(sourceName, path, "expected text");
        }
        return value.get<std::string>();
    }

    // Parses a configuration asset text key name while throwing validation errors if the string
    // token is completely blank
    std::string jsonName(
        const nlohmann::json& value,
        std::string_view description,
        std::string_view sourceName,
        std::string_view path)
    {
        std::string name = jsonText(value, sourceName, path);
        if (name.empty())
        {
            failJson(sourceName, path, std::string(description) + " cannot be empty");
        }
        return name;
    }

    // Helper wrapper function that grabs a required JSON member node address and parses its
    // contents as a non-empty key name identifier
    std::string readName(
        const nlohmann::json& object,
        std::string_view key,
        std::string_view description,
        std::string_view sourceName,
        std::string_view path)
    {
        return jsonName(
            requiredJsonMember(object, key, sourceName, path),
            description,
            sourceName,
            fieldPath(path, key));
    }

    // Structural constraint check ensuring a target JSON block represents an array node holding
    // exactly two distinct values
    void checkJsonPair(
        const nlohmann::json& value,
        std::string_view shape,
        std::string_view sourceName,
        std::string_view path)
    {
        if (!value.is_array() || value.size() != 2)
        {
            failJson(sourceName, path, std::string("expected ") + std::string(shape));
        }
    }

    // Parses a 2-element layout coordinates array block directly down into a modern GLM
    // two-dimensional floating-point math vector
    glm::vec2 jsonVector(
        const nlohmann::json& value,
        std::string_view sourceName,
        std::string_view path)
    {
        // Enforce basic structural matrix layout pair boundary rules mappings ([X, Y])
        checkJsonPair(value, "[x, y]", sourceName, path);
        return {
            jsonNumber(
                value[0],
                sourceName,
                indexPath(path, 0)), // Map element index 0 to the horizontal X dimension component
            jsonNumber(
                value[1],
                sourceName,
                indexPath(path, 1)) // Map element index 1 to the vertical Y dimension component
        };
    }

    // High-level extraction helper that resolves a mandatory dictionary key node and returns its
    // contents parsed as a safe integer boundaries range
    int readInteger(
        const nlohmann::json& object,
        std::string_view key,
        std::string_view sourceName,
        std::string_view path)
    {
        return jsonInteger(
            requiredJsonMember(object, key, sourceName, path), sourceName, fieldPath(path, key));
    }

    // Extracts an optional numerical value setting parameters out of the dictionary block, safely
    // keeping existing defaults intact if the key is missing
    void readOptionalInteger(
        const nlohmann::json& object,
        std::string_view key,
        int& result, // Passed by mutable reference to preserve the fallback default setting when
                     // parsing checks drop out
        std::string_view sourceName,
        std::string_view path)
    {
        // Check if the underlying member configuration key pointer address resolves successfully
        // inside active scopes
        if (const nlohmann::json* found = optionalJsonMember(object, key, sourceName, path))
        {
            result = jsonInteger(*found, sourceName, fieldPath(path, key));
        }
    }

    // Extracts a mandatory numerical scalar element from a specific dictionary member key node path
    float readNumber(
        const nlohmann::json& object,
        std::string_view key,
        std::string_view sourceName,
        std::string_view path)
    {
        return jsonNumber(
            requiredJsonMember(object, key, sourceName, path), sourceName, fieldPath(path, key));
    }

    // Safely reads a numeric scalar parameter, preserving existing defaults intact if the key node
    // doesn't exist
    void readOptionalNumber(
        const nlohmann::json& object,
        std::string_view key,
        float&
            result, // Passed by reference to maintain original defaults when fallback traces occur
        std::string_view sourceName,
        std::string_view path)
    {
        // Check if the underlying member configuration key pointer address resolves successfully
        // inside active scopes
        if (const nlohmann::json* found = optionalJsonMember(object, key, sourceName, path))
        {
            result = jsonNumber(*found, sourceName, fieldPath(path, key));
        }
    }

    // Extracts a mandatory boolean configuration flag, throwing a validation argument error path if
    // missing
    bool readBoolean(
        const nlohmann::json& object,
        std::string_view key,
        std::string_view sourceName,
        std::string_view path)
    {
        return jsonBoolean(
            requiredJsonMember(object, key, sourceName, path), sourceName, fieldPath(path, key));
    }

    // Extracts an optional boolean state toggle, keeping default structural states active upon
    // dictionary lookup drops
    void readOptionalBoolean(
        const nlohmann::json& object,
        std::string_view key,
        bool& result,
        std::string_view sourceName,
        std::string_view path)
    {
        if (const nlohmann::json* found = optionalJsonMember(object, key, sourceName, path))
        {
            result = jsonBoolean(*found, sourceName, fieldPath(path, key));
        }
    }

    // Extracts a mandatory text block character string stream from the targeted required file path
    std::string readText(
        const nlohmann::json& object,
        std::string_view key,
        std::string_view sourceName,
        std::string_view path)
    {
        return jsonText(
            requiredJsonMember(object, key, sourceName, path), sourceName, fieldPath(path, key));
    }

    // Extracts an optional string configuration label, leaving fallback system text string
    // variables unchanged if unassigned
    void readOptionalText(
        const nlohmann::json& object,
        std::string_view key,
        std::string& result,
        std::string_view sourceName,
        std::string_view path)
    {
        if (const nlohmann::json* found = optionalJsonMember(object, key, sourceName, path))
        {
            result = jsonText(*found, sourceName, fieldPath(path, key));
        }
    }

    // Parses a mandatory 2D geometric array block structure directly into a standard
    // two-dimensional math vector
    glm::vec2 readVector(
        const nlohmann::json& object,
        std::string_view key,
        std::string_view sourceName,
        std::string_view path)
    {
        return jsonVector(
            requiredJsonMember(object, key, sourceName, path), sourceName, fieldPath(path, key));
    }

    // Extracts an optional 2D dimension vector parameter, protecting base coordinate defaults upon
    // schema validation checks drops
    void readOptionalVector(
        const nlohmann::json& object,
        std::string_view key,
        glm::vec2& result,
        std::string_view sourceName,
        std::string_view path)
    {
        if (const nlohmann::json* found = optionalJsonMember(object, key, sourceName, path))
        {
            result = jsonVector(*found, sourceName, fieldPath(path, key));
        }
    }

    // Deserialises raw structural attributes to construct a standalone 2D sprite boundaries
    // coordinate framing region
    SpriteRegion jsonSpriteRegion(
        const nlohmann::json& value,
        std::string_view sourceName,
        std::string_view path)
    {
        SpriteRegion region;
        // Map the multi-dimensional vector elements representing positioning frames and sizing
        // domains
        region.position = readVector(value, "position", sourceName, path);
        region.size = readVector(value, "size", sourceName, path);
        return region;
    }

    // Resolves optional spatial layout string markers used to snap animation anchors to character
    // geometries
    void readOptionalSpriteAnchor(
        const nlohmann::json& object,
        std::string_view key,
        SpriteAnchor&
            result, // Passed by reference to safely skip assignment blocks if fields drop out
        std::string_view sourceName,
        std::string_view path)
    {
        std::string anchor;
        readOptionalText(object, key, anchor, sourceName, path);
        if (anchor.empty())
        {
            return; // No value specified; bypass mapping overrides while leaving current defaults
                    // intact
        }

        // Match string keys safely down to specialized internal framework configuration tokens
        if (anchor == "feet")
        {
            result =
                SpriteAnchor::BodyFeet; // Snap rendering origin flush onto collision base floors
        }
        else if (anchor == "center")
        {
            result = SpriteAnchor::BodyCenter; // Snap rendering origin precisely to the bounding
                                               // core box midpoint
        }
        else
        {
            // Throw descriptive validation alerts pointing directly to the erroneous configuration
            // file layer path node
            failJson(
                sourceName,
                fieldPath(path, key),
                "unknown sprite anchor '" + anchor + "'; expected feet or center");
        }
    }

    // Parses a complete 2D texture sprite profile struct, extracting coordinate mapping bounds
    // alongside pivot properties
    Sprite jsonSprite(
        const nlohmann::json& value,
        std::string_view sourceName,
        std::string_view path)
    {
        // Enforce strict asset schema boundaries validation to intercept unregistered field
        // insertions loops
        checkJsonFields(value, {"position", "size", "anchor"}, sourceName, path);
        Sprite sprite;
        sprite.region = jsonSpriteRegion(value, sourceName, path);
        readOptionalSpriteAnchor(value, "anchor", sprite.anchor, sourceName, path);
        return sprite;
    }

    // Extracts an optional scalar integer value, packing successful query hits cleanly inside
    // standard std::optional layers
    void readOptionalInteger(
        const nlohmann::json& object,
        std::string_view key,
        std::optional<int>& result,
        std::string_view sourceName,
        std::string_view path)
    {
        if (const nlohmann::json* found = optionalJsonMember(object, key, sourceName, path))
        {
            result = jsonInteger(*found, sourceName, fieldPath(path, key));
        }
    }

    // Overloaded optional parser mapping successfully resolved sub-fields directly into flat
    // reference targets
    void readOptionalSprite(
        const nlohmann::json& object,
        std::string_view key,
        Sprite& result,
        std::string_view sourceName,
        std::string_view path)
    {
        if (const nlohmann::json* found = optionalJsonMember(object, key, sourceName, path))
        {
            result = jsonSprite(*found, sourceName, fieldPath(path, key));
        }
    }

    // Overloaded optional parser mapping successfully resolved sub-fields into std::optional
    // wrapped sprite profiles
    void readOptionalSprite(
        const nlohmann::json& object,
        std::string_view key,
        std::optional<Sprite>& result,
        std::string_view sourceName,
        std::string_view path)
    {
        if (const nlohmann::json* found = optionalJsonMember(object, key, sourceName, path))
        {
            result = jsonSprite(*found, sourceName, fieldPath(path, key));
        }
    }

    // Executes local drive input-output lookups to stream the raw contents of asset configuration
    // files into text strings
    std::string loadContentText(const std::filesystem::path& path)
    {
        std::ifstream file(path);
        // Intercept filesystem faults early to output clean, absolute diagnostic system path
        // tracing logs
        if (!file)
        {
            throw std::invalid_argument(
                "Could not open content file '" + std::filesystem::absolute(path).string() + "'");
        }
        return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
    }

    // Main parsing entry gateway that catches syntax compilation syntax faults and reconstructs
    // precise line and column error pointers
    nlohmann::json parseContentRoot(std::string_view text, std::string_view sourceName)
    {
        try
        {
            return nlohmann::json::parse(text.begin(), text.end());
        }
        catch (const nlohmann::json::parse_error& error)
        {
            // JSON reports a one-based byte position, including one past the end at EOF.
            std::size_t line = 1;
            std::size_t column = 1;

            // Back-calculate specific human-readable file coordinate spaces metrics line by line
            // using the raw data character offsets
            for (std::size_t index = 0; index < text.size() && index + 1 < error.byte; ++index)
            {
                if (text[index] == '\n')
                {
                    ++line;
                    column = 1; // Reset structural horizontal columns spacing back onto line
                                // margins anchors
                }
                else
                {
                    ++column;
                }
            }

            // Format a complete diagnostic telemetry error tracing summary before raising argument
            // failures Example: "assets/catalogs/actors.json: line 24, column 15: invalid JSON:
            // syntax error ..."
            failJson(
                sourceName,
                "line " + std::to_string(line) + ", column " + std::to_string(column),
                std::string("invalid JSON: ") + error.what());
        }
    }
}
