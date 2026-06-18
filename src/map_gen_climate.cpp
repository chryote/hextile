#include "hexcrawl_map_generator.h"
#include <cmath>
#include <algorithm>
#include <vector>

void HexcrawlMapGenerator::step_simulate_climate() {
    // 1. Simulate Temperature
    for (int i = 0; i < (int)cells.size(); ++i) {
        float lat = (float)cells[i].y / (float)height;
        float t_base = lat;

        float h_diff = std::max(0.0f, cells[i].elevation - ocean_level);
        float temp = t_base - lapse_rate * h_diff;
        cells[i].temperature = std::max(0.0f, std::min(1.0f, temp));
    }

    // 2. Simulate Wind/Moisture
    for (int i = 0; i < (int)cells.size(); ++i) {
        if (cells[i].elevation < ocean_level) {
            cells[i].moisture = 1.0f;
        } else {
            cells[i].moisture = 0.0f;
        }
    }

    for (int col = width - 1; col >= 0; --col) {
        for (int row = 0; row < height; ++row) {
            int idx = row * width + col;
            if (cells[idx].elevation < ocean_level) {
                cells[idx].moisture = 1.0f;
                continue;
            }

            std::vector<int> neighbors = get_neighbors_internal(idx);
            float incoming_moisture = 0.0f;
            int count = 0;

            for (int n_idx : neighbors) {
                int nx = n_idx % width;
                if (nx > col || (nx == col && n_idx > idx)) {
                    incoming_moisture += cells[n_idx].moisture;
                    count++;
                }
            }

            float m_in = base_moisture;
            if (count > 0) {
                m_in = incoming_moisture / count;
            }

            float precipitation = 0.0f;
            float east_elevation = ocean_level;
            int e_count = 0;
            for (int n_idx : neighbors) {
                int nx = n_idx % width;
                if (nx > col) {
                    east_elevation += cells[n_idx].elevation;
                    e_count++;
                }
            }
            if (e_count > 0) {
                east_elevation /= e_count;
            }

            if (cells[idx].elevation > east_elevation) {
                float slope = cells[idx].elevation - east_elevation;
                precipitation = m_in * slope * precipitation_factor;
                precipitation = std::max(0.0f, std::min(m_in, precipitation));
            }

            float cell_moisture = m_in - precipitation;
            cells[idx].moisture = std::max(0.05f, cell_moisture * moisture_retention_rate);
        }
    }

    std::vector<float> temp_moisture(cells.size());
    for (int i = 0; i < (int)cells.size(); ++i) {
        if (cells[i].elevation < ocean_level) {
            temp_moisture[i] = 1.0f;
            continue;
        }

        std::vector<int> neighbors = get_neighbors_internal(i);
        float sum = cells[i].moisture;
        int count = 1;
        for (int n_idx : neighbors) {
            if (cells[n_idx].elevation >= ocean_level) {
                sum += cells[n_idx].moisture;
                count++;
            }
        }
        temp_moisture[i] = sum / count;
    }

    for (int i = 0; i < (int)cells.size(); ++i) {
        cells[i].moisture = temp_moisture[i];
    }
}
