#include "actor_catalog.hpp"
#include "content_diagnostics.hpp"
#include "content_json.hpp"
#include "content_validation.hpp"
#include "animation_catalog.hpp"
#include "content/actor_definition.hpp"
#include "simple_platformer/combat/combat.hpp"
#include "simple_platformer/movement/platformer_movement.hpp"
#include "simple_platformer/movement/flying_movement.hpp"
#include "simple_platformer/movement/surface_climb.hpp"
#include "simple_platformer/npc/npc.hpp"
#include <filesystem>
#include <initializer_list>
#include <optional>
#include <stdexcept>
#include <nlohmann/json.hpp>
#include <string_view>
#include <string>

namespace simple_platformer
{
    namespace
    {
        using Json = nlohmann::json;

        // Maps a textual string configuration key directly down to an engine relational team
        // classification token
        Team jsonTeam(const Json& value, std::string_view sourceName, std::string_view path)
        {
            const std::string team = jsonText(value, sourceName, path);
            if (team == "player")
            {
                return Team::Player; // Friendly user entities faction layer
            }
            if (team == "enemy")
            {
                return Team::Enemy; // Hostile aggressive entities faction layer
            }
            if (team == "neutral")
            {
                return Team::Neutral; // Passive or environmental entities faction layer
            }
            // Catch and report unrecognized string tokens to safeguard relational faction logic
            // loops
            failJson(
                sourceName,
                path,
                "unknown team '" + team + "'; expected player, enemy, or neutral");
        }

        // Maps high-level artificial intelligence personality tactical choices to corresponding
        // state machine codes
        NpcTactic jsonNpcTactic(
            const Json& value,
            std::string_view sourceName,
            std::string_view path)
        {
            const std::string tactic = jsonText(value, sourceName, path);
            if (tactic == "pursuer")
            {
                return NpcTactic::Pursuer; // Actively tracks and pathfinds straight to player
                                           // positions
            }
            if (tactic == "coward")
            {
                return NpcTactic::Coward; // Prioritizes escape routes and breaks engagement upon
                                          // threat triggers
            }
            if (tactic == "charger")
            {
                return NpcTactic::Charger; // Commits to rapid linear velocity burst attacks on
                                           // sight
            }
            if (tactic == "keepDistance")
            {
                return NpcTactic::KeepDistance; // Stabilizes navigation nodes loops to maintain
                                                // ranged circle barriers
            }
            failJson(
                sourceName,
                path,
                "unknown tactic '" + tactic +
                    "'; expected pursuer, keepDistance, coward or charger");
        }

        // Translates structural directional initialization text strings into simple horizontal
        // orientation constants
        Facing jsonFacing(const Json& value, std::string_view sourceName, std::string_view path)
        {
            const std::string facing = jsonText(value, sourceName, path);
            if (facing == "left")
            {
                return Facing::Left;
            }
            if (facing == "right")
            {
                return Facing::Right;
            }
            failJson(sourceName, path, "unknown facing '" + facing + "'; expected left or right");
        }

        // Each component reader starts from the C++ defaults and takes only the fields the
        // file names, so an empty object means "this component, as configured in code".

        // Parses and populates detailed kinematic movement attributes mapping standard 2D
        // platforming metrics
        PlatformerMovementConfig jsonPlatformerConfig(
            const Json& value,
            std::string_view sourceName,
            const std::string& path)
        {
            // Verify that the object maps strictly to allowed physics variables properties keys
            checkJsonFields(
                value,
                {"maximumSpeed",
                 "groundAcceleration",
                 "airAcceleration",
                 "groundDeceleration",
                 "jumpSpeed",
                 "gravity",
                 "jumpReleaseGravity",
                 "maximumFallSpeed",
                 "coyoteDuration",
                 "jumpBufferDuration"},
                sourceName,
                path);

            PlatformerMovementConfig
                config; // Instantiated with hardcoded baseline default constants weights in C++

            // Inline lambda utility function to shorten repetitive optional lookup logic passes
            const auto number = [&](std::string_view key, float& field)
            { readOptionalNumber(value, key, field, sourceName, path); };

            // --- STREAM PARAMETERS DOWNWARD: Safely apply configuration overrides over default
            // values ---
            number("maximumSpeed", config.maximumSpeed); // Terminal horizontal velocity limits
            number(
                "groundAcceleration",
                config.groundAcceleration); // Ground horizontal speedup coefficients
            number("airAcceleration", config.airAcceleration); // Mid-air lateral agility handling
            number(
                "groundDeceleration",
                config.groundDeceleration);        // Ground friction dampening responses
            number("jumpSpeed", config.jumpSpeed); // Upward vertical velocity force impulse
            number("gravity", config.gravity);     // Standard baseline gravity acceleration profile
            number(
                "jumpReleaseGravity",
                config.jumpReleaseGravity); // Gravity multiplier applied when jump buttons are let
                                            // go early (Variable Jump heights)
            number(
                "maximumFallSpeed",
                config.maximumFallSpeed); // Maximum terminal velocity threshold for falling drops
            number("coyoteDuration", config.coyoteDuration); // Time buffer window allowing jumps
                                                             // after walking past platform edges
            number(
                "jumpBufferDuration",
                config.jumpBufferDuration); // Time buffer window processing jump inputs sent right
                                            // before landing on blocks

            return config;
        }

