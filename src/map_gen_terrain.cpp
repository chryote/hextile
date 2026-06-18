#include "hexcrawl_map_generator.h"
#include <cmath>
#include <algorithm>
#include <numeric>
#include <vector>
#include <random>

#include "perlin_noise.h"


void HexcrawlMapGenerator::step_add_base_noise() {
    PerlinNoise noise_gen(seed + 2);
    
    Vector2 center_pos = get_cell_position(height / 2 * width + width / 2);
    float max_dist = center_pos.length() * 0.85f;

    for (int i = 0; i < (int)cells.size(); ++i) {
        Vector2 pos = get_cell_position(i);
        
        double nx = (double)pos.x * noise_scale;
        double ny = (double)pos.y * noise_scale;
        
        float noise_val = (float)noise_gen.octave_noise(nx, ny, noise_octaves, noise_persistence, noise_lacunarity);
        cells[i].noise_val = noise_val * noise_amplitude;

        float raw_elev = cells[i].uplift + cells[i].noise_val;

        float dist = pos.distance_to(center_pos);
        float ratio = dist / max_dist;
        if (ratio > 1.0f) ratio = 1.0f;
        float falloff = 1.0f - ratio * ratio * ratio;

        cells[i].elevation = (raw_elev + 0.12f) * falloff - 0.12f;
    }
}

void HexcrawlMapGenerator::step_run_erosion() {
    if (cells.empty()) return;

    std::mt19937 erosion_rng(seed + 3);

    std::vector<int> land_indices;
    for (int i = 0; i < (int)cells.size(); ++i) {
        if (cells[i].elevation > ocean_level) {
            land_indices.push_back(i);
        }
    }

    if (land_indices.empty()) return;

    for (int iter = 0; iter < erosion_iterations; ++iter) {
        int rand_idx = erosion_rng() % land_indices.size();
        int c = land_indices[rand_idx];

        float water = 1.0f;
        float speed = 1.0f;
        float sediment = 0.0f;

        for (int step = 0; step < 30; ++step) {
            std::vector<int> neighbors = get_neighbors_internal(c);
            if (neighbors.empty()) break;

            int n_min = -1;
            float min_elev = cells[c].elevation;

            for (int n_idx : neighbors) {
                if (cells[n_idx].elevation < min_elev) {
                    min_elev = cells[n_idx].elevation;
                    n_min = n_idx;
                }
            }

            if (n_min == -1) {
                cells[c].elevation += sediment;
                break;
            }

            if (cells[n_min].elevation < ocean_level) {
                cells[n_min].elevation += sediment;
                break;
            }

            float dh = cells[c].elevation - cells[n_min].elevation;
            float capacity = water * speed * dh * erosion_capacity_factor;

            if (sediment < capacity) {
                float amount = (capacity - sediment) * erosion_rate;
                amount = std::min(amount, dh);
                cells[c].elevation -= amount;
                sediment += amount;
            } else {
                float amount = (sediment - capacity) * deposition_rate;
                cells[c].elevation += amount;
                sediment -= amount;
            }

            speed = std::sqrt(speed * speed + dh * gravity_factor);
            water *= (1.0f - water_evaporation_rate);
            c = n_min;
        }
    }
}
