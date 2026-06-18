#include "hexcrawl_map_generator.h"
#include <queue>
#include <algorithm>
#include <cmath>
#include <numeric>
#include <iostream>

HexcrawlMapGenerator::HexcrawlMapGenerator() {}
HexcrawlMapGenerator::~HexcrawlMapGenerator() {}

void HexcrawlMapGenerator::initialize(int p_width, int p_height, int p_seed) {
    width = p_width;
    height = p_height;
    seed = p_seed;
    rng.seed(seed);

    cells.clear();
    cells.resize(width * height);
    for (int i = 0; i < width * height; ++i) {
        cells[i].index = i;
        cells[i].x = i % width;
        cells[i].y = i / width;
        cells[i].plate_id = -1;
        cells[i].elevation = 0.0f;
        cells[i].uplift = 0.0f;
        cells[i].noise_val = 0.0f;
        cells[i].temperature = 0.0f;
        cells[i].moisture = 0.0f;
        cells[i].water_accumulation = 0.0f;
        cells[i].biome = 0;
        cells[i].river_next_idx = -1;
        cells[i].is_river = false;
        
        cells[i].landmass_id = -1;
        cells[i].biome_region_id = -1;
        cells[i].micro_region_id = -1;
        cells[i].mountain_range_id = -1;
        cells[i].river_id = -1;
        cells[i].overlay = "none";

        cells[i].holding_count = 0;
        for (int h = 0; h < 5; ++h) {
            cells[i].holding_ids[h] = -1;
        }
    }
    plates.clear();
    regions.clear();
    global_holdings.clear();
    player_current_holding_id = -1;
    player_current_room_id = -1;
}

std::vector<int> HexcrawlMapGenerator::get_neighbors_internal(int idx) const {
    int x = idx % width;
    int y = idx / width;
    std::vector<int> neighbors;

    int left = x - 1;
    int right = x + 1;
    int top = y - 1;
    int bottom = y + 1;

    bool is_odd = (y & 1) != 0;

    auto add_if_valid = [&](int nx, int ny) {
        if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
            neighbors.push_back(ny * width + nx);
        }
    };

    add_if_valid(left, y);
    add_if_valid(right, y);

    if (is_odd) {
        add_if_valid(x, top);
        add_if_valid(right, top);
        add_if_valid(x, bottom);
        add_if_valid(right, bottom);
    } else {
        add_if_valid(left, top);
        add_if_valid(x, top);
        add_if_valid(left, bottom);
        add_if_valid(x, bottom);
    }
    return neighbors;
}

PackedInt32Array HexcrawlMapGenerator::get_neighbors(int idx) const {
    PackedInt32Array arr;
    std::vector<int> neighbors = get_neighbors_internal(idx);
    arr.resize(neighbors.size());
    for (size_t i = 0; i < neighbors.size(); ++i) {
        arr[i] = neighbors[i];
    }
    return arr;
}

bool HexcrawlMapGenerator::is_shallow_water(int idx) const {
    if (idx < 0 || idx >= (int)cells.size()) return false;
    float elev = cells[idx].elevation;
    if (elev >= ocean_level) return false;
    if (elev > ocean_level - shallow_water_depth) return true;

    std::vector<int> neighbors = get_neighbors_internal(idx);
    for (int n_idx : neighbors) {
        if (cells[n_idx].elevation >= ocean_level) {
            return true;
        }
    }
    return false;
}

Vector2 HexcrawlMapGenerator::get_cell_position(int idx) const {
    int x = idx % width;
    int y = idx / width;
    bool is_odd = (y & 1) != 0;
    float px = (float)x + (is_odd ? 0.5f : 0.0f);
    float py = (float)y * 0.8660254f; // sqrt(3)/2
    return Vector2(px, py);
}





void HexcrawlMapGenerator::step_generation(int step_index) {
    switch (step_index) {
        case 1:
            step_generate_plates();
            break;
        case 2:
            step_compute_uplift();
            break;
        case 3:
            step_add_base_noise();
            break;
        case 4:
            step_run_erosion();
            break;
        case 5:
            step_generate_rivers();
            break;
        case 6:
            step_simulate_climate();
            break;
        case 7:
            step_assign_biomes();
            break;
        case 8:
            step_generate_holdings();
            break;
        default:
            break;
    }
}

