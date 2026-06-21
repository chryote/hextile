#ifndef INTERACTABLE_SYSTEM_H
#define INTERACTABLE_SYSTEM_H

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

enum class InteractionTag : uint32_t {
    None        = 0,
    Choppable   = 1 << 0,
    Burnable    = 1 << 1,
    Forageable  = 1 << 2,
    Scrapeable  = 1 << 3,
    Clearable   = 1 << 4,
    Searchable  = 1 << 5
};

// Central vocabulary mapping for the MUD-style parser interface
struct CommandVerbRegistry {
    // Expected Layout: Map of lower-case verbs to game interaction tags
    static const std::unordered_map<std::string, InteractionTag>& get_mappings();
};

struct SubElementFeature {
    std::string keyword;                  // The specific sensory noun (e.g., "sap", "branches")
    InteractionTag active_tag;            // The valid structural tag allowed right now
    std::vector<std::string> valid_tools; // Specific item strings required (empty = bare hands allowed)
    
    // Dynamic Feedback String Blocks
    std::string success_msg;              // Printed on successful execution
    std::string failure_msg;              // Printed if the verb mismatches the tag
    std::string no_tool_msg;              // Printed if the player lacks the required tool

    // Refactor extensions for state mutation and description replacement
    InteractionTag success_mutation_tag = InteractionTag::None;
    std::string desc_substring_to_replace;
    std::string mutated_desc_substring;
};

struct AdvancedInteractableTemplate {
    std::string id;
    std::string name;
    std::string main_description;         // Base sensory text printed to the room
    
    // Tracks all individual sensory targets inside this object
    // Key: target keyword string (lowercase) -> Value: Feature rules
    std::unordered_map<std::string, SubElementFeature> features;

    // Helper to get description dynamically based on the runtime tag states
    std::string get_description(const std::unordered_map<std::string, InteractionTag>& runtime_tag_states) const;
};

class ObjectDictionaryManager {
private:
    // Master Registry: Template ID -> Structural Archetype
    std::unordered_map<std::string, AdvancedInteractableTemplate> master_registry;
    
    // Biome Mapping: Biome ID (int) -> Vector of compatible Template IDs
    std::unordered_map<int, std::vector<std::string>> biome_pools;

public:
    void initialize_dictionaries();
    const AdvancedInteractableTemplate* get_template(const std::string& template_id) const;
    std::vector<std::string> get_pool_for_biome(int biome_id) const;
};

struct ActiveObjectInstance {
    std::string template_id;
    // Map of overridden runtime tags to support state mutations/depletions dynamically
    std::unordered_map<std::string, InteractionTag> runtime_tag_states; 
};

#endif // INTERACTABLE_SYSTEM_H
