#ifndef HEXTILE_FRAMEWORK_H
#define HEXTILE_FRAMEWORK_H

#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/string.hpp>
#include <vector>

using namespace godot;

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
};

#endif // HEXTILE_FRAMEWORK_H