void HexcrawlMapGenerator::run_full_pipeline() {
    step_generate_plates();
    step_compute_uplift();
    step_add_base_noise();
    step_run_erosion();
    step_generate_rivers();
    step_simulate_climate();
    step_assign_biomes();
    step_generate_holdings();
}

Dictionary HexcrawlMapGenerator::get_cell_data(int idx) const {
    Dictionary data;
    if (idx < 0 || idx >= (int)cells.size()) return data;

    const HexCell &c = cells[idx];
    data["index"] = c.index;
    data["x"] = c.x;
    data["y"] = c.y;
    data["plate_id"] = c.plate_id;
    data["elevation"] = c.elevation;
    data["uplift"] = c.uplift;
    data["noise_val"] = c.noise_val;
    data["temperature"] = c.temperature;
    data["moisture"] = c.moisture;
    data["water_accumulation"] = c.water_accumulation;
    data["biome"] = c.biome;
    data["river_next_idx"] = c.river_next_idx;
    data["is_river"] = c.is_river;
    data["overlay"] = c.overlay;

    Array holding_ids_arr;
    for (int h = 0; h < c.holding_count; ++h) {
        holding_ids_arr.append(c.holding_ids[h]);
    }
    data["holding_ids"] = holding_ids_arr;
    data["holding_count"] = c.holding_count;
    
    Vector2 pos = get_cell_position(idx);
    data["position"] = pos;

    String landmass_name = "";
    if (c.landmass_id >= 0 && c.landmass_id < (int)regions.size()) {
        landmass_name = regions[c.landmass_id].name;
    }
    data["landmass_name"] = landmass_name;

    String biome_region_name = "";
    if (c.biome_region_id >= 0 && c.biome_region_id < (int)regions.size()) {
        biome_region_name = regions[c.biome_region_id].name;
    }
    data["biome_region_name"] = biome_region_name;

    String micro_region_name = "";
    if (c.micro_region_id >= 0 && c.micro_region_id < (int)regions.size()) {
        micro_region_name = regions[c.micro_region_id].name;
    }
    data["micro_region_name"] = micro_region_name;

    String mountain_range_name = "";
    if (c.mountain_range_id >= 0 && c.mountain_range_id < (int)regions.size()) {
        mountain_range_name = regions[c.mountain_range_id].name;
    }
    data["mountain_range_name"] = mountain_range_name;

    String river_name = "";
    if (c.river_id >= 0 && c.river_id < (int)regions.size()) {
        river_name = regions[c.river_id].name;
    }
    data["river_name"] = river_name;

    return data;
}

Array HexcrawlMapGenerator::get_river_paths() const {
    Array paths;
    std::vector<bool> visited(cells.size(), false);

    for (int i = 0; i < (int)cells.size(); ++i) {
        // Rivers start when flow accumulation is significant
        if (cells[i].is_river && cells[i].river_next_idx != -1 && !visited[i]) {
            // Check if this is a river source (nobody flows into it, or its accumulation is smaller than its downstream)
            bool is_source = true;
            std::vector<int> neighbors = get_neighbors_internal(i);
            for (int n_idx : neighbors) {
                if (cells[n_idx].river_next_idx == i && cells[n_idx].is_river) {
                    is_source = false;
                    break;
                }
            }

            if (is_source) {
                Array path;
                int curr = i;
                while (curr != -1 && cells[curr].elevation >= ocean_level) {
                    path.append(get_cell_position(curr));
                    visited[curr] = true;
                    curr = cells[curr].river_next_idx;
                }
                // If it ends in ocean, add ocean cell position too
                if (curr != -1) {
                    path.append(get_cell_position(curr));
                }
                paths.append(path);
            }
        }
    }
    return paths;
}

PackedFloat32Array HexcrawlMapGenerator::get_elevations() const {
    PackedFloat32Array arr;
    arr.resize(cells.size());
    for (size_t i = 0; i < cells.size(); ++i) {
        arr[i] = cells[i].elevation;
    }
    return arr;
}

PackedFloat32Array HexcrawlMapGenerator::get_moistures() const {
    PackedFloat32Array arr;
    arr.resize(cells.size());
    for (size_t i = 0; i < cells.size(); ++i) {
        arr[i] = cells[i].moisture;
    }
    return arr;
}