        // Parses configuration fields defining free-flying movement properties for airborne
        // entities
        FlyingMovement jsonFlyingMovement(
            const Json& value,
            std::string_view sourceName,
            const std::string& path)
        {
            checkJsonFields(value, {"speed"}, sourceName, path);
            FlyingMovement config; // Instantiated with hardcoded baseline default parameters in C++
            readOptionalNumber(value, "speed", config.speed, sourceName, path);
            return config;
        }

        // Parses configuration parameters managing vertical wall or ceiling crawling locomotion
        // speeds
        SurfaceClimbConfig jsonSurfaceClimbConfig(
            const Json& value,
            std::string_view sourceName,
            const std::string& path)
        {
            checkJsonFields(value, {"speed"}, sourceName, path);
            SurfaceClimbConfig
                config; // Keeps structural code-defined default metrics if JSON fields are empty
            readOptionalNumber(value, "speed", config.speed, sourceName, path);
            return config;
        }

        // Unpacks AI monitoring threshold attributes mapping sensory detection radiuses and memory
        // timers
        NpcSenses jsonNpcSenses(
            const Json& value,
            std::string_view sourceName,
            const std::string& path)
        {
            // Verify that the object contains strictly supported behavior properties keys
            checkJsonFields(
                value,
                {"noticeDistance", "standoffDistance", "targetMemoryDuration", "searchDuration"},
                sourceName,
                path);
            NpcSenses config;

            // --- STREAM PARAMETERS DOWNWARD: Safely apply AI tracking configuration overrides ---
            readOptionalNumber(
                value,
                "noticeDistance",
                config.noticeDistance,
                sourceName,
                path); // Maximum line-of-sight raycast detection radius
            readOptionalNumber(
                value,
                "standoffDistance",
                config.standoffDistance,
                sourceName,
                path); // Ideal circle tracking separation gap maintained by ranged units
            readOptionalNumber(
                value,
                "targetMemoryDuration",
                config.targetMemoryDuration,
                sourceName,
                path); // Cooldown threshold tracking how long an NPC remembers a target after
                       // losing sight
            readOptionalNumber(
                value,
                "searchDuration",
                config.searchDuration,
                sourceName,
                path); // Time duration spent scanning around the last known target coordinate spot
            return config;
        }

        // Deserialises close-quarters melee combat parameters, mapping timings and attack framing
        // dimensions
        BiteAttack jsonBite(const Json& value, std::string_view sourceName, const std::string& path)
        {
            checkJsonFields(
                value,
                {"damage",
                 "hitboxSize",
                 "reach",
                 "windupDuration",
                 "activeDuration",
                 "recoveryDuration"},
                sourceName,
                path);
            BiteAttack config;

            // Inline helper lambda statement to reduce code duplication for reading optional
            // floating-point variables
            const auto number = [&](std::string_view key, float& field)
            { readOptionalNumber(value, key, field, sourceName, path); };

            readOptionalInteger(value, "damage", config.damage, sourceName, path);
            readOptionalVector(
                value,
                "hitboxSize",
                config.hitboxSize,
                sourceName,
                path); // Dimensional box boundaries width/height metrics
            number(
                "reach",
                config.reach); // Positional linear projection extension offset from the center
            number(
                "windupDuration",
                config.windupDuration); // Pre-attack action delay window (Telegraph phase)
            number("activeDuration", config.activeDuration); // Time frame window during which the
                                                             // physical hitbox deals damage
            number(
                "recoveryDuration",
                config.recoveryDuration); // Post-attack action lock phase (Cool down rest cycle)
            return config;
        }

