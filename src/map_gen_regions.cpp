#include "hexcrawl_map_generator.h"
#include "name_generator.h"
#include <vector>
#include <queue>
#include <cmath>
#include <algorithm>

void HexcrawlMapGenerator::step_generate_region_names() {
    regions.clear();
    for (size_t i = 0; i < cells.size(); ++i) {
        cells[i].landmass_id = -1;
        cells[i].biome_region_id = -1;
        cells[i].micro_region_id = -1;
        cells[i].mountain_range_id = -1;
        cells[i].river_id = -1;
    }

    std::mt19937 step_rng(seed + 999);
    NameGenerator name_gen;
    int next_region_id = 0;

    auto flood_fill = [&](int start_idx, std::vector<bool> &global_visited, auto criteria_func) -> std::vector<int> {
        std::vector<int> component;
        std::queue<int> q;

        q.push(start_idx);
        global_visited[start_idx] = true;

        while (!q.empty()) {
            int curr = q.front();
            q.pop();
            component.push_back(curr);

            std::vector<int> neighbors = get_neighbors_internal(curr);
            for (int n_idx : neighbors) {
                if (!global_visited[n_idx] && criteria_func(curr, n_idx)) {
                    global_visited[n_idx] = true;
                    q.push(n_idx);
                }
            }
        }
        return component;
    };

    // 1. Segment Landmasses (elevation >= ocean_level)
    std::vector<bool> visited_landmass(cells.size(), false);
    for (int i = 0; i < (int)cells.size(); ++i) {
        if (cells[i].elevation >= ocean_level && !visited_landmass[i]) {
            std::vector<int> component = flood_fill(i, visited_landmass, [&](int c, int n) {
                return cells[n].elevation >= ocean_level;
            });

            for (int idx : component) {
                cells[idx].landmass_id = next_region_id;
            }

            Region reg;
            reg.id = next_region_id;
            reg.cell_indices = component;
            
            Vector2 center(0, 0);
            for (int idx : component) {
                center += get_cell_position(idx);
            }
            if (!component.empty()) center /= (float)component.size();
            reg.center_position = center;

            if (component.size() >= 100) {
                reg.type = "continent";
                reg.name = name_gen.generate_landmass_name(step_rng);
            } else {
                reg.type = "island";
                reg.name = name_gen.generate_island_name(step_rng);
            }

            regions.push_back(reg);
            next_region_id++;
        }
    }

    // 2. Segment Mountain Ranges (elevation >= 0.70)
    std::vector<bool> visited_mountain(cells.size(), false);
    for (int i = 0; i < (int)cells.size(); ++i) {
        if (cells[i].elevation >= 0.70f && !visited_mountain[i]) {
            std::vector<int> component = flood_fill(i, visited_mountain, [&](int c, int n) {
                return cells[n].elevation >= 0.70f;
            });

            for (int idx : component) {
                cells[idx].mountain_range_id = next_region_id;
            }

            Region reg;
            reg.id = next_region_id;
            reg.type = "mountain_range";
            reg.cell_indices = component;
            
            Vector2 center(0, 0);
            for (int idx : component) {
                center += get_cell_position(idx);
            }
            if (!component.empty()) center /= (float)component.size();
            reg.center_position = center;

            reg.name = name_gen.generate_mountain_name(step_rng);
            regions.push_back(reg);
            next_region_id++;
        }
    }

    // 3. Segment Biome Regions & Micro-regions
    std::vector<bool> visited_biome(cells.size(), false);
    for (int i = 0; i < (int)cells.size(); ++i) {
        if (!visited_biome[i]) {
            int target_biome = cells[i].biome;
            std::vector<int> component = flood_fill(i, visited_biome, [&](int c, int n) {
                return cells[n].biome == target_biome;
            });

            Region reg;
            reg.id = next_region_id;
            reg.cell_indices = component;
            
            Vector2 center(0, 0);
            for (int idx : component) {
                center += get_cell_position(idx);
            }
            if (!component.empty()) center /= (float)component.size();
            reg.center_position = center;

            bool on_land = false;
            for (int idx : component) {
                if (cells[idx].elevation >= ocean_level) {
                    on_land = true;
                    break;
                }
            }

            if (component.size() < 5 && on_land) {
                reg.type = "micro_region";
                reg.name = name_gen.generate_micro_region_name(step_rng);
                for (int idx : component) {
                    cells[idx].micro_region_id = next_region_id;
                }
            } else {
                reg.type = "biome_region";
                reg.name = name_gen.generate_biome_region_name(target_biome, step_rng);
                for (int idx : component) {
                    cells[idx].biome_region_id = next_region_id;
                }
            }

            regions.push_back(reg);
            next_region_id++;
        }
    }

    // 4. Segment Rivers
    Array river_paths = get_river_paths();
    for (int r = 0; r < river_paths.size(); ++r) {
        Array path = river_paths[r];
        if (path.size() < 2) continue;

        std::vector<int> component;
        for (int p = 0; p < path.size(); ++p) {
            Vector2 pos = path[p];
            int y = (int)round(pos.y / 0.8660254f);
            bool is_odd = (y & 1) != 0;
            int x = (int)round(pos.x - (is_odd ? 0.5f : 0.0f));
            if (x >= 0 && x < width && y >= 0 && y < height) {
                int idx = y * width + x;
                component.push_back(idx);
                cells[idx].river_id = next_region_id;
            }
        }

        if (component.empty()) continue;

        Region reg;
        reg.id = next_region_id;
        reg.type = "river";
        reg.cell_indices = component;
        
        Vector2 center(0, 0);
        for (int idx : component) {
            center += get_cell_position(idx);
        }
        if (!component.empty()) center /= (float)component.size();
        reg.center_position = center;

        reg.name = name_gen.generate_river_name(step_rng);
        regions.push_back(reg);
        next_region_id++;
    }
}

Array HexcrawlMapGenerator::get_regions() const {
    Array arr;
    for (const auto &reg : regions) {
        Dictionary d;
        d["id"] = reg.id;
        d["name"] = reg.name;
        d["type"] = reg.type;
        d["center"] = reg.center_position;
        
        Array indices;
        for (int idx : reg.cell_indices) {
            indices.append(idx);
        }
        d["cell_indices"] = indices;
        arr.append(d);
    }
    return arr;
}