PackedFloat32Array HexcrawlMapGenerator::get_temperatures() const {
    PackedFloat32Array arr;
    arr.resize(cells.size());
    for (size_t i = 0; i < cells.size(); ++i) {
        arr[i] = cells[i].temperature;
    }
    return arr;
}

PackedInt32Array HexcrawlMapGenerator::get_biomes() const {
    PackedInt32Array arr;
    arr.resize(cells.size());
    for (size_t i = 0; i < cells.size(); ++i) {
        arr[i] = cells[i].biome;
    }
    return arr;
}

PackedInt32Array HexcrawlMapGenerator::get_plates() const {
    PackedInt32Array arr;
    arr.resize(cells.size());
    for (size_t i = 0; i < cells.size(); ++i) {
        arr[i] = cells[i].plate_id;
    }
    return arr;
}

PackedInt32Array HexcrawlMapGenerator::get_river_next() const {
    PackedInt32Array arr;
    arr.resize(cells.size());
    for (size_t i = 0; i < cells.size(); ++i) {
        arr[i] = cells[i].river_next_idx;
    }
    return arr;
}

PackedFloat32Array HexcrawlMapGenerator::get_water_accumulations() const {
    PackedFloat32Array arr;
    arr.resize(cells.size());
    for (size_t i = 0; i < cells.size(); ++i) {
        arr[i] = cells[i].water_accumulation;
    }
    return arr;
}

PackedStringArray HexcrawlMapGenerator::get_overlays() const {
    PackedStringArray arr;
    arr.resize(cells.size());
    for (size_t i = 0; i < cells.size(); ++i) {
        arr[i] = cells[i].overlay;
    }
    return arr;
}

void HexcrawlMapGenerator::compute_overlays() {
    // 1. Reset overlays and identify mountains
    for (int i = 0; i < (int)cells.size(); ++i) {
        if (cells[i].elevation >= 0.85f) {
            cells[i].overlay = "high_mountain";
        } else if (cells[i].elevation >= 0.70f) {
            cells[i].overlay = "low_mountain";
        } else {
            cells[i].overlay = "none";
        }
    }

    // 2. Identify river sources and propagate hills
    std::vector<bool> is_hill(cells.size(), false);
    for (int i = 0; i < (int)cells.size(); ++i) {
        if (cells[i].is_river && cells[i].river_next_idx != -1) {
            // Check if this is a river source (nobody flows into it)
            bool is_source = true;
            std::vector<int> neighbors = get_neighbors_internal(i);
            for (int n_idx : neighbors) {
                if (cells[n_idx].river_next_idx == i && cells[n_idx].is_river) {
                    is_source = false;
                    break;
                }
            }

            if (is_source) {
                is_hill[i] = true;
                for (int n_idx : neighbors) {
                    is_hill[n_idx] = true;
                }
            }
        }
    }

    // 3. Assign hills to cells that are land and not already mountains
    for (int i = 0; i < (int)cells.size(); ++i) {
        if (is_hill[i] && cells[i].elevation >= ocean_level && cells[i].overlay == "none") {
            cells[i].overlay = "hill";
        }
    }
}

Vector2 HexcrawlMapGenerator::get_plate_velocity(int plate_id) const {
    if (plate_id >= 0 && plate_id < (int)plates.size()) {
        return plates[plate_id].velocity;
    }
    return Vector2();
}

