/**
 * @file        Diagnostic.cpp
 * @brief       Declares diagnostic path-building formatting helpers and error reporting
 *              utilities shared by JSON readers and asset validators.
 *
 * @author      Mohd Fadzuli
 * @author      Adapted by: Toh Da Jun
 * @copyright   Copyright (c) 2026 simple_platformer. All rights reserved.
 */

#include "content_diagnostics.hpp"
#include <cstddef>
#include <functional>
#include <stdexcept>
#include <string>
#include <string_view>

namespace simple_platformer
{
    // Exception translation wrapper that safely runs validation checks and routes standard errors
    // into structured JSON diagnostics format strings
    void validateInFile(std::string_view sourceName, const std::function<void()>& validate)
    {
        try
        {
            validate(); // Execute the target verification check lambda function block payload
        }
        catch (const std::invalid_argument& error)
        {
            // Catch raw system arguments errors and repackage them along with the source file
            // identification metadata string
            failJson(sourceName, {}, error.what());
        }
    }

    // Concatenates an asset path string and a sub-field text key together using dot notation syntax
    // definitions
    std::string fieldPath(std::string_view path, std::string_view key)
    {
        // If the upstream structural path identifier is empty, export the target key name directly
        // as a root node marker
        if (path.empty())
        {
            return std::string(key);
        }

        std::string result(path);
        result += "."; // Append standard dot separator between tree layers (e.g., "pickups" ->
                       // "pickups.sprite")
        result += key;
        return result;
    }

    // Appends a numerical index bracket suffix to a path string to cleanly track layout array
    // structures elements
    std::string indexPath(std::string_view path, std::size_t index)
    {
        std::string result(path);
        result += "[";
        result += std::to_string(
            index);    // Translate matrix positional indices numbers to readable text markers
        result += "]"; // Complete structural bracket layout (e.g., "mapRows" -> "mapRows[4]")
        return result;
    }

    // Constructs a unified diagnostic failure string specifying file parameters, data paths, and
    // error rules before throwing a runtime argument exception
    void failJson(std::string_view sourceName, std::string_view path, std::string_view message)
    {
        std::string prefix;

        // 1. Prepend physical data asset file paths descriptions if valid descriptors pass
        // validation checks
        if (!sourceName.empty())
        {
            prefix += std::string(sourceName) + ": ";
        }

        // 2. Append the calculated nested JSON schema node map hierarchy path signature trace
        if (!path.empty())
        {
            prefix += std::string(path) + ": ";
        }

        // Finalize assembly and deliberately trigger a structural argument crash to capture precise
        // stack traces logs Example Output Result: "assets/levels/level1.json: map.tiles.empty:
        // missing 'blocksMovement'"
        throw std::invalid_argument(prefix + std::string(message));
    }
}
