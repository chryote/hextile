# Codebase Guardrails & Development Guidelines

This document serves as a guide and constraint set for both human developers and agentic AI coders working on the **World Sim Procedural Hexcrawl** project. It outlines rules for maintaining stability, cleanliness, and correctness across the C++ (GDExtension) and GDScript layers.

---

## 1. C++ GDExtension Rules

### Static / Global Initialization Constraint (CRITICAL)
- **Rule**: Do **NOT** declare global or static variables using Godot API types (e.g., `godot::String`, `godot::Array`, `godot::Dictionary`, `godot::Vector2`, `godot::Color`).
- **Why**: Windows loads the DLL and initializes global objects before Godot-CPP binds the Godot engine API function pointers. Constructing Godot types statically invokes uninitialized function pointers, causing a crash during DLL loading (**Windows Error 1114: A dynamic link library initialization routine failed**).
- **Correct Pattern**: Store database constants as standard C++ types (e.g., `const char*`, `int`, `double`, `std::string`) and construct the Godot type dynamically inside functions at runtime:
  ```cpp
  // Correct:
  static const std::vector<const char*> prefixes = {"Eld", "Val"};
  String get_name(int idx) {
      return String(prefixes[idx]); // Constructed safely at runtime
  }
  ```

### Avoid Hardcoded Magic Numbers
- **Rule**: Algorithmic constants (e.g., tectonic velocities, uplift decay coefficients, erosion capacity coefficients, climate moisture retention, temperature lapse rates) must not be hardcoded inside generation step files.
- **Correct Pattern**: Declare these parameters as private members of `HexcrawlMapGenerator`, expose them as properties using `ClassDB::bind_method` and `ADD_PROPERTY` in `_bind_methods()`, and access them through getters/setters. This allows real-time tuning from the Godot editor inspector or GDScript without recompiling.

### Single Source of Truth for Grid Math
- **Rule**: Grid-level math calculations (such as neighbor indices, hexagonal coordinate calculations, distance calculations, and shallow water checks) must be implemented in C++ and exposed publicly to Godot ClassDB.
- **Why**: Avoids duplicating hex math scripts in GDScript (e.g., removing static libraries like `HexMath.gd`), keeping the logic synchronized and performant.

### Memory & Index Safety
- **Rule**: Always perform boundary checks before indexing into arrays or vectors (such as checking `plate_id >= 0 && plate_id < plates.size()` or cell bounds). This prevents segmentation faults or access violations in the GDExtension module.

---

## 2. GDScript & UI Rules

### Scene-Based UI Layout
- **Rule**: Construct UI layout trees visually in Godot scene files (`.tscn`) using standard containers (`PanelContainer`, `MarginContainer`, `VBoxContainer`, etc.). Do **NOT** programmatically instantiate extensive UI trees in script files.
- **Correct Pattern**: Bind script control logic to visual nodes using `@onready` variables and signals connected either in the editor or programmatically during `_ready()`.

### Call GDExtension for Simulation State & Math
- **Rule**: The GDScript frontend must act purely as a controller and renderer. Do not duplicate generation logic or grid calculations in script files. Call the exposed C++ class methods (`get_cell_position()`, `get_neighbors()`, `is_shallow_water()`) directly.

---

## 3. Build & Compilation Workflow

### Releasing DLL File Locks
- **Rule**: Always close the Godot editor or running player instance before compiling the C++ library.
- **Why**: Godot holds a write-lock on the DLL while running, causing SCons to fail with `Access is denied` (Linker Error).
- **Troubleshooting**: If SCons reports access errors, verify no Godot editor is running, clean the stale build files using:
  ```powershell
  Remove-Item demo\bin\*.dll, demo\bin\*.exp, demo\bin\*.lib, demo\bin\*.pdb, demo\bin\*.ilk -ErrorAction SilentlyContinue
  ```
  And then rebuild.

### Standard SCons Compilation Command
- **Debug Target**:
  ```powershell
  python -m SCons -j4 platform=windows target=template_debug
  ```
- **Release Target**:
  ```powershell
  python -m SCons -j4 platform=windows target=template_release
  ```

---

## 4. Verification Checklists
Before submitting changes:
1. Ensure SCons compiles and links the DLL cleanly.
2. Run Godot in headless validation mode to check for compile or loader errors:
   ```powershell
   .\Godot_v4.4.1-stable_win64.exe --path demo --headless --quit --verbose
   ```
3. Ensure no GDExtension dynamic library load errors are output to the standard error stream.