void HexcrawlMapGenerator::generate_debug_map() {
    if (width <= 0 || height <= 0) return;

    cells.clear();
    cells.resize(width * height);
    
    plates.clear();
    int actual_num_plates = 8;
    for (int p = 0; p < actual_num_plates; ++p) {
        Plate plate;
        plate.id = p;
        float angle = (float)p / actual_num_plates * 2.0f * 3.14159f;
        plate.velocity = Vector2(cos(angle), sin(angle)) * 1.5f;
        plates.push_back(plate);
    }
    
    std::vector<int> debug_biomes = {
        0,   // Deep Ocean
        13,  // Shallow Coast
        14,  // Lake
        1,   // Sandy Beach
        15,  // Sandy Basin
        2,   // Tundra
        3,   // Glacier/Snow
        4,   // Boreal Forest (Taiga)
        5,   // Cold Desert
        6,   // Temperate Grassland
        7,   // Temperate Forest
        8,   // Temperate Rainforest
        9,   // Subtropical Desert
        10,  // Savanna
        11,  // Tropical Dry Forest
        12   // Tropical Rainforest
    };

    for (int i = 0; i < (int)cells.size(); ++i) {
        int cx = i % width;
        int cy = i / width;
        
        HexCell &c = cells[i];
        c.index = i;
        c.x = cx;
        c.y = cy;
        
        int biome_idx = cy % debug_biomes.size();
        c.biome = debug_biomes[biome_idx];
        
        int col_feature = cx % 10;
        
        c.plate_id = cy % actual_num_plates;
        
        bool is_water_biome = (c.biome == 0 || c.biome == 13 || c.biome == 14);
        
        if (is_water_biome) {
            c.elevation = ocean_level - 0.15f;
        } else {
            c.elevation = ocean_level + 0.10f;
        }
        c.temperature = (float)cx / (width - 1);
        c.moisture = (float)cy / (height - 1);
        c.water_accumulation = 1.0f;
        c.uplift = 0.0f;
        c.river_next_idx = -1;
        c.is_river = false;
        c.overlay = "none";
        
        switch (col_feature) {
            case 0:
                break;
            case 1:
                if (!is_water_biome) {
                    c.elevation = ocean_level + 0.20f;
                    c.uplift = 0.40f;
                    c.overlay = "hill";
                }
                break;
            case 2:
                if (!is_water_biome) {
                    c.elevation = 0.78f;
                    c.uplift = 0.60f;
                    c.overlay = "low_mountain";
                }
                break;
            case 3:
                if (!is_water_biome) {
                    c.elevation = 0.90f;
                    c.uplift = 0.85f;
                    c.overlay = "high_mountain";
                }
                break;
            case 4:
                if (!is_water_biome) {
                    c.elevation = ocean_level + 0.15f;
                    c.water_accumulation = 1.0f;
                    c.is_river = false;
                    c.overlay = "hill";
                }
                break;
            case 5:
                if (!is_water_biome) {
                    c.elevation = ocean_level + 0.05f;
                    c.water_accumulation = 25.0f;
                    c.is_river = true;
                }
                break;
            case 6:
                if (is_water_biome) {
                    c.elevation = -0.20f;
                } else {
                    c.elevation = 1.15f;
                }
                break;
            default:
                break;
        }
    }

    for (int row = 0; row < height; ++row) {
        int src_col = 4;
        int dst_col = 5;
        if (src_col < width && dst_col < width) {
            int src_idx = row * width + src_col;
            int dst_idx = row * width + dst_col;
            
            if (cells[src_idx].elevation >= ocean_level && cells[dst_idx].elevation >= ocean_level) {
                cells[src_idx].river_next_idx = dst_idx;
                cells[src_idx].is_river = true;
                cells[dst_idx].is_river = true;
                cells[dst_idx].water_accumulation = 25.0f;
            }
        }
    }

    step_generate_region_names();
    step_generate_holdings();
}



