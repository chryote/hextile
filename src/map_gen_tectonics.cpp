#include "hexcrawl_map_generator.h"
#include <queue>
#include <algorithm>
#include <cmath>
#include <numeric>

void HexcrawlMapGenerator::step_generate_plates() {
    std::mt19937 step_rng(seed + 1);
    plates.clear();

    std::vector<int> indices(width * height);
    std::iota(indices.begin(), indices.end(), 0);
    std::shuffle(indices.begin(), indices.end(), step_rng);

    int actual_num_plates = std::min(num_plates, (int)indices.size());
    std::queue<std::pair<int, int>> q;

    for (int i = 0; i < actual_num_plates; ++i) {
        int seed_idx = indices[i];
        Plate p;
        p.id = i;
        float angle = (float)step_rng() / step_rng.max() * 2.0f * 3.14159f;
        float speed = plate_speed_min + (float)step_rng() / step_rng.max() * (plate_speed_max - plate_speed_min);
        p.velocity = Vector2(cos(angle), sin(angle)) * speed;
        plates.push_back(p);

        cells[seed_idx].plate_id = i;
        q.push({seed_idx, i});
    }

    while (!q.empty()) {
        auto current = q.front();
        q.pop();

        int curr_idx = current.first;
        int curr_plate = current.second;

        std::vector<int> neighbors = get_neighbors_internal(curr_idx);
        for (int n_idx : neighbors) {
            if (cells[n_idx].plate_id == -1) {
                cells[n_idx].plate_id = curr_plate;
                q.push({n_idx, curr_plate});
            }
        }
    }
}

void HexcrawlMapGenerator::step_compute_uplift() {
    for (int i = 0; i < (int)cells.size(); ++i) {
        cells[i].uplift = 0.0f;
    }

    std::vector<std::pair<int, float>> boundary_uplifts;

    for (int i = 0; i < (int)cells.size(); ++i) {
        int my_plate = cells[i].plate_id;
        if (my_plate == -1 || my_plate < 0 || my_plate >= (int)plates.size()) continue;

        Vector2 my_pos = get_cell_position(i);
        Vector2 my_vel = plates[my_plate].velocity;

        float max_uplift = 0.0f;
        float max_depression = 0.0f;

        std::vector<int> neighbors = get_neighbors_internal(i);
        bool is_boundary = false;

        for (int n_idx : neighbors) {
            int n_plate = cells[n_idx].plate_id;
            if (n_plate >= 0 && n_plate < (int)plates.size() && n_plate != my_plate) {
                is_boundary = true;
                Vector2 n_pos = get_cell_position(n_idx);
                Vector2 n_vel = plates[n_plate].velocity;

                Vector2 normal = (n_pos - my_pos).normalized();
                Vector2 rel_vel = my_vel - n_vel;

                float dot = rel_vel.dot(normal);

                if (dot > 0.15f) {
                    float force = dot * uplift_collision_factor;
                    if (force > max_uplift) max_uplift = force;
                } else if (dot < -0.15f) {
                    float force = dot * -uplift_separation_factor;
                    if (force < max_depression) max_depression = force;
                } else {
                    float cross = std::abs(rel_vel.x * normal.y - rel_vel.y * normal.x);
                    float force = cross * uplift_shear_factor;
                    if (force > max_uplift) max_uplift = force;
                }
            }
        }

        if (is_boundary) {
            float final_uplift = max_uplift > 0.0f ? max_uplift : max_depression;
            if (std::abs(final_uplift) > 0.01f) {
                boundary_uplifts.push_back({i, final_uplift});
                cells[i].uplift = final_uplift;
            }
        }
    }

    std::queue<std::pair<int, float>> q;
    std::vector<bool> visited(cells.size(), false);

    for (auto b : boundary_uplifts) {
        q.push(b);
        visited[b.first] = true;
    }

    float decay = uplift_decay;

    while (!q.empty()) {
        auto curr = q.front();
        q.pop();

        int curr_idx = curr.first;
        float curr_uplift = curr.second;

        if (std::abs(curr_uplift) < 0.05f) continue;

        std::vector<int> neighbors = get_neighbors_internal(curr_idx);
        for (int n_idx : neighbors) {
            float next_uplift = curr_uplift * decay;
            if (std::abs(next_uplift) > std::abs(cells[n_idx].uplift)) {
                cells[n_idx].uplift = next_uplift;
                q.push({n_idx, next_uplift});
            }
        }
    }
}
