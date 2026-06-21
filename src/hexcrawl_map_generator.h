#ifndef HEXCRAWL_MAP_GENERATOR_H
#define HEXCRAWL_MAP_GENERATOR_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/string.hpp>
#include "hextile_framework.h"
#include <vector>
#include <random>
#include <unordered_map>

using namespace godot;

class HexcrawlMapGenerator : public RefCounted {
    GDCLASS(HexcrawlMapGenerator, RefCounted);

private:
    int width = 60;
    int height = 40;
    int seed = 12345;
    int num_plates = 10;
    float ocean_level = 0.35f;
    float noise_scale = 0.08f;
    int noise_octaves = 4;
    float noise_persistence = 0.5f;
    float noise_lacunarity = 2.0f;
    int erosion_iterations = 50000;
    float erosion_rate = 0.05f;
    float deposition_rate = 0.05f;
    float precipitation_factor = 1.5f;
    float lapse_rate = 0.4f;
    float wind_direction = 0.0f;

    // Advanced simulation properties
    float plate_speed_min = 0.5f;
    float plate_speed_max = 2.0f;
    float uplift_collision_factor = 1.5f;
    float uplift_separation_factor = -0.8f;
    float uplift_shear_factor = 0.4f;
    float uplift_decay = 0.58f;
    float noise_amplitude = 0.35f;
    float erosion_capacity_factor = 0.12f;
    float gravity_factor = 9.8f;
    float water_evaporation_rate = 0.05f;
    float river_threshold = 12.0f;
    float base_moisture = 0.4f;
    float moisture_retention_rate = 0.96f;
    float shallow_water_depth = 0.08f;
    float beach_elevation_threshold = 0.02f;

    std::vector<HexCell> cells;
    std::vector<Plate> plates;
    std::vector<Region> regions;
    std::mt19937 rng;

    std::unordered_map<int, Holding> global_holdings;
    int player_current_holding_id = -1;
    int player_current_room_id = -1;

    ObjectDictionaryManager object_manager;

    void compute_overlays();
    String execute_interaction_on_list(std::vector<ActiveObjectInstance>& interactables, const String& command);
    std::vector<std::string> get_filtered_templates_for_cell(const HexCell& cell);

protected:
    static void _bind_methods();

public:
    HexcrawlMapGenerator();
    ~HexcrawlMapGenerator();

    void initialize(int p_width, int p_height, int p_seed);
    void run_full_pipeline();
    void generate_debug_map();
    void step_generation(int step_index);

    // Steps of the pipeline
    void step_generate_plates();
    void step_compute_uplift();
    void step_add_base_noise();
    void step_run_erosion();
    void step_generate_rivers();
    void step_simulate_climate();
    void step_assign_biomes();
    void step_generate_region_names();
    void step_generate_holdings();
    void build_mud_rooms_for_holding(Holding& holding, const HexCell& cell);

    // Traversal & query helpers
    Dictionary get_holding_data(int holding_id) const;
    bool enter_holding(int holding_id);
    bool move_player_dir(int dir_val);
    Dictionary get_player_current_room_data() const;
    bool is_player_in_holding() const;
    void exit_holding();
    String interact_in_room(const String& command);
    String interact_on_cell(int cell_idx, const String& command);

    // Math/Grid helpers exposed to GDScript
    std::vector<int> get_neighbors_internal(int idx) const;
    PackedInt32Array get_neighbors(int idx) const;
    Vector2 get_cell_position(int idx) const;
    bool is_shallow_water(int idx) const;

    // Data getters
    int get_cell_count() const { return (int)cells.size(); }
    Dictionary get_cell_data(int idx) const;
    Array get_river_paths() const;
    Vector2 get_plate_velocity(int plate_id) const;
    Array get_regions() const;

    PackedFloat32Array get_elevations() const;
    PackedFloat32Array get_moistures() const;
    PackedFloat32Array get_temperatures() const;
    PackedInt32Array get_biomes() const;
    PackedInt32Array get_plates() const;
    PackedInt32Array get_river_next() const;
    PackedFloat32Array get_water_accumulations() const;
    PackedStringArray get_overlays() const;