void HexcrawlMapGenerator::_bind_methods() {
    ClassDB::bind_method(D_METHOD("initialize", "width", "height", "seed"), &HexcrawlMapGenerator::initialize);
    ClassDB::bind_method(D_METHOD("run_full_pipeline"), &HexcrawlMapGenerator::run_full_pipeline);
    ClassDB::bind_method(D_METHOD("generate_debug_map"), &HexcrawlMapGenerator::generate_debug_map);
    ClassDB::bind_method(D_METHOD("step_generation", "step_index"), &HexcrawlMapGenerator::step_generation);
    ClassDB::bind_method(D_METHOD("step_generate_holdings"), &HexcrawlMapGenerator::step_generate_holdings);
    ClassDB::bind_method(D_METHOD("get_holding_data", "holding_id"), &HexcrawlMapGenerator::get_holding_data);
    ClassDB::bind_method(D_METHOD("enter_holding", "holding_id"), &HexcrawlMapGenerator::enter_holding);
    ClassDB::bind_method(D_METHOD("move_player_dir", "dir_val"), &HexcrawlMapGenerator::move_player_dir);
    ClassDB::bind_method(D_METHOD("get_player_current_room_data"), &HexcrawlMapGenerator::get_player_current_room_data);
    ClassDB::bind_method(D_METHOD("is_player_in_holding"), &HexcrawlMapGenerator::is_player_in_holding);
    ClassDB::bind_method(D_METHOD("exit_holding"), &HexcrawlMapGenerator::exit_holding);
    ClassDB::bind_method(D_METHOD("get_plate_velocity", "plate_id"), &HexcrawlMapGenerator::get_plate_velocity);

    ClassDB::bind_method(D_METHOD("get_cell_count"), &HexcrawlMapGenerator::get_cell_count);
    ClassDB::bind_method(D_METHOD("get_cell_data", "idx"), &HexcrawlMapGenerator::get_cell_data);
    ClassDB::bind_method(D_METHOD("get_river_paths"), &HexcrawlMapGenerator::get_river_paths);
    ClassDB::bind_method(D_METHOD("get_regions"), &HexcrawlMapGenerator::get_regions);

    ClassDB::bind_method(D_METHOD("get_elevations"), &HexcrawlMapGenerator::get_elevations);
    ClassDB::bind_method(D_METHOD("get_overlays"), &HexcrawlMapGenerator::get_overlays);
    ClassDB::bind_method(D_METHOD("get_moistures"), &HexcrawlMapGenerator::get_moistures);
    ClassDB::bind_method(D_METHOD("get_temperatures"), &HexcrawlMapGenerator::get_temperatures);
    ClassDB::bind_method(D_METHOD("get_biomes"), &HexcrawlMapGenerator::get_biomes);
    ClassDB::bind_method(D_METHOD("get_plates"), &HexcrawlMapGenerator::get_plates);
    ClassDB::bind_method(D_METHOD("get_river_next"), &HexcrawlMapGenerator::get_river_next);
    ClassDB::bind_method(D_METHOD("get_water_accumulations"), &HexcrawlMapGenerator::get_water_accumulations);

    // Math/Grid helpers exposed to GDScript
    ClassDB::bind_method(D_METHOD("get_neighbors", "idx"), &HexcrawlMapGenerator::get_neighbors);
    ClassDB::bind_method(D_METHOD("get_cell_position", "idx"), &HexcrawlMapGenerator::get_cell_position);
    ClassDB::bind_method(D_METHOD("is_shallow_water", "idx"), &HexcrawlMapGenerator::is_shallow_water);

    // Getters and Setters for properties
    ClassDB::bind_method(D_METHOD("get_width"), &HexcrawlMapGenerator::get_width);
    ClassDB::bind_method(D_METHOD("set_width", "val"), &HexcrawlMapGenerator::set_width);
    ClassDB::bind_method(D_METHOD("get_height"), &HexcrawlMapGenerator::get_height);
    ClassDB::bind_method(D_METHOD("set_height", "val"), &HexcrawlMapGenerator::set_height);
    ClassDB::bind_method(D_METHOD("get_seed"), &HexcrawlMapGenerator::get_seed);
    ClassDB::bind_method(D_METHOD("set_seed", "val"), &HexcrawlMapGenerator::set_seed);
    ClassDB::bind_method(D_METHOD("get_num_plates"), &HexcrawlMapGenerator::get_num_plates);
    ClassDB::bind_method(D_METHOD("set_num_plates", "val"), &HexcrawlMapGenerator::set_num_plates);
    ClassDB::bind_method(D_METHOD("get_ocean_level"), &HexcrawlMapGenerator::get_ocean_level);
    ClassDB::bind_method(D_METHOD("set_ocean_level", "val"), &HexcrawlMapGenerator::set_ocean_level);
    ClassDB::bind_method(D_METHOD("get_noise_scale"), &HexcrawlMapGenerator::get_noise_scale);
    ClassDB::bind_method(D_METHOD("set_noise_scale", "val"), &HexcrawlMapGenerator::set_noise_scale);
    ClassDB::bind_method(D_METHOD("get_noise_octaves"), &HexcrawlMapGenerator::get_noise_octaves);
    ClassDB::bind_method(D_METHOD("set_noise_octaves", "val"), &HexcrawlMapGenerator::set_noise_octaves);
    ClassDB::bind_method(D_METHOD("get_noise_persistence"), &HexcrawlMapGenerator::get_noise_persistence);
    ClassDB::bind_method(D_METHOD("set_noise_persistence", "val"), &HexcrawlMapGenerator::set_noise_persistence);
    ClassDB::bind_method(D_METHOD("get_noise_lacunarity"), &HexcrawlMapGenerator::get_noise_lacunarity);
    ClassDB::bind_method(D_METHOD("set_noise_lacunarity", "val"), &HexcrawlMapGenerator::set_noise_lacunarity);
    ClassDB::bind_method(D_METHOD("get_erosion_iterations"), &HexcrawlMapGenerator::get_erosion_iterations);
    ClassDB::bind_method(D_METHOD("set_erosion_iterations", "val"), &HexcrawlMapGenerator::set_erosion_iterations);
    ClassDB::bind_method(D_METHOD("get_erosion_rate"), &HexcrawlMapGenerator::get_erosion_rate);
    ClassDB::bind_method(D_METHOD("set_erosion_rate", "val"), &HexcrawlMapGenerator::set_erosion_rate);
    ClassDB::bind_method(D_METHOD("get_deposition_rate"), &HexcrawlMapGenerator::get_deposition_rate);
    ClassDB::bind_method(D_METHOD("set_deposition_rate", "val"), &HexcrawlMapGenerator::set_deposition_rate);
    ClassDB::bind_method(D_METHOD("get_precipitation_factor"), &HexcrawlMapGenerator::get_precipitation_factor);
    ClassDB::bind_method(D_METHOD("set_precipitation_factor", "val"), &HexcrawlMapGenerator::set_precipitation_factor);
    ClassDB::bind_method(D_METHOD("get_lapse_rate"), &HexcrawlMapGenerator::get_lapse_rate);
    ClassDB::bind_method(D_METHOD("set_lapse_rate", "val"), &HexcrawlMapGenerator::set_lapse_rate);
    ClassDB::bind_method(D_METHOD("get_wind_direction"), &HexcrawlMapGenerator::get_wind_direction);
    ClassDB::bind_method(D_METHOD("set_wind_direction", "val"), &HexcrawlMapGenerator::set_wind_direction);

    // Advanced properties getters and setters
    ClassDB::bind_method(D_METHOD("get_plate_speed_min"), &HexcrawlMapGenerator::get_plate_speed_min);
    ClassDB::bind_method(D_METHOD("set_plate_speed_min", "val"), &HexcrawlMapGenerator::set_plate_speed_min);
    ClassDB::bind_method(D_METHOD("get_plate_speed_max"), &HexcrawlMapGenerator::get_plate_speed_max);
    ClassDB::bind_method(D_METHOD("set_plate_speed_max", "val"), &HexcrawlMapGenerator::set_plate_speed_max);
    ClassDB::bind_method(D_METHOD("get_uplift_collision_factor"), &HexcrawlMapGenerator::get_uplift_collision_factor);
    ClassDB::bind_method(D_METHOD("set_uplift_collision_factor", "val"), &HexcrawlMapGenerator::set_uplift_collision_factor);
    ClassDB::bind_method(D_METHOD("get_uplift_separation_factor"), &HexcrawlMapGenerator::get_uplift_separation_factor);
    ClassDB::bind_method(D_METHOD("set_uplift_separation_factor", "val"), &HexcrawlMapGenerator::set_uplift_separation_factor);
    ClassDB::bind_method(D_METHOD("get_uplift_shear_factor"), &HexcrawlMapGenerator::get_uplift_shear_factor);
    ClassDB::bind_method(D_METHOD("set_uplift_shear_factor", "val"), &HexcrawlMapGenerator::set_uplift_shear_factor);
    ClassDB::bind_method(D_METHOD("get_uplift_decay"), &HexcrawlMapGenerator::get_uplift_decay);
    ClassDB::bind_method(D_METHOD("set_uplift_decay", "val"), &HexcrawlMapGenerator::set_uplift_decay);
    ClassDB::bind_method(D_METHOD("get_noise_amplitude"), &HexcrawlMapGenerator::get_noise_amplitude);
    ClassDB::bind_method(D_METHOD("set_noise_amplitude", "val"), &HexcrawlMapGenerator::set_noise_amplitude);
    ClassDB::bind_method(D_METHOD("get_erosion_capacity_factor"), &HexcrawlMapGenerator::get_erosion_capacity_factor);
    ClassDB::bind_method(D_METHOD("set_erosion_capacity_factor", "val"), &HexcrawlMapGenerator::set_erosion_capacity_factor);
    ClassDB::bind_method(D_METHOD("get_gravity_factor"), &HexcrawlMapGenerator::get_gravity_factor);
    ClassDB::bind_method(D_METHOD("set_gravity_factor", "val"), &HexcrawlMapGenerator::set_gravity_factor);
    ClassDB::bind_method(D_METHOD("get_water_evaporation_rate"), &HexcrawlMapGenerator::get_water_evaporation_rate);
    ClassDB::bind_method(D_METHOD("set_water_evaporation_rate", "val"), &HexcrawlMapGenerator::set_water_evaporation_rate);
    ClassDB::bind_method(D_METHOD("get_river_threshold"), &HexcrawlMapGenerator::get_river_threshold);
    ClassDB::bind_method(D_METHOD("set_river_threshold", "val"), &HexcrawlMapGenerator::set_river_threshold);
    ClassDB::bind_method(D_METHOD("get_base_moisture"), &HexcrawlMapGenerator::get_base_moisture);
    ClassDB::bind_method(D_METHOD("set_base_moisture", "val"), &HexcrawlMapGenerator::set_base_moisture);
    ClassDB::bind_method(D_METHOD("get_moisture_retention_rate"), &HexcrawlMapGenerator::get_moisture_retention_rate);
    ClassDB::bind_method(D_METHOD("set_moisture_retention_rate", "val"), &HexcrawlMapGenerator::set_moisture_retention_rate);
    ClassDB::bind_method(D_METHOD("get_shallow_water_depth"), &HexcrawlMapGenerator::get_shallow_water_depth);
    ClassDB::bind_method(D_METHOD("set_shallow_water_depth", "val"), &HexcrawlMapGenerator::set_shallow_water_depth);
    ClassDB::bind_method(D_METHOD("get_beach_elevation_threshold"), &HexcrawlMapGenerator::get_beach_elevation_threshold);
    ClassDB::bind_method(D_METHOD("set_beach_elevation_threshold", "val"), &HexcrawlMapGenerator::set_beach_elevation_threshold);

    // Properties
    ADD_PROPERTY(PropertyInfo(Variant::INT, "width"), "set_width", "get_width");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "height"), "set_height", "get_height");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "seed"), "set_seed", "get_seed");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "num_plates"), "set_num_plates", "get_num_plates");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "ocean_level"), "set_ocean_level", "get_ocean_level");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "noise_scale"), "set_noise_scale", "get_noise_scale");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "noise_octaves"), "set_noise_octaves", "get_noise_octaves");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "noise_persistence"), "set_noise_persistence", "get_noise_persistence");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "noise_lacunarity"), "set_noise_lacunarity", "get_noise_lacunarity");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "erosion_iterations"), "set_erosion_iterations", "get_erosion_iterations");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "erosion_rate"), "set_erosion_rate", "get_erosion_rate");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "deposition_rate"), "set_deposition_rate", "get_deposition_rate");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "precipitation_factor"), "set_precipitation_factor", "get_precipitation_factor");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "lapse_rate"), "set_lapse_rate", "get_lapse_rate");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "wind_direction"), "set_wind_direction", "get_wind_direction");

    // Advanced properties bindings
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "plate_speed_min"), "set_plate_speed_min", "get_plate_speed_min");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "plate_speed_max"), "set_plate_speed_max", "get_plate_speed_max");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "uplift_collision_factor"), "set_uplift_collision_factor", "get_uplift_collision_factor");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "uplift_separation_factor"), "set_uplift_separation_factor", "get_uplift_separation_factor");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "uplift_shear_factor"), "set_uplift_shear_factor", "get_uplift_shear_factor");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "uplift_decay"), "set_uplift_decay", "get_uplift_decay");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "noise_amplitude"), "set_noise_amplitude", "get_noise_amplitude");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "erosion_capacity_factor"), "set_erosion_capacity_factor", "get_erosion_capacity_factor");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "gravity_factor"), "set_gravity_factor", "get_gravity_factor");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "water_evaporation_rate"), "set_water_evaporation_rate", "get_water_evaporation_rate");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "river_threshold"), "set_river_threshold", "get_river_threshold");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "base_moisture"), "set_base_moisture", "get_base_moisture");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "moisture_retention_rate"), "set_moisture_retention_rate", "get_moisture_retention_rate");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "shallow_water_depth"), "set_shallow_water_depth", "get_shallow_water_depth");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "beach_elevation_threshold"), "set_beach_elevation_threshold", "get_beach_elevation_threshold");
}

