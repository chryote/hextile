#ifndef HEXTILE_FRAMEWORK_H
#define HEXTILE_FRAMEWORK_H

#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/string.hpp>
#include <vector>
#include <string>
#include <cstdint>
#include "interactable_system.h"

using namespace godot;

// Simulation Flags (Bitmasks for tag-driven behavior)
namespace Tags {
    enum Type : uint64_t {
        None        = 0,
        Raidable    = 1 << 0,
        Burnable    = 1 << 1,
        Sanctuary   = 1 << 2,
        Lootable    = 1 << 3,
        Inhabited   = 1 << 4,
        Underground = 1 << 5,
        Campable    = 1 << 6,
        Explorable  = 1 << 7,
        Forageable  = 1 << 8
    };
}

// Inline helper for checking tags
inline bool has_tag(uint64_t bitmask, uint64_t tag) {
    return (bitmask & tag) == tag;
}

// MUD Cardinal Directions
enum class Direction : uint8_t {
    North = 0, South, East, West, Up, Down, In, Out, Count
};

// Granular MUD-style Room
struct SubArea {
    int local_id;                // Index within the holding's local vector
    std::string name;            // e.g., "Tavern Cellar"
    std::string description;     // Text-RPG presentation string
    uint64_t tags = Tags::None;  // Dynamic simulation statuses

    // Links to other SubArea local_ids within the same holding (-1 if blocked)
    int exits[8] = { -1, -1, -1, -1, -1, -1, -1, -1 };
    
    std::vector<int> actor_ids;  // NPCs or Enemies currently inside
    std::vector<int> item_ids;   // Loot containers or objects dropped
    std::vector<ActiveObjectInstance> interactables; // Active interactable objects in this room
};

// CK2-Style Holding Area Type
enum class HoldingType : uint8_t {
    Wilderness, Ruins, Camp, Outpost, InlandSettlement, 
    HarborSettlement, Fortress, MerchantEnclave, Shrine
};

// The Macroscopic Anchor
struct Holding {
    int id;                           // Global Unique ID
    int parent_hex_idx;               // Parent Hextile reference
    HoldingType type;
    std::string name;                 // e.g., "Blackstone Keep"
    uint64_t tags = Tags::None;

    std::vector<SubArea> sub_areas;   // Local MUD room graph
    int entry_sub_area_idx = 0;       // Starting room when entering holding
};

struct Plate {
    int id;
    Vector2 velocity;
};

struct Region {
    int id = -1;
    String name;
    String type; // "continent", "island", "biome_region", "micro_region", "mountain_range", "river"
    std::vector<int> cell_indices;
    Vector2 center_position;
};

struct HexCell {
    int index;
    int x, y; // col, row
    int plate_id = -1;
    float elevation = 0.0f;
    float uplift = 0.0f;
    float noise_val = 0.0f;
    float temperature = 0.0f;
    float moisture = 0.0f;
    float water_accumulation = 0.0f;
    int biome = 0;
    int river_next_idx = -1;
    bool is_river = false;
    
    int landmass_id = -1;
    int biome_region_id = -1;
    int micro_region_id = -1;
    int mountain_range_id = -1;
    int river_id = -1;
    String overlay = "none";

    // Holdings Integration
    int holding_ids[5] = { -1, -1, -1, -1, -1 };
    uint8_t holding_count = 0;

    std::vector<ActiveObjectInstance> interactables;
};

#endif // HEXTILE_FRAMEWORK_H

