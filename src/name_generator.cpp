#include "name_generator.h"

const std::vector<const char*> NameGenerator::land_prefixes = {"Eld", "Val", "Gon", "Thor", "Ar", "Bel", "Khar", "Ost", "Sol", "Zul", "West", "North", "Ost", "Lith"};
const std::vector<const char*> NameGenerator::land_suffixes = {"oria", "gard", "ia", "alis", "anthia", "mor", "heim", "ost", "lund", "beck", "ford", "wood", "shire"};

const std::vector<const char*> NameGenerator::island_adjectives = {"Misty", "Lonely", "Stormy", "Shadowy", "Whispering", "Silent", "Savage", "Golden", "Emerald", "Frozen"};
const std::vector<const char*> NameGenerator::island_nouns = {"Isle", "Cay", "Rock", "Reef", "Skerry", "Atoll", "Haven", "Crest"};

const std::vector<const char*> NameGenerator::mountain_prefixes = {"Frost", "Iron", "Storm", "Doom", "Wyrm", "Shadow", "Thunder", "Crag", "Cloud", "Spire"};
const std::vector<const char*> NameGenerator::mountain_nouns = {"Peaks", "Spire", "Crags", "Range", "Heights", "Hills", "Tops", "Crowns"};

const std::vector<const char*> NameGenerator::river_adjectives = {"Silver", "Whispering", "Roaring", "Cold", "Swift", "Deep", "Shimmering", "Shadow", "Silent", "Wild"};
const std::vector<const char*> NameGenerator::river_nouns = {"River", "flow", "Fork", "Creek", "Run", "Stream", "Waters"};

const std::vector<const char*> NameGenerator::forest_prefixes = {"Shadow", "Whisper", "Green", "Timber", "Elder", "Oak", "Pine", "Tangle", "Wildwood", "Silent"};
const std::vector<const char*> NameGenerator::forest_suffixes = {"wood", "forest", "canopy", "grove", "reach", "thicket", "shroud"};

const std::vector<const char*> NameGenerator::desert_prefixes = {"Sunburned", "Dust", "Red", "Arid", "Scorched", "Dry", "Shifting"};
const std::vector<const char*> NameGenerator::desert_suffixes = {"Sands", "Wastes", "Basin", "Flats", "Dunes"};

const std::vector<const char*> NameGenerator::grassland_prefixes = {"Windward", "Green", "Sun", "High", "Rolling", "Wide"};
const std::vector<const char*> NameGenerator::grassland_suffixes = {"Plains", "Meadows", "Steppe", "Downs", "Pastures"};

const std::vector<const char*> NameGenerator::tundra_prefixes = {"Frost", "Grey", "Ice", "Boreal", "Bleak"};
const std::vector<const char*> NameGenerator::tundra_suffixes = {"Mire", "Wastes", "Tundra", "Flats"};

const std::vector<const char*> NameGenerator::water_prefixes = {"Mirror", "Calm", "Deep", "Blue", "Still", "Quiet"};
const std::vector<const char*> NameGenerator::water_suffixes = {"Lake", "Basin", "Bay", "Pool", "Waters"};

const std::vector<const char*> NameGenerator::micro_prefixes = {"Hidden", "Gallows", "Falcon", "Hermit", "Deadman", "Eagle", "Old"};
const std::vector<const char*> NameGenerator::micro_suffixes = {"Glade", "Hill", "Roost", "Glen", "Hollow", "Ridge", "Dell"};

String NameGenerator::generate_landmass_name(std::mt19937 &rng) {
    int p_idx = rng() % land_prefixes.size();
    int s_idx = rng() % land_suffixes.size();
    return String(land_prefixes[p_idx]) + String(land_suffixes[s_idx]);
}

String NameGenerator::generate_island_name(std::mt19937 &rng) {
    int adj_idx = rng() % island_adjectives.size();
    int n_idx = rng() % island_nouns.size();
    if (rng() % 2 == 0) {
        return String(island_nouns[n_idx]) + " of " + String(island_adjectives[adj_idx]);
    } else {
        return String(island_adjectives[adj_idx]) + " " + String(island_nouns[n_idx]);
    }
}

String NameGenerator::generate_mountain_name(std::mt19937 &rng) {
    int p_idx = rng() % mountain_prefixes.size();
    int n_idx = rng() % mountain_nouns.size();
    return "The " + String(mountain_prefixes[p_idx]) + " " + String(mountain_nouns[n_idx]);
}

String NameGenerator::generate_river_name(std::mt19937 &rng) {
    int adj_idx = rng() % river_adjectives.size();
    int n_idx = rng() % river_nouns.size();
    String suf = river_nouns[n_idx];
    if (suf == "flow" || suf == "run") {
        return String(river_adjectives[adj_idx]) + suf;
    } else {
        return "The " + String(river_adjectives[adj_idx]) + " " + suf;
    }
}

String NameGenerator::generate_biome_region_name(int biome_id, std::mt19937 &rng) {
    const std::vector<const char*> *prefixes = &micro_prefixes;
    const std::vector<const char*> *suffixes = &micro_suffixes;
    
    if (biome_id == 0 || biome_id == 13) {
        prefixes = &water_prefixes;
        suffixes = &water_suffixes;
    } else if (biome_id == 14) {
        prefixes = &water_prefixes;
        suffixes = &water_suffixes;
    } else if (biome_id == 1 || biome_id == 15 || biome_id == 9 || biome_id == 5) {
        prefixes = &desert_prefixes;
        suffixes = &desert_suffixes;
    } else if (biome_id == 2 || biome_id == 3) {
        prefixes = &tundra_prefixes;
        suffixes = &tundra_suffixes;
    } else if (biome_id == 4 || biome_id == 7 || biome_id == 8 || biome_id == 11 || biome_id == 12) {
        prefixes = &forest_prefixes;
        suffixes = &forest_suffixes;
    } else if (biome_id == 6 || biome_id == 10) {
        prefixes = &grassland_prefixes;
        suffixes = &grassland_suffixes;
    }
    
    int p_idx = rng() % prefixes->size();
    int s_idx = rng() % suffixes->size();
    
    String suf = (*suffixes)[s_idx];
    if (suf == "wood" || suf == "forest" || suf == "reach" || suf == "grove" || suf == "thicket" || suf == "shroud" || suf == "flow" || suf == "run") {
        return String((*prefixes)[p_idx]) + suf;
    } else {
        return "The " + String((*prefixes)[p_idx]) + " " + suf;
    }
}

String NameGenerator::generate_micro_region_name(std::mt19937 &rng) {
    int p_idx = rng() % micro_prefixes.size();
    int s_idx = rng() % micro_suffixes.size();
    return "The " + String(micro_prefixes[p_idx]) + " " + String(micro_suffixes[s_idx]);
}
