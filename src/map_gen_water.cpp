#include "hexcrawl_map_generator.h"
#include <vector>
#include <numeric>
#include <algorithm>

void HexcrawlMapGenerator::step_generate_rivers() {
    for (int i = 0; i < (int)cells.size(); ++i) {
        cells[i].water_accumulation = 1.0f;
        cells[i].river_next_idx = -1;
        cells[i].is_river = false;
    }

    std::vector<int> sorted_indices(cells.size());
    std::iota(sorted_indices.begin(), sorted_indices.end(), 0);
    std::sort(sorted_indices.begin(), sorted_indices.end(), [this](int a, int b) {
        return cells[a].elevation > cells[b].elevation;
    });

    for (int idx : sorted_indices) {
        std::vector<int> neighbors = get_neighbors_internal(idx);
        int n_min = -1;
        float min_elev = cells[idx].elevation;

        for (int n_idx : neighbors) {
            if (cells[n_idx].elevation < min_elev) {
                min_elev = cells[n_idx].elevation;
                n_min = n_idx;
            }
        }

        if (n_min != -1 && cells[idx].elevation >= ocean_level) {
            cells[idx].river_next_idx = n_min;
            cells[n_min].water_accumulation += cells[idx].water_accumulation;
        }
    }

    for (int i = 0; i < (int)cells.size(); ++i) {
        if (cells[i].elevation >= ocean_level && cells[i].water_accumulation >= river_threshold) {
            cells[i].is_river = true;
        }
    }

    compute_overlays();
}
