# Project Style & Modularization Guide

This document outlines structural patterns and style conventions to prevent codebase clutter and maintain scalability as new features, frameworks, or simulation pipelines are added to the **World Sim** project.

---

## 1. Project Directory Structure

Maintain a clean separation between raw simulation logic (C++) and Godot's runtime editor/UI layer (GDScript).

```
d:\world-sim\
├── src\                      # C++ Source Code (GDExtension)
│   ├── hexcrawl_map_generator.h/.cpp # Coordination class
│   ├── hextile_framework.h   # Core shared structs
│   ├── register_types.h/.cpp # Extension entrypoint
│   ├── perlin_noise.h/.cpp   # Noise math module
│   ├── name_generator.h/.cpp # Name generation module
│   └── map_gen_*.cpp         # Pipeline steps (Tectonics, Terrain, etc.)
│
├── demo\                     # Godot Project
│   ├── bin\                  # GDExtension DLLs & configurations
│   ├── camera_controller.gd  # Camera pan & zoom logic
│   ├── main.gd               # Main controller (UI mapping)
│   ├── main.tscn             # Main visual layout scene
│   ├── map_renderer.gd       # Canvas drawing loop
│   └── project.godot         # Godot project settings
│
└── GUARDRAIL.md, PROJECT_STYLE.md # Project documentation
```

---

## 2. C++ Architectural Guidelines (Preventing Bloat)

### De-Cluttering the Monolithic Generator
As the procedural map pipeline expands (e.g., adding flora, paths, kingdoms, roads, or dungeon spawners):
1. **Do NOT** add all execution functions and state variables directly as members of `HexcrawlMapGenerator`.
2. **Encapsulate New Pipelines**: Create dedicated helper classes or processing engines in separate files (e.g., `KingdomSimulator`, `RoadBuilder`).
3. **Pass State by Reference**: Pass the cell vector or specific sub-data structure to these helper classes for manipulation:
   ```cpp
   // Avoid: adding member functions to HexcrawlMapGenerator
   void HexcrawlMapGenerator::simulate_kingdoms(); // BLOAT

   // Prefer: delegating to separate modules
   #include "kingdom_simulator.h"
   void HexcrawlMapGenerator::step_simulate_kingdoms() {
       KingdomSimulator simulator(config_params);
       simulator.process_kingdoms(cells, width, height); // CLEAN
   }
   ```

### Grouping Configuration Parameters
Avoid adding dozens of independent scalar member variables and properties directly to the main generator class. Group them into distinct configuration structs:
```cpp
struct ClimateConfig {
    float lapse_rate;
    float wind_direction;
    float base_moisture;
    float moisture_retention_rate;
};
```
This keeps class definitions small, readable, and easy to pass around.

---

## 3. Coding Style Conventions

### C++ Style Guidelines
- **Naming Conventions**:
  - Class Names: `PascalCase` (e.g., `NameGenerator`, `HexcrawlMapGenerator`).
  - Source/Header Files: `snake_case` (e.g., `perlin_noise.cpp`, `name_generator.h`).
  - Variables, Properties, and Methods: `snake_case` (e.g., `get_neighbors_internal()`, `plate_speed_min`).
- **Header Cleanliness**:
  - Avoid putting `#include` statements in header files if a forward declaration (`class ClassName;` or `struct StructName;`) is sufficient.
  - Group standard library headers (`<vector>`, `<random>`) separately from Godot headers and local headers.
- **Bindings Organization**:
  - Keep `ClassDB::bind_method` calls in `_bind_methods()` ordered exactly matching their declaration order in the header files.

### GDScript Style Guidelines
Always adhere to Godot's Official GDScript Style Guide:
- **Naming Conventions**:
  - Script File names and Member Variables: `snake_case` (e.g., `map_renderer.gd`, `selected_cell_idx`).
  - Classes: `PascalCase` (e.g., `class_name HexMath`).
  - Constants: `ALL_CAPS_SNAKE` (e.g., `MAX_ZOOM`).
- **Node References**:
  - Declare node references at the top of the file using `@onready var` with relative pathing from the attached node:
    ```gdscript
    @onready var map_renderer = $MapContainer/MapRenderer
    ```
- **Static Typings**:
  - Explicitly declare argument types and return values for all script helper methods to ensure readability and editor autocompletion:
    ```gdscript
    func _get_cell_at_position(pos: Vector2) -> int:
    ```

---

## 4. How to Add a New Pipeline Step
To add a new map generation step (e.g., "Add Kingdoms and Factions"):
1. **Define the Logic**: Create a new C++ source file `src/map_gen_kingdoms.cpp`.
2. **Expose Pipeline Step**: Declare a public step method in `hexcrawl_map_generator.h` (e.g., `void step_generate_kingdoms()`).
3. **Register Step Method**: Bind it in `_bind_methods()` so it can be called individually if debugging step-by-step.
4. **Trigger in Pipeline**: Call the step inside `run_full_pipeline()` in `hexcrawl_map_generator.cpp`.
5. **Update UI**: Add the visualization options and inspectors in `main.tscn` and `main.gd` respectively.