    // Getters / Setters for configuration
    int get_width() const { return width; }
    void set_width(int val) { width = val; }

    int get_height() const { return height; }
    void set_height(int val) { height = val; }

    int get_seed() const { return seed; }
    void set_seed(int val) { seed = val; }

    int get_num_plates() const { return num_plates; }
    void set_num_plates(int val) { num_plates = val; }

    float get_ocean_level() const { return ocean_level; }
    void set_ocean_level(float val) { ocean_level = val; }

    float get_noise_scale() const { return noise_scale; }
    void set_noise_scale(float val) { noise_scale = val; }

    int get_noise_octaves() const { return noise_octaves; }
    void set_noise_octaves(int val) { noise_octaves = val; }

    float get_noise_persistence() const { return noise_persistence; }
    void set_noise_persistence(float val) { noise_persistence = val; }

    float get_noise_lacunarity() const { return noise_lacunarity; }
    void set_noise_lacunarity(float val) { noise_lacunarity = val; }

    int get_erosion_iterations() const { return erosion_iterations; }
    void set_erosion_iterations(int val) { erosion_iterations = val; }

    float get_erosion_rate() const { return erosion_rate; }
    void set_erosion_rate(float val) { erosion_rate = val; }

    float get_deposition_rate() const { return deposition_rate; }
    void set_deposition_rate(float val) { deposition_rate = val; }

    float get_precipitation_factor() const { return precipitation_factor; }
    void set_precipitation_factor(float val) { precipitation_factor = val; }

    float get_lapse_rate() const { return lapse_rate; }
    void set_lapse_rate(float val) { lapse_rate = val; }

    float get_wind_direction() const { return wind_direction; }
    void set_wind_direction(float val) { wind_direction = val; }

    // Advanced properties getters/setters
    float get_plate_speed_min() const { return plate_speed_min; }
    void set_plate_speed_min(float val) { plate_speed_min = val; }

    float get_plate_speed_max() const { return plate_speed_max; }
    void set_plate_speed_max(float val) { plate_speed_max = val; }

    float get_uplift_collision_factor() const { return uplift_collision_factor; }
    void set_uplift_collision_factor(float val) { uplift_collision_factor = val; }

    float get_uplift_separation_factor() const { return uplift_separation_factor; }
    void set_uplift_separation_factor(float val) { uplift_separation_factor = val; }

    float get_uplift_shear_factor() const { return uplift_shear_factor; }
    void set_uplift_shear_factor(float val) { uplift_shear_factor = val; }

    float get_uplift_decay() const { return uplift_decay; }
    void set_uplift_decay(float val) { uplift_decay = val; }

    float get_noise_amplitude() const { return noise_amplitude; }
    void set_noise_amplitude(float val) { noise_amplitude = val; }

    float get_erosion_capacity_factor() const { return erosion_capacity_factor; }
    void set_erosion_capacity_factor(float val) { erosion_capacity_factor = val; }

    float get_gravity_factor() const { return gravity_factor; }
    void set_gravity_factor(float val) { gravity_factor = val; }

    float get_water_evaporation_rate() const { return water_evaporation_rate; }
    void set_water_evaporation_rate(float val) { water_evaporation_rate = val; }

    float get_river_threshold() const { return river_threshold; }
    void set_river_threshold(float val) { river_threshold = val; }

    float get_base_moisture() const { return base_moisture; }
    void set_base_moisture(float val) { base_moisture = val; }

    float get_moisture_retention_rate() const { return moisture_retention_rate; }
    void set_moisture_retention_rate(float val) { moisture_retention_rate = val; }

    float get_shallow_water_depth() const { return shallow_water_depth; }
    void set_shallow_water_depth(float val) { shallow_water_depth = val; }

    float get_beach_elevation_threshold() const { return beach_elevation_threshold; }
    void set_beach_elevation_threshold(float val) { beach_elevation_threshold = val; }
};

#endif // HEXCRAWL_MAP_GENERATOR_H
