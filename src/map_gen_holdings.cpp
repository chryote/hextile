#include "hexcrawl_map_generator.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <random>

enum class BiomeEnv {
    Glacial,
    Desert,
    Forest,
    Grassland,
    Coastal
};

static BiomeEnv get_biome_env(int biome) {
    switch (biome) {
        case 2: case 3:
            return BiomeEnv::Glacial;
        case 5: case 9:
            return BiomeEnv::Desert;
        case 4: case 7: case 8: case 11: case 12:
            return BiomeEnv::Forest;
        case 6: case 10:
            return BiomeEnv::Grassland;
        case 1: case 15:
        default:
            return BiomeEnv::Coastal;
    }
}

void HexcrawlMapGenerator::step_generate_holdings() {
    int global_holding_counter = 0;
    global_holdings.clear();
    player_current_holding_id = -1;
    player_current_room_id = -1;

    std::mt19937 step_rng(seed + 12345);

    static const std::vector<std::string> adjectives = {
        "Silent", "Shadowy", "Ancient", "Forgotten", "Lost", "Mysterious", "Sacred",
        "Desolate", "Forbidden", "Hidden", "Quiet", "Echoing", "Shrouded", "Grim",
        "Eldritch", "Vast"
    };
    static const std::vector<std::string> nouns = {
        "Hollow", "Sanctuary", "Reach", "Expanse", "Depths", "Crossing", "Haven",
        "Domain", "Vantage", "Rest", "Passage", "Boundary"
    };

    for (auto& cell : cells) {
        if (cell.elevation < ocean_level) continue;

        Holding new_holding;
        new_holding.id = global_holding_counter++;
        new_holding.parent_hex_idx = cell.index;
        new_holding.type = HoldingType::Wilderness;
        
        std::string adj = adjectives[step_rng() % adjectives.size()];
        std::string noun = nouns[step_rng() % nouns.size()];
        new_holding.name = adj + " " + noun;

        build_mud_rooms_for_holding(new_holding, cell);

        global_holdings[new_holding.id] = new_holding;
        
        cell.holding_count = 0;
        cell.holding_ids[cell.holding_count++] = new_holding.id;
    }
}

