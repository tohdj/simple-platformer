#include "content_validation.hpp"
#include "content_diagnostics.hpp"

#include <cstddef>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <glm/vec2.hpp>

#include "simple_platformer/math/validation.hpp"
#include "simple_platformer/render/sprite.hpp"
#include "simple_platformer/world/tile_map.hpp"
#include "tile_catalog.hpp"
#include "level_data.hpp"
#include "simple_platformer/math/validation.hpp"

namespace simple_platformer
{
    // Validates a sprite asset profile to guarantee positional coordinates are finite/non-negative
    // and sizing parameters are strictly positive
    void validateContentSprite(const Sprite& sprite)
    {
        if (!isFiniteNonNegative(sprite.region.position) || !isFinitePositive(sprite.region.size))
        {
            throw std::invalid_argument(
                "sprite requires finite non-negative atlas position and positive size");
        }
    }

    // Assures that a sprite's custom sub-rect bounding layout parameters stay completely enclosed
    // within the hardware texture sheet dimensions
    void requireInAtlas(
        const SpriteRegion& region,
        glm::ivec2 atlasSize,
        std::string_view sourceName,
        std::string_view path)
    {
        // Compute the bottom-right corner point coordinate of the target sprite framing layout
        const glm::vec2 far = region.position + region.size;

        // Throw structured validation error exceptions if boundaries run past maximum edge
        // dimension allocations
        if (far.x > static_cast<float>(atlasSize.x) || far.y > static_cast<float>(atlasSize.y))
        {
            failJson(
                sourceName,
                path,
                "region runs past the " + std::to_string(atlasSize.x) + " by " +
                    std::to_string(atlasSize.y) + " atlas");
        }
    }

    // Validates inline pickup custom settings criteria (item stacks density rules and core size
    // dimensions) when an explicit asset reference template name is absent
    void validatePickupSettings(
        const PickupPlacement& placement,
        const std::string& path,
        std::string_view sourceName)
    {
        // If this layout assignment references a pre-defined configuration catalog name template
        // directly, trust its intrinsic properties values
        if (!placement.definitionName.empty())
        {
            return;
        }

        // Otherwise, enforce raw standalone constraint overrides checks (positive item quantity
        // counts and finite physics bodies sizes)
        if (placement.stack.quantity <= 0)
        {
            failJson(
                sourceName,
                fieldPath(path, "quantity"),
                "expected a positive integer, got " + std::to_string(placement.stack.quantity));
        }
        if (!isFinitePositive(placement.bodySize))
        {
            failJson(sourceName, fieldPath(path, "bodySize"), "expected a finite, positive size");
        }
    }

    // Validates level exit gateway mapping configurations, confirming naming properties, key
    // demands, and destination scenes bounds
    void validateExitSettings(
        const ExitPlacement& placement,
        const std::string& path,
        std::string_view sourceName)
    {
        if (placement.definitionName.empty())
        {
            failJson(
                sourceName, fieldPath(path, "definition"), "exit definition name cannot be empty");
        }
        // If gate entry parameters track mandatory keys items, verify that standard item count
        // limits are strictly positive numbers
        if (placement.requirement && placement.requirement->quantity <= 0)
        {
            failJson(
                sourceName,
                fieldPath(path, "requirement.quantity"),
                "expected a positive integer, got " +
                    std::to_string(placement.requirement->quantity));
        }
        // Confirm sequence redirection index parameters match scene targets layout tracking
        // boundaries
        if (placement.nextLevel && *placement.nextLevel <= 0)
        {
            failJson(sourceName, fieldPath(path, "nextLevel"), "level number must be positive");
        }
    }