        // Unpacks passive touch-based damage settings applied when an entity collides directly with
        // another actor
        ContactDamage jsonContactDamage(
            const Json& value,
            std::string_view sourceName,
            const std::string& path)
        {
            checkJsonFields(value, {"damage"}, sourceName, path);
            ContactDamage config;
            readOptionalInteger(value, "damage", config.damage, sourceName, path);
            return config;
        }

        // Parses configuration parameters defining projectiles, firing rate limits, and sprite
        // assets for ranged weaponry
        RangedWeapon jsonRangedWeapon(
            const Json& value,
            std::string_view sourceName,
            const std::string& path)
        {
            // Verify that the object contains strictly supported combat property keys
            checkJsonFields(
                value,
                {"damage",
                 "projectileSize",
                 "projectileSpeed",
                 "projectileLifetime",
                 "shootDuration",
                 "recoveryDuration",
                 "breaksTiles",
                 "sprite"},
                sourceName,
                path);
            RangedWeapon config; // Instantiated with base default values configured in code

            // Inline helper lambda to streamline reading multiple floating-point scalar metrics
            const auto number = [&](std::string_view key, float& field)
            { readOptionalNumber(value, key, field, sourceName, path); };

            // --- STREAM PARAMETERS DOWNWARD: Safely apply weapon property overrides ---
            readOptionalInteger(value, "damage", config.damage, sourceName, path);
            readOptionalVector(
                value,
                "projectileSize",
                config.projectileSize,
                sourceName,
                path); // Width and height vectors of the projectile bounding box
            number(
                "projectileSpeed",
                config.projectileSpeed); // Linear travel velocity tracking scalar
            number(
                "projectileLifetime",
                config.projectileLifetime); // Expiration countdown limit in seconds for spawned
                                            // projectile instances
            number("shootDuration", config.shootDuration); // Action lock window length required to
                                                           // fire a shot (Telegraph phase)
            number(
                "recoveryDuration",
                config.recoveryDuration); // Cool down rest cycle threshold required before
                                          // consecutive shots can fire
            readOptionalBoolean(
                value,
                "breaksTiles",
                config.breaksTiles,
                sourceName,
                path); // Logic flag determining destruction behavior when tiles are hit
            readOptionalSprite(
                value,
                "sprite",
                config.projectileSprite,
                sourceName,
                path); // Sprite atlas clipping coordinate descriptors mapping the asset icon
            return config;
        }

        // Extracts an optional string key mapping faction groups, updating the team variable if
        // found
        void readOptionalTeam(
            const Json& object,
            std::string_view key,
            Team& result,
            std::string_view sourceName,
            const std::string& path)
        {
            if (const Json* found = optionalJsonMember(object, key, sourceName, path))
            {
                result = jsonTeam(*found, sourceName, fieldPath(path, key));
            }
        }

        // Extracts an optional string key mapping AI personality tracking behaviours, updating the
        // variable if found
        void readOptionalNpcTactic(
            const Json& object,
            std::string_view key,
            NpcTactic& result,
            std::string_view sourceName,
            const std::string& path)
        {
            if (const Json* found = optionalJsonMember(object, key, sourceName, path))
            {
                result = jsonNpcTactic(*found, sourceName, fieldPath(path, key));
            }
        }

        // Extracts an optional initial orientation text indicator string, transforming the value if
        // found
        void readOptionalFacing(
            const Json& object,
            std::string_view key,
            Facing& result,
            std::string_view sourceName,
            const std::string& path)
        {
            if (const Json* found = optionalJsonMember(object, key, sourceName, path))
            {
                result = jsonFacing(*found, sourceName, fieldPath(path, key));
            }
        }

        // Conditionally instantiates a platformer movement sub-component inside an optional layer
        // wrapper block
        void readOptionalPlatformerConfig(
            const Json& object,
            std::string_view key,
            std::optional<PlatformerMovementConfig>& result,
            std::string_view sourceName,
            const std::string& path)
        {
            if (const Json* found = optionalJsonMember(object, key, sourceName, path))
            {
                result = jsonPlatformerConfig(*found, sourceName, fieldPath(path, key));
            }
        }

        // Conditionally instantiates free-flying navigation parameters if an airborne config block
        // is present
        void readOptionalFlyingMovement(
            const Json& object,
            std::string_view key,
            std::optional<FlyingMovement>& result,
            std::string_view sourceName,
            const std::string& path)
        {
            if (const Json* found = optionalJsonMember(object, key, sourceName, path))
            {
                result = jsonFlyingMovement(*found, sourceName, fieldPath(path, key));
            }
        }