void HexcrawlMapGenerator::build_mud_rooms_for_holding(Holding& holding, const HexCell& cell) {
    if (holding.type == HoldingType::Wilderness) {
        BiomeEnv env = get_biome_env(cell.biome);

        // Room 0: Trail Entry
        SubArea trail;
        trail.local_id = 0;
        trail.tags = Tags::None;
        trail.exits[static_cast<size_t>(Direction::North)] = 1;

        // Room 1: Feature Room (Riverbank vs Glade)
        SubArea feature_room;
        feature_room.local_id = 1;
        feature_room.tags = Tags::Lootable;
        feature_room.exits[static_cast<size_t>(Direction::South)] = 0;
        feature_room.exits[static_cast<size_t>(Direction::East)] = 2;

        // Room 2: Cave Mouth
        SubArea cave_mouth;
        cave_mouth.local_id = 2;
        cave_mouth.tags = Tags::None;
        cave_mouth.exits[static_cast<size_t>(Direction::West)] = 1;
        cave_mouth.exits[static_cast<size_t>(Direction::Down)] = 3;

        // Room 3: Cavern (Underground)
        SubArea cavern;
        cavern.local_id = 3;
        cavern.tags = Tags::Underground | Tags::Lootable;
        cavern.exits[static_cast<size_t>(Direction::Up)] = 2;

        // Spawns if Mountain
        bool is_mountain = (cell.overlay == "high_mountain" || cell.overlay == "low_mountain");
        SubArea summit;
        if (is_mountain) {
            summit.local_id = 4;
            summit.tags = Tags::None;
            summit.exits[static_cast<size_t>(Direction::Down)] = 2;
            cave_mouth.exits[static_cast<size_t>(Direction::Up)] = 4;
        }

        // Apply environmental content
        switch (env) {
            case BiomeEnv::Forest:
                trail.name = "Overgrown Forest Trail";
                trail.description = "A narrow game trail winding through dense, ancient trunks. Sunbeams filter through a thick, mossy canopy.";
                
                if (cell.is_river) {
                    feature_room.name = "Mossy Riverbank";
                    feature_room.description = "A rushing freshwater river flows through the trees. The banks are slick with wet moss and ferns.";
                } else {
                    feature_room.name = "Sunlit Glade";
                    feature_room.description = "A quiet clearing where sunlight pours through a break in the trees. Wildflowers and forest berries grow here.";
                }

                cave_mouth.name = "Moss-Draped Cave Mouth";
                cave_mouth.description = "The dark opening of a natural stone cave, draped with hanging creepers and wet moss.";

                cavern.name = "Roots Cavern";
                cavern.description = "A damp underground chamber where ancient tree roots pierce the ceiling like wooden fingers.";

                if (is_mountain) {
                    summit.name = "Forest Canopy Overlook";
                    summit.description = "A high rocky summit offering an endless view over a green ocean of forest treetops stretching to the horizon.";
                }
                break;

            case BiomeEnv::Desert:
                trail.name = "Dusty Desert Path";
                trail.description = "A faint trail marked by animal tracks through shifting sand and sun-bleached rocks. The air is dry and shimmering with heat.";
                
                if (cell.is_river) {
                    feature_room.name = "Oasis Spring";
                    feature_room.description = "A rare, cool spring pool fed by underground waters. Palm trees and scrub grow near the refreshing shoreline.";
                    feature_room.tags |= Tags::Sanctuary;
                } else {
                    feature_room.name = "Rocky Hollow";
                    feature_room.description = "A shallow basin surrounded by dry boulders, offering brief shade from the intense sun.";
                }

                cave_mouth.name = "Sandstone Cave Entrance";
                cave_mouth.description = "A gaping hole in the sandstone cliff face. The interior is cool and shadows stretch deep inside.";

                cavern.name = "Dusty Sandstone Chamber";
                cavern.description = "A cool, dry underground cave with sand-swept floors and smooth, wind-carved rock formations.";

                if (is_mountain) {
                    summit.name = "Sun-Baked Ridge";
                    summit.description = "A blistering peak overlooking vast expanses of desert dunes and dry, shimmering canyons.";
                }
                break;

            case BiomeEnv::Glacial:
                trail.name = "Frozen Animal Track";
                trail.description = "A narrow trail pressed into deep snow and hard-packed ice. Cold wind howls across the desolate white landscape.";
                
                if (cell.is_river) {
                    feature_room.name = "Icy River Crossing";
                    feature_room.description = "A partially frozen river rushes through the ice. Huge shards of ice grind together in the cold flow.";
                } else {
                    feature_room.name = "Snowy Ravine";
                    feature_room.description = "A sheltered depression in the ice sheets, providing refuge from the biting glacial winds.";
                }

                cave_mouth.name = "Glacial Crevasse";
                cave_mouth.description = "A deep crack in the blue glacier ice. Frost covers the walls, leading down into dark depths.";

                cavern.name = "Frozen Ice Cave";
                cavern.description = "An underground chamber formed of solid, translucent blue ice. Icicles hang from the ceiling like sharp teeth.";

                if (is_mountain) {
                    summit.name = "Frozen Summit";
                    summit.description = "A windswept icy peak. From this freezing height, the world looks like an endless sheet of white.";
                }
                break;

            case BiomeEnv::Grassland:
                trail.name = "Wind-Swept Grassland Trail";
                trail.description = "A simple path worn into a sea of swaying tallgrass. The sky is open and wide overhead.";
                
                if (cell.is_river) {
                    feature_room.name = "Rushing Riverbank";
                    feature_room.description = "A wide river winds through the open plains, its waters clear, cool, and fast-moving.";
                } else {
                    feature_room.name = "Grassy Clearing";
                    feature_room.description = "A peaceful opening in the swaying grasses, marked by a ring of weathered stones.";
                }

                cave_mouth.name = "Rocky Cave Opening";
                cave_mouth.description = "A natural rock fissure in a low outcrop, leading down into darkness.";

                cavern.name = "Limestone Cave";
                cavern.description = "A dark underground room with limestone deposits and dripping stalactites.";

                if (is_mountain) {
                    summit.name = "Windswept Summit";
                    summit.description = "A high grassy peak where the winds blow fierce and clean, offering views of the flat lands below.";
                }
                break;

            case BiomeEnv::Coastal:
            default:
                trail.name = "Sandy Coastline Trail";
                trail.description = "A path winding through coastal dunes and scrub. The salt spray of the water hangs thick in the air.";
                
                if (cell.is_river) {
                    feature_room.name = "Tidal Creek Mouth";
                    feature_room.description = "A saltwater creek empties into the sand here, with small crabs scurrying along the muddy edges.";
                } else {
                    feature_room.name = "Sandy Basin";
                    feature_room.description = "A sandy depression between dunes, sheltered by windswept sea grass.";
                }

                cave_mouth.name = "Salt-Crusted Cave Entrance";
                cave_mouth.description = "A sea cave entrance worn into the coastal rock, the stones white with dried sea salt.";

                cavern.name = "Tidal Cavern";
                cavern.description = "A damp sea cavern where the sound of the ocean echoes off the walls. Tidepools remain in the depressions.";

                if (is_mountain) {
                    summit.name = "Ocean Cliff Summit";
                    summit.description = "A high cliff edge directly overlooking the coastline, with the wind bringing the roar of the surf from below.";
                }
                break;
        }

        // Apply slope/climb modifications to trail description
        if (cell.overlay == "hill") {
            trail.description += " The trail slopes gently upward over rolling rises.";
        } else if (is_mountain) {
            trail.description += " The trail winds steeply upward along a rugged, rocky cliffside.";
        }

        holding.sub_areas.push_back(trail);
        holding.sub_areas.push_back(feature_room);
        holding.sub_areas.push_back(cave_mouth);
        holding.sub_areas.push_back(cavern);
        if (is_mountain) {
            holding.sub_areas.push_back(summit);
        }
        holding.entry_sub_area_idx = 0;
    }
}