    // Blocks duplicate conflicting allocation track errors for unique scene components that must
    // hold exactly one placement instance (e.g., player spawn anchor locations)
    void validateSinglePlacement(
        const std::vector<PlacementOrigin>& origins,
        std::string_view kind,
        std::string_view sourceName)
    {
        if (origins.empty())
        {
            failJson(sourceName, kind, "expected exactly one placement");
        }

        // Intercept logic crashes by culling collections out if duplicate markers or fields are
        // parsed from structural maps descriptions
        if (origins.size() > 1)
        {
            const auto& duplicate = origins[1];
            // Format precise debugging strings tracking if the error stem source comes out of an
            // ascii layout text symbol or a structured key parameter field
            const std::string description =
                duplicate.marker ? " marker '" + std::string(1, *duplicate.marker) + "'"
                                 : " placement";
            failJson(
                sourceName,
                duplicate.path,
                "second " + std::string(kind) + description + "; " + std::string(kind) +
                    " already placed at " + origins.front().path);
        }
    }

    // Validates fundamental metadata catalogs arrays for tiles, checking indices consistency, size
    // parameters ratios, and cyclic structural loop conditions
    void validateTileCatalog(const TileCatalog& catalog)
    {
        if (catalog.tileSize <= 0)
        {
            throw std::invalid_argument("tile catalog needs a positive tileSize");
        }

        // Hard core rule check invariant: the indexing string "empty" must reliably map down to
        // array vector index position zero
        const auto empty = catalog.ids.find("empty");
        if (empty == catalog.ids.end() || empty->second != 0 || catalog.definitions.empty())
        {
            throw std::invalid_argument("tile catalog must reserve ID zero for 'empty'");
        }

        // Physics logic check invariant: air/empty cell definitions cannot block entities, obscure
        // field-of-view lines, or hold climbable properties
        if (catalog.definitions.front().blocksMovement || catalog.definitions.front().blocksSight ||
            catalog.definitions.front().climbable)
        {
            throw std::invalid_argument("empty must allow movement and sight and not be climbable");
        }

        std::set<int> usedIds;
        for (const auto& entry : catalog.ids)
        {
            const int id = entry.second;
            // Capture pointers trying to resolve out-of-bounds index markers or duplicating
            // registered tracking integers
            if (id < 0 || static_cast<std::size_t>(id) >= catalog.definitions.size() ||
                !usedIds.insert(id).second)
            {
                throw std::invalid_argument(
                    "invalid or repeated tile ID for '" + entry.first + "'");
            }
            if (id == 0)
            {
                // Air gap definitions skip rendering draw pipelines entirely; bypass sprite
                // coordinates validation loops safely
                continue;
            }

            const TileDefinition& definition = catalog.definitions[static_cast<std::size_t>(id)];

            // Physics rules trace checking invariant: climbable mesh boundaries must act as
            // standard solid walls to allow climbing velocity ticks to hook correctly
            if (definition.climbable && !definition.blocksMovement)
            {
                throw std::invalid_argument(
                    "climbable tile '" + entry.first + "' must block movement");
            }

            // Confirm the asset texture cropping boundary regions evaluate inside valid limits
            // ranges parameters
            const auto& sprite = definition.sprite;
            if (!isFiniteNonNegative(sprite.position) || !isFinitePositive(sprite.size))
            {
                throw std::invalid_argument("invalid sprite region for tile '" + entry.first + "'");
            }
            if (sprite.size != glm::vec2{catalog.tileSize, catalog.tileSize})
            {
                const std::string side = std::to_string(catalog.tileSize);
                std::string message = "tile '" + entry.first + "' sprite must be ";
                message += side;
                message += " by ";
                message += side;
                message += " atlas pixels, the catalog's tileSize";
                throw std::invalid_argument(message);
            }

            // Catch structural collision bugs where a tile breaks into itself, or references a
            // completely non-existent item index template location
            const auto& breaksInto =
                catalog.definitions[static_cast<std::size_t>(id)].breaksIntoTileId;
            if (breaksInto.has_value() &&
                (*breaksInto < 0 ||
                 static_cast<std::size_t>(*breaksInto) >= catalog.definitions.size() ||
                 *breaksInto == id))
            {
                throw std::invalid_argument("invalid breaksInto tile for '" + entry.first + "'");
            }
        }

        // Assert complete vector-to-dictionary coverage density balance configurations
        if (usedIds.size() != catalog.definitions.size())
        {
            throw std::invalid_argument("every tile definition must have a catalog name");
        }
    }

