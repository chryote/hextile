#include "hexcrawl_map_generator.h"
#include <vector>
#include <queue>
#include <algorithm>

void HexcrawlMapGenerator::step_assign_biomes() {
    std::vector<bool> connected_to_ocean(cells.size(), false);
    std::queue<int> q;

    for (int i = 0; i < (int)cells.size(); ++i) {
        if (cells[i].elevation < ocean_level) {
            int cx = cells[i].x;
            int cy = cells[i].y;
            if (cx == 0 || cx == width - 1 || cy == 0 || cy == height - 1) {
                connected_to_ocean[i] = true;
                q.push(i);
            }
        }
    }

    while (!q.empty()) {
        int curr = q.front();
        q.pop();

        std::vector<int> neighbors = get_neighbors_internal(curr);
        for (int n_idx : neighbors) {
            if (cells[n_idx].elevation < ocean_level && !connected_to_ocean[n_idx]) {
                connected_to_ocean[n_idx] = true;
                q.push(n_idx);
            }
        }
    }

    for (int i = 0; i < (int)cells.size(); ++i) {
        if (cells[i].elevation < ocean_level) {
            if (connected_to_ocean[i]) {
                cells[i].biome = 0;
            } else {
                cells[i].biome = 14;
            }
            continue;
        }

        if (cells[i].elevation < ocean_level + beach_elevation_threshold && cells[i].temperature > 0.35f) {
            cells[i].biome = 1;
            continue;
        }

        float t = cells[i].temperature;
        float m = cells[i].moisture;

        if (t < 0.15f) {
            if (m > 0.4f) {
                cells[i].biome = 3;
            } else {
                cells[i].biome = 2;
            }
        } else if (t < 0.35f) {
            if (m < 0.25f) {
                cells[i].biome = 5;
            } else {
                cells[i].biome = 4;
            }
        } else if (t < 0.65f) {
            if (m < 0.25f) {
                cells[i].biome = 6;
            } else if (m < 0.65f) {
                cells[i].biome = 7;
            } else {
                cells[i].biome = 8;
            }
        } else {
            if (m < 0.18f) {
                cells[i].biome = 9;
            } else if (m < 0.45f) {
                cells[i].biome = 10;
            } else if (m < 0.72f) {
                cells[i].biome = 11;
            } else {
                cells[i].biome = 12;
            }
        }
    }

    for (int i = 0; i < (int)cells.size(); ++i) {
        if (cells[i].biome == 1) {
            std::vector<int> neighbors = get_neighbors_internal(i);
            for (int n_idx : neighbors) {
                if (cells[n_idx].biome == 14) {
                    cells[i].biome = 15;
                    break;
                }
            }
        }
    }

    step_generate_region_names();
}