Dictionary HexcrawlMapGenerator::get_holding_data(int holding_id) const {
    Dictionary data;
    auto it = global_holdings.find(holding_id);
    if (it == global_holdings.end()) return data;

    const Holding& holding = it->second;
    data["id"] = holding.id;
    data["parent_hex_idx"] = holding.parent_hex_idx;
    data["name"] = String(holding.name.c_str());
    data["entry_sub_area_idx"] = holding.entry_sub_area_idx;

    String type_str = "Unknown";
    switch (holding.type) {
        case HoldingType::Wilderness: type_str = "Wilderness"; break;
        case HoldingType::Ruins: type_str = "Ruins"; break;
        case HoldingType::Camp: type_str = "Camp"; break;
        case HoldingType::Outpost: type_str = "Outpost"; break;
        case HoldingType::InlandSettlement: type_str = "Inland Settlement"; break;
        case HoldingType::HarborSettlement: type_str = "Harbor Settlement"; break;
        case HoldingType::Fortress: type_str = "Fortress"; break;
        case HoldingType::MerchantEnclave: type_str = "Merchant Enclave"; break;
        case HoldingType::Shrine: type_str = "Shrine"; break;
    }
    data["type"] = type_str;

    Array tags_arr;
    if (has_tag(holding.tags, Tags::Raidable)) tags_arr.append("Raidable");
    if (has_tag(holding.tags, Tags::Burnable)) tags_arr.append("Burnable");
    if (has_tag(holding.tags, Tags::Sanctuary)) tags_arr.append("Sanctuary");
    if (has_tag(holding.tags, Tags::Lootable)) tags_arr.append("Lootable");
    if (has_tag(holding.tags, Tags::Inhabited)) tags_arr.append("Inhabited");
    if (has_tag(holding.tags, Tags::Underground)) tags_arr.append("Underground");
    data["tags"] = tags_arr;

    Array sub_areas_arr;
    for (const auto& room : holding.sub_areas) {
        Dictionary r_data;
        r_data["local_id"] = room.local_id;
        r_data["name"] = String(room.name.c_str());
        r_data["description"] = String(room.description.c_str());
        
        Array r_tags;
        if (has_tag(room.tags, Tags::Raidable)) r_tags.append("Raidable");
        if (has_tag(room.tags, Tags::Burnable)) r_tags.append("Burnable");
        if (has_tag(room.tags, Tags::Sanctuary)) r_tags.append("Sanctuary");
        if (has_tag(room.tags, Tags::Lootable)) r_tags.append("Lootable");
        if (has_tag(room.tags, Tags::Inhabited)) r_tags.append("Inhabited");
        if (has_tag(room.tags, Tags::Underground)) r_tags.append("Underground");
        r_data["tags"] = r_tags;

        Dictionary exits_dict;
        static const char* dir_names[] = { "north", "south", "east", "west", "up", "down", "in", "out" };
        for (int d = 0; d < 8; ++d) {
            exits_dict[dir_names[d]] = room.exits[d];
        }
        r_data["exits"] = exits_dict;

        sub_areas_arr.append(r_data);
    }
    data["sub_areas"] = sub_areas_arr;

    return data;
}