    // Assures that every text-based character glyph mapping bound inside map legends resolves to a
    // verified catalog item profile definition
    void validateTileLegend(const std::map<char, std::string>& legend, const TileCatalog& catalog)
    {
        if (legend.empty())
        {
            throw std::invalid_argument("tileLegend: expected at least one symbol");
        }
        for (const auto& entry : legend)
        {
            if (catalog.ids.find(entry.second) == catalog.ids.end())
            {
                throw std::invalid_argument(
                    "Unknown tile name '" + entry.second + "' in tileLegend");
            }
        }
    }

    // Ensures text-based ASCII map glyph keys map uniquely without naming collisions between layers
    void validateLegendSymbols(
        const std::vector<std::string>& tileSymbols,
        const std::vector<std::string>& objectSymbols,
        std::string_view sourceName)
    {
        std::set<std::string> tiles;
        // --- STAGE 1: VALIDATE THE STATIC TILE GROUND LEGEND MAP SYMBOLS ---
        for (const auto& symbol : tileSymbols)
        {
            // ASCII map parsing layouts require isolated, single-character symbols keys
            if (symbol.size() != 1)
            {
                failJson(sourceName, "tileLegend", "symbols must be one character");
            }
            // Intercept duplicates: flag the schema if the letter is registered twice within the
            // list
            if (!tiles.insert(symbol).second)
            {
                failJson(sourceName, fieldPath("tileLegend", symbol), "repeated symbol");
            }
        }

        std::set<std::string> objects;
        // --- STAGE 2: VALIDATE THE DYNAMIC ENTITY OBJECT LEGEND MAP SYMBOLS ---
        for (const auto& symbol : objectSymbols)
        {
            const std::string path = fieldPath("objectLegend", symbol);
            if (symbol.size() != 1)
            {
                failJson(sourceName, path, "symbols must be one character");
            }
            // Enforce layer encapsulation: a single letter cannot map to a floor block and an
            // interactive character
            if (tiles.count(symbol) != 0)
            {
                failJson(sourceName, path, "symbol is also defined in tileLegend");
            }
            if (!objects.insert(symbol).second)
            {
                failJson(sourceName, path, "repeated symbol");
            }
        }
    }

    // Validates the literal dimensions and glyph elements consistency across a flat ASCII character
    // map diagram grid
    void validateMapRows(
        const std::vector<std::string>& rows,
        const std::map<char, std::string>& legend,
        std::string_view sourceName)
    {
        // Intercept logical fallacies: schemas must define a valid scene footprint to initialize
        // layout fields
        if (rows.empty())
        {
            failJson(sourceName, "map", "expected at least one row");
        }

        // Cache the string boundary size threshold of row 0 to evaluate standard rectangular
        // constraints
        const std::size_t width = rows.front().size();
        if (width == 0)
        {
            failJson(sourceName, "map[0]", "row cannot be empty");
        }

        // --- LOOP 1: ENFORCE UNIFORM GRID RECTANGULAR ALIGNMENT ---
        for (std::size_t row = 0; row < rows.size(); ++row)
        {
            const std::string path = indexPath("map", row);
            // Catch jagged map formatting strings errors early to avoid crash offsets downstream
            if (rows[row].size() != width)
            {
                failJson(
                    sourceName,
                    path,
                    "expected " + std::to_string(width) + " columns, got " +
                        std::to_string(rows[row].size()));
            }

            // --- LOOP 2: SCAN CELL BY CELL FOR UNREGISTERED GLYPH CHARACTERS ---
            for (std::size_t column = 0; column < width; ++column)
            {
                const char symbol = rows[row][column];
                // Catch any raw text symbol layout marks that have no definition mapping entry
                // matching headers logs
                if (legend.find(symbol) == legend.end())
                {
                    failJson(
                        sourceName,
                        indexPath(path, column),
                        "unknown symbol '" + std::string(1, symbol) +
                            "'; define it in tileLegend or objectLegend");
                }
            }
        }
    }
}