        // Conditionally maps crawling speed configurations inside optional properties variables
        // wrappers
        void readOptionalSurfaceClimbConfig(
            const Json& object,
            std::string_view key,
            std::optional<SurfaceClimbConfig>& result,
            std::string_view sourceName,
            const std::string& path)
        {
            if (const Json* found = optionalJsonMember(object, key, sourceName, path))
            {
                result = jsonSurfaceClimbConfig(*found, sourceName, fieldPath(path, key));
            }
        }

        // Conditionally populates sensory field parameters and memory duration limits for automated
        // agents
        void readOptionalNpcSenses(
            const Json& object,
            std::string_view key,
            std::optional<NpcSenses>& result,
            std::string_view sourceName,
            const std::string& path)
        {
            if (const Json* found = optionalJsonMember(object, key, sourceName, path))
            {
                result = jsonNpcSenses(*found, sourceName, fieldPath(path, key));
            }
        }

        // Conditionally populates close-quarters melee timing tracking values into optional layers
        void readOptionalBite(
            const Json& object,
            std::string_view key,
            std::optional<BiteAttack>& result,
            std::string_view sourceName,
            const std::string& path)
        {
            if (const Json* found = optionalJsonMember(object, key, sourceName, path))
            {
                result = jsonBite(*found, sourceName, fieldPath(path, key));
            }
        }

        // Conditionally maps passive entity contact hazard collision damage settings if present
        void readOptionalContactDamage(
            const Json& object,
            std::string_view key,
            std::optional<ContactDamage>& result,
            std::string_view sourceName,
            const std::string& path)
        {
            if (const Json* found = optionalJsonMember(object, key, sourceName, path))
            {
                result = jsonContactDamage(*found, sourceName, fieldPath(path, key));
            }
        }

        // Conditionally binds ranged weapons system sub-components, enabling project calculations
        // parameters
        void readOptionalRangedWeapon(
            const Json& object,
            std::string_view key,
            std::optional<RangedWeapon>& result,
            std::string_view sourceName,
            const std::string& path)
        {
            if (const Json* found = optionalJsonMember(object, key, sourceName, path))
            {
                result = jsonRangedWeapon(*found, sourceName, fieldPath(path, key));
            }
        }

        // Deserialises an individual actor's sub-components, properties, and core schema fields out
        // of a JSON block
        ActorDefinition jsonActorDefinition(
            const Json& value,
            std::string_view sourceName,
            const std::string& path)
        {
            // Verify that the object contains strictly supported sub-component property keys
            checkJsonFields(
                value,
                {"bodySize",
                 "team",
                 "facing",
                 "spriteAnchor",
                 "animations",
                 "health",
                 "inventorySlots",
                 "platformer",
                 "flying",
                 "surfaceClimb",
                 "senses",
                 "tactic",
                 "bite",
                 "contactDamage",
                 "ranged"},
                sourceName,
                path);
            ActorDefinition result;

            // --- STREAM PARAMETERS DOWNWARD: Extract required vectors and conditionally update
            // optional modules ---
            result.bodySize = readVector(
                value,
                "bodySize",
                sourceName,
                path); // Core physical width and height vectors bounds
            readOptionalText(
                value,
                "animations",
                result.animations,
                sourceName,
                path); // Linked animation resource catalog key name
            readOptionalSpriteAnchor(
                value,
                "spriteAnchor",
                result.spriteAnchor,
                sourceName,
                path); // Pivot configuration snapping (Feet vs Center)
            readOptionalTeam(value, "team", result.team, sourceName, path);
            readOptionalFacing(value, "facing", result.facing, sourceName, path);
            readOptionalInteger(value, "health", result.health, sourceName, path);
            readOptionalInteger(value, "inventorySlots", result.inventorySlots, sourceName, path);
            readOptionalPlatformerConfig(value, "platformer", result.platformer, sourceName, path);
            readOptionalFlyingMovement(value, "flying", result.flying, sourceName, path);
            readOptionalSurfaceClimbConfig(
                value, "surfaceClimb", result.surfaceClimb, sourceName, path);
            readOptionalNpcSenses(value, "senses", result.senses, sourceName, path);
            readOptionalNpcTactic(value, "tactic", result.tactic, sourceName, path);
            readOptionalBite(value, "bite", result.bite, sourceName, path);
            readOptionalContactDamage(
                value, "contactDamage", result.contactDamage, sourceName, path);
            readOptionalRangedWeapon(value, "ranged", result.ranged, sourceName, path);
            return result;
        }
    }

