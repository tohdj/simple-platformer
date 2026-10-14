#include "item_catalog.hpp"
#include "content_diagnostics.hpp"
#include "content_json.hpp"
#include "content_validation.hpp"
#include "simple_platformer/inventory/item.hpp"
#include <cstddef>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <set>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace simple_platformer
{
    // Intercepts data anomalies across the item catalog map to enforce non-empty names, valid
    // sprites, and unique tracking IDs
    void validateItemCatalog(const ItemCatalog& catalog)
    {
        std::set<ItemId> ids;
        for (const auto& entry : catalog.definitions)
        {
            try
            {
                // Ensure the map dictionary look-up string key identifier isn't completely blank
                if (entry.first.empty())
                {
                    throw std::invalid_argument("item name cannot be empty");
                }
                validateItemDefinition(entry.second);
                validateContentSprite(entry.second.icon);

                // Track item IDs to assert and guarantee unique constraint boundaries across our
                // records
                if (!ids.insert(entry.second.id).second)
                {
                    throw std::invalid_argument("repeated item ID");
                }
            }
            catch (const std::invalid_argument& error)
            {
                // Catch errors and route a path diagnostic description specifying the target nested
                // dictionary node
                failJson({}, fieldPath("items", entry.first), error.what());
            }
        }
    }

    // Parses a raw configuration string stream down into an engine-ready binary ItemCatalog dataset
    // map
    ItemCatalog parseItemCatalog(std::string_view text, std::string_view sourceName)
    {
        // 1. Initialise structural JSON token parsing out of the text block
        const auto root = parseContentRoot(text, sourceName);
        checkJsonFields(root, {"items"}, sourceName, "root");
        const auto& definitions = requiredJsonMember(root, "items", sourceName, "root");
        checkJsonObject(definitions, sourceName, "items");

        ItemCatalog catalog;

        // Prevent array limits index overflow allocation crashes before walking through entries
        // loops
        if (definitions.size() > static_cast<std::size_t>(std::numeric_limits<ItemId>::max()))
        {
            failJson(sourceName, "items", "too many item definitions");
        }

        // 2. Loop through all sub-node keys nested under our primary catalog tracking parent node
        for (const auto& entry : definitions.items())
        {
            const std::string path = fieldPath("items", entry.key());
            const auto& value = entry.value();

            // Validate that the nested record adheres strictly to expected layout requirements
            // parameters
            checkJsonFields(
                value,
                {"name", "icon", "maximumStack", "effect", "effectAmount"},
                sourceName,
                path);

            ItemDefinition item;

            // Assign deterministic session-local IDs in JSON key order; new names can shift them.
            item.id = static_cast<ItemId>(catalog.definitions.size() + 1);
            item.name = readText(value, "name", sourceName, path);
            item.icon = jsonSprite(
                requiredJsonMember(value, "icon", sourceName, path),
                sourceName,
                fieldPath(path, "icon"));
            item.maximumStack = readInteger(value, "maximumStack", sourceName, path);

            // 3. Resolve optional status modifications side-effect properties tokens
            std::string effect = "none";
            readOptionalText(value, "effect", effect, sourceName, path);
            if (effect == "heal")
            {
                item.effect = ItemEffect::Heal;
            }
            else if (effect != "none")
            {
                failJson(
                    sourceName,
                    fieldPath(path, "effect"),
                    "unknown effect '" + effect + "'; expected heal or none");
            }
            readOptionalInteger(value, "effectAmount", item.effectAmount, sourceName, path);

            catalog.definitions.emplace(entry.key(), item);
        }

        // 4. Conclude operations by executing full post-construction file data integrity passes
        validateInFile(sourceName, [&] { validateItemCatalog(catalog); });
        return catalog;
    }

    // Handles file text reading disk input-output queries and streams strings downstream to target
    // text compiling nodes
    ItemCatalog loadItemCatalog(const std::filesystem::path& path)
    {
        return parseItemCatalog(loadContentText(path), path.string());
    }

    // Safe bounds-checked dataset dictionary search tool extraction item data references matching a
    // query name
    const ItemDefinition& itemDefinition(const ItemCatalog& catalog, const std::string& name)
    {
        const auto found = catalog.definitions.find(name);
        if (found == catalog.definitions.end())
        {
            throw std::invalid_argument("unknown item '" + name + "'");
        }
        return found->second;
    }

    // Translates a human-readable text stack blueprint reference cleanly into a real numeric
    // inventory tracking stack
    ItemStack composeItemStack(const ItemCatalog& catalog, const NamedItemStack& stack)
    {
        // Enforce fundamental logical safety rules: inventory allocations must contain at least one
        // item
        if (stack.quantity <= 0)
        {
            throw std::invalid_argument("item quantity must be positive");
        }

        // Query the catalog definitions by string name to fetch and return its unique underlying
        // database integer identification ID handle
        return {itemDefinition(catalog, stack.item).id, stack.quantity};
    }

    // Unpacks catalog dictionary data structures to compile a sequential vector array of active
    // game items
    std::vector<ItemDefinition> composeItems(const ItemCatalog& catalog, int textureId)
    {
        // Assert catalog validity before copying definitions into the display buffer
        validateItemCatalog(catalog);
        std::vector<ItemDefinition> result;

        // Loop through all parsed catalog items to populate the raw vector array storage
        for (const auto& entry : catalog.definitions)
        {
            auto item = entry.second; // Create a local mutable copy of the item definition template
            item.icon.textureId = textureId; // Explicitly link the global GPU texture handles index
                                             // onto the item icon
            result.push_back(item);
        }
        return result;
    }


    // Validates clip metrics boundary domains to verify every item icon image resides completely
    // within physical atlas sizes
    void validateItemAtlasRegions(
        const ItemCatalog& catalog,
        glm::ivec2 atlasSize,
        std::string_view sourceName)
    {
        for (const auto& [name, definition] : catalog.definitions)
        {
            // Traverse every entry and trap out-of-bounds texture clipping coordinates before
            // graphics rendering passes run
            requireInAtlas(
                definition.icon.region,
                atlasSize,
                sourceName,
                fieldPath(fieldPath("items", name), "icon"));
        }
    }
}