bool HexcrawlMapGenerator::enter_holding(int holding_id) {
    auto it = global_holdings.find(holding_id);
    if (it == global_holdings.end()) return false;

    player_current_holding_id = holding_id;
    player_current_room_id = it->second.entry_sub_area_idx;
    return true;
}

bool HexcrawlMapGenerator::move_player_dir(int dir_val) {
    if (player_current_holding_id == -1 || player_current_room_id == -1) return false;
    if (dir_val < 0 || dir_val >= static_cast<int>(Direction::Count)) return false;

    auto it = global_holdings.find(player_current_holding_id);
    if (it == global_holdings.end()) return false;

    const Holding& holding = it->second;
    if (player_current_room_id < 0 || player_current_room_id >= (int)holding.sub_areas.size()) return false;

    const SubArea& current_room = holding.sub_areas[player_current_room_id];
    int next_room_id = current_room.exits[dir_val];
    if (next_room_id != -1 && next_room_id >= 0 && next_room_id < (int)holding.sub_areas.size()) {
        player_current_room_id = next_room_id;
        return true;
    }
    return false;
}

Dictionary HexcrawlMapGenerator::get_player_current_room_data() const {
    Dictionary data;
    if (player_current_holding_id == -1 || player_current_room_id == -1) {
        data["in_holding"] = false;
        return data;
    }

    auto it = global_holdings.find(player_current_holding_id);
    if (it == global_holdings.end()) {
        data["in_holding"] = false;
        return data;
    }

    const Holding& holding = it->second;
    if (player_current_room_id < 0 || player_current_room_id >= (int)holding.sub_areas.size()) {
        data["in_holding"] = false;
        return data;
    }

    const SubArea& room = holding.sub_areas[player_current_room_id];
    data["in_holding"] = true;
    data["holding_id"] = player_current_holding_id;
    data["holding_name"] = String(holding.name.c_str());
    data["room_id"] = player_current_room_id;
    data["room_name"] = String(room.name.c_str());
    data["room_description"] = String(room.description.c_str());

    Array r_tags;
    if (has_tag(room.tags, Tags::Raidable)) r_tags.append("Raidable");
    if (has_tag(room.tags, Tags::Burnable)) r_tags.append("Burnable");
    if (has_tag(room.tags, Tags::Sanctuary)) r_tags.append("Sanctuary");
    if (has_tag(room.tags, Tags::Lootable)) r_tags.append("Lootable");
    if (has_tag(room.tags, Tags::Inhabited)) r_tags.append("Inhabited");
    if (has_tag(room.tags, Tags::Underground)) r_tags.append("Underground");
    data["room_tags"] = r_tags;

    Dictionary exits_dict;
    static const char* dir_names[] = { "north", "south", "east", "west", "up", "down", "in", "out" };
    for (int d = 0; d < 8; ++d) {
        exits_dict[dir_names[d]] = room.exits[d];
    }
    data["exits"] = exits_dict;

    return data;
}

bool HexcrawlMapGenerator::is_player_in_holding() const {
    return player_current_holding_id != -1;
}

void HexcrawlMapGenerator::exit_holding() {
    player_current_holding_id = -1;
    player_current_room_id = -1;
}