    // Parses a raw text config stream down into an operational binary ActorCatalog database
    // collection map
    ActorCatalog parseActorCatalog(
        std::string_view text,
        std::string_view sourceName,
        const AnimationCatalog& animations)
    {
        // 1. Initialize structural JSON token parsing layouts out of the file stream
        const auto root = parseContentRoot(text, sourceName);
        checkJsonFields(root, {"player", "actors"}, sourceName, "root");

        ActorCatalog result;
        // Identify which entity blueprint string token represents the primary human-controlled user
        // container
        result.player = readText(root, "player", sourceName, "root");

        const auto& definitions = requiredJsonMember(root, "actors", sourceName, "root");
        checkJsonObject(definitions, sourceName, "actors");
        if (definitions.empty())
        {
            failJson(sourceName, "actors", "expected a nonempty object");
        }

        // 2. Loop through all child object dictionary keys to populate the blueprint listings
        // arrays
        for (const auto& entry : definitions.items())
        {
            if (entry.key().empty())
            {
                failJson(sourceName, "actors", "actor name cannot be empty");
            }
            // Parse out specific sub-components fields and store under the template dictionary name
            // key lookup
            result.definitions.emplace(
                entry.key(),
                jsonActorDefinition(entry.value(), sourceName, fieldPath("actors", entry.key())));
        }

        // 3. Fire complete content validation verification routines before returning output dataset
        // structures
        validateInFile(sourceName, [&] { validateActorCatalog(result, animations); });
        return result;
    }

    // Runs deep state validation tests ensuring internal components integrity alongside player
    // entity constraints
    void validateActorCatalog(const ActorCatalog& catalog, const AnimationCatalog& animations)
    {
        // --- STAGE 1: RECURSIVE BLUEPRINT VALIDATION ---
        for (const auto& entry : catalog.definitions)
        {
            if (entry.first.empty())
            {
                throw std::invalid_argument("actor name cannot be empty");
            }
            try
            {
                // Verify entity definitions structure using our low-level dry-run composer handler
                validateActorDefinition(entry.second, animations);
            }
            catch (const std::invalid_argument& error)
            {
                failJson({}, fieldPath("actors", entry.first), error.what());
            }
        }

        // --- STAGE 2: PLAYER PROFILE SPECIFIC INVARIANT RULES TESTS ---
        const auto& player = actorDefinition(catalog, catalog.player);
        // Player units must use discrete keyboard hooks; automated sensory processing must remain
        // turned off
        if (player.senses)
        {
            throw std::invalid_argument("player definition must not enable NPC sensing");
        }
        // Player configurations must explicitly hold health and inventory records to map cleanly
        // into display HUD widgets
        if (!player.health || !player.inventorySlots)
        {
            throw std::invalid_argument(
                "player definition requires health and inventorySlots for the game HUD");
        }
    }

    // Handles file text reading disk input-output queries and streams data directly downward to
    // text parsing units
    ActorCatalog loadActorCatalog(
        const std::filesystem::path& path,
        const AnimationCatalog& animations)
    {
        return parseActorCatalog(loadContentText(path), path.string(), animations);
    }

    // Safe bounds-checked search utility extracting an actor definition dictionary record matching
    // a target key string name
    const ActorDefinition& actorDefinition(const ActorCatalog& catalog, const std::string& name)
    {
        const auto found = catalog.definitions.find(name);
        if (found == catalog.definitions.end())
        {
            throw std::invalid_argument("unknown actor definition '" + name + "'");
        }
        return found->second;
    }

    // Validates clip boundaries coordinate regions to verify ranged projectile sprite images fit
    // inside the physical atlas
    void validateActorAtlasRegions(
        const ActorCatalog& catalog,
        glm::ivec2 atlasSize,
        std::string_view sourceName)
    {
        for (const auto& [name, definition] : catalog.definitions)
        {
            // If the entity profile utilizes a ranged projectile weapon component, check its
            // graphic asset bounds placement
            if (definition.ranged.has_value())
            {
                requireInAtlas(
                    definition.ranged->projectileSprite.region,
                    atlasSize,
                    sourceName,
                    fieldPath(fieldPath(fieldPath("actors", name), "ranged"), "sprite"));
            }
        }
    }
}
