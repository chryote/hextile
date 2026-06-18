#ifndef NAME_GENERATOR_H
#define NAME_GENERATOR_H

#include <godot_cpp/variant/string.hpp>
#include <vector>
#include <random>

using namespace godot;

class NameGenerator {
private:
    static const std::vector<const char*> land_prefixes;
    static const std::vector<const char*> land_suffixes;
    
    static const std::vector<const char*> island_adjectives;
    static const std::vector<const char*> island_nouns;
    
    static const std::vector<const char*> mountain_prefixes;
    static const std::vector<const char*> mountain_nouns;
    
    static const std::vector<const char*> river_adjectives;
    static const std::vector<const char*> river_nouns;
    
    static const std::vector<const char*> forest_prefixes;
    static const std::vector<const char*> forest_suffixes;
    
    static const std::vector<const char*> desert_prefixes;
    static const std::vector<const char*> desert_suffixes;
    
    static const std::vector<const char*> grassland_prefixes;
    static const std::vector<const char*> grassland_suffixes;
    
    static const std::vector<const char*> tundra_prefixes;
    static const std::vector<const char*> tundra_suffixes;
    
    static const std::vector<const char*> water_prefixes;
    static const std::vector<const char*> water_suffixes;

    static const std::vector<const char*> micro_prefixes;
    static const std::vector<const char*> micro_suffixes;

public:
    String generate_landmass_name(std::mt19937 &rng);
    String generate_island_name(std::mt19937 &rng);
    String generate_mountain_name(std::mt19937 &rng);
    String generate_river_name(std::mt19937 &rng);
    String generate_biome_region_name(int biome_id, std::mt19937 &rng);
    String generate_micro_region_name(std::mt19937 &rng);
};

#endif // NAME_GENERATOR_H
