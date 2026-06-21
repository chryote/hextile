extends Control

# References to children
@onready var map_container = $MapContainer
@onready var map_renderer = $MapContainer/MapRenderer
@onready var camera = $MapContainer/Camera2D
@onready var ui_layer = $UI

# UI Panels from main.tscn
@onready var control_panel = $UI/ControlPanel
@onready var collapse_button = $UI/ControlPanel/Margin/VBox/Header/CollapseButton
@onready var content_vbox = $UI/ControlPanel/Margin/VBox/Content
@onready var status_label = $UI/ControlPanel/Margin/VBox/Content/StatusLabel
@onready var scroll_vbox = $UI/ControlPanel/Margin/VBox/Content/Scroll/ScrollVBox

@onready var generate_button = $UI/ControlPanel/Margin/VBox/Content/Actions/GenerateButton
@onready var reset_button = $UI/ControlPanel/Margin/VBox/Content/Actions/ResetButton
@onready var debug_button = $UI/ControlPanel/Margin/VBox/Content/DebugButton

@onready var layer_selector = $UI/ControlPanel/Margin/VBox/Content/LayerSelector
@onready var rivers_check = $UI/ControlPanel/Margin/VBox/Content/RiversCheck
@onready var borders_check = $UI/ControlPanel/Margin/VBox/Content/BordersCheck
@onready var vectors_check = $UI/ControlPanel/Margin/VBox/Content/VectorsCheck

@onready var legend_panel = $UI/LegendPanel
@onready var legend_list = $UI/LegendPanel/Margin/VBox/Scroll/LegendList

@onready var inspector_panel = $UI/InspectorPanel
@onready var inspector_title = $UI/InspectorPanel/Margin/VBox/Title
@onready var inspector_list = $UI/InspectorPanel/Margin/VBox/Scroll/InspectorList

# Map Generator instance
var map_gen: RefCounted

# Generation parameters
var map_width = 60
var map_height = 40
var map_seed = 12345
var num_plates = 12
var ocean_level = 0.35
var noise_scale = 0.07
var noise_octaves = 4
var erosion_iterations = 60000
var erosion_rate = 0.05
var wind_angle_deg = 0.0 # 0 means East-to-West

# Simulation state
var current_step = 0 # 0 to 7
var active_layer = "biomes" # biomes, elevation, plates, temp, moisture, water
var show_rivers = true
var show_borders = true
var show_plate_vel = true

# Interaction state
var hovered_cell_idx = -1
var selected_cell_idx = -1
var hex_size = 22.0
var spacing_scale = hex_size * sqrt(3)
var is_debug_map = false

var interaction_input_container: VBoxContainer
var interaction_line_edit: LineEdit
var interaction_send_btn: Button
var last_interaction_result: String = ""

# Biome names and colors
var biome_names = {
	0: "Deep Ocean",
	13: "Shallow Coast",
	14: "Lake",
	1: "Sandy Lowland",
	15: "Sandy Basin",
	2: "Tundra",
	3: "Glacier/Snow",
	4: "Boreal Forest (Taiga)",
	5: "Cold Desert",
	6: "Temperate Grassland",
	7: "Temperate Forest",
	8: "Temperate Rainforest",
	9: "Subtropical Desert",
	10: "Savanna",
	11: "Tropical Dry Forest",
	12: "Tropical Rainforest"
}

var biome_colors = {
	0: Color("#102542"), # Deep Ocean
	13: Color("#185e7a"), # Shallow Coast
	14: Color("#206894"), # Lake
	1: Color("#dec595"), # Beach
	15: Color("#af9566"), # Basin
	2: Color("#8fa093"), # Tundra
	3: Color("#f0f5fa"), # Snow
	4: Color("#28522c"), # Taiga
	5: Color("#ab9a82"), # Cold Desert
	6: Color("#7fa155"), # Grassland
	7: Color("#3b6b3b"), # Temperate Forest
	8: Color("#1a5235"), # Temperate Rainforest
	9: Color("#cfae70"), # Subtropical Desert
	10: Color("#b5a76c"), # Savanna
	11: Color("#2f7d46"), # Tropical Dry Forest
	12: Color("#05521b")  # Tropical Rainforest
}

func _ready():
	# Instantiate GDExtension Class
	if ClassDB.class_exists("HexcrawlMapGenerator"):
		map_gen = ClassDB.instantiate("HexcrawlMapGenerator")
	else:
		push_error("HexcrawlMapGenerator GDExtension class not found! Make sure GDExtension library is built successfully.")
		return

	# Setup Camera
	camera.position = Vector2(map_width * spacing_scale / 2.0, map_height * hex_size * 1.5 / 2.0)
	camera.zoom = Vector2(0.7, 0.7)

	# Stop Main control from intercepting mouse clicks
	mouse_filter = Control.MOUSE_FILTER_IGNORE

	# Initialize UI structure and wire up inputs
	_setup_ui()

	# Run initial step 0 map
	_generate_full_map()

func _process(_delta):
	# Handle mouse hover
	var prev_hover = hovered_cell_idx
	
	# Only hover if mouse is not over a UI panel
	var hovered_control = get_viewport().gui_get_hovered_control()
	if hovered_control != null and hovered_control.mouse_filter != Control.MOUSE_FILTER_IGNORE:
		hovered_cell_idx = -1
	else:
		var mouse_pos = map_container.get_local_mouse_position()
		hovered_cell_idx = _get_cell_at_position(mouse_pos)
		
	if hovered_cell_idx != prev_hover:
		map_renderer.queue_redraw()

func _unhandled_input(event):
	if event is InputEventMouseButton and event.button_index == MOUSE_BUTTON_LEFT and event.pressed:
		var mouse_pos = map_container.get_local_mouse_position()
		var clicked = _get_cell_at_position(mouse_pos)
		if clicked != selected_cell_idx:
			map_gen.exit_holding()
			last_interaction_result = ""
		selected_cell_idx = clicked
		_update_inspector()
		map_renderer.queue_redraw()

# Simulation operations
func _reset_map():
	is_debug_map = false
	current_step = 0
	selected_cell_idx = -1
	hovered_cell_idx = -1
	
	if map_gen != null:
		map_gen.exit_holding()
	
	# Apply parameters to generator
	map_gen.set_width(map_width)
	map_gen.set_height(map_height)
	map_gen.set_seed(map_seed)
	map_gen.set_num_plates(num_plates)
	map_gen.set_ocean_level(ocean_level)
	map_gen.set_noise_scale(noise_scale)
	map_gen.set_noise_octaves(noise_octaves)
	map_gen.set_erosion_iterations(erosion_iterations)
	map_gen.set_erosion_rate(erosion_rate)
	map_gen.set_wind_direction(wind_angle_deg * PI / 180.0)

	map_gen.initialize(map_width, map_height, map_seed)
	_update_status()
	_update_inspector()
	_update_legend()
	map_renderer.queue_redraw()

func _generate_full_map():
	_reset_map()
	map_gen.run_full_pipeline()
	current_step = 8
	active_layer = "biomes"
	
	if layer_selector:
		layer_selector.select(0) # Select biomes

	_update_status()
	_update_inspector()
	_update_legend()
	map_renderer.queue_redraw()

func _generate_debug_map():
	_reset_map()
	is_debug_map = true
	
	map_gen.set_width(map_width)
	map_gen.set_height(map_height)
	map_gen.set_ocean_level(ocean_level)
	map_gen.generate_debug_map()
	
	current_step = 8
	active_layer = "biomes"
	
	if layer_selector:
		layer_selector.select(0) # Select biomes

	_update_status()
	_update_inspector()
	_update_legend()
	map_renderer.queue_redraw()

# UI Setup and Signal Binding
func _setup_ui():
	# Glassmorphic style setups
	var panel_style = StyleBoxFlat.new()
	panel_style.bg_color = Color(0.08, 0.09, 0.12, 0.85)
	panel_style.border_width_left = 1
	panel_style.border_width_top = 1
	panel_style.border_width_right = 1
	panel_style.border_width_bottom = 1
	panel_style.border_color = Color(0.25, 0.3, 0.4, 0.35)
	panel_style.corner_radius_top_left = 12
	panel_style.corner_radius_top_right = 12
	panel_style.corner_radius_bottom_left = 12
	panel_style.corner_radius_bottom_right = 12
	panel_style.shadow_color = Color(0, 0, 0, 0.4)
	panel_style.shadow_size = 12

	control_panel.add_theme_stylebox_override("panel", panel_style)
	legend_panel.add_theme_stylebox_override("panel", panel_style)
	inspector_panel.add_theme_stylebox_override("panel", panel_style)

	# Collapse panel logic
	collapse_button.pressed.connect(func():
		content_vbox.visible = !content_vbox.visible
		collapse_button.text = "—" if content_vbox.visible else "+"
		control_panel.size.y = 0
	)

	# Action Button logic
	generate_button.pressed.connect(_generate_full_map)
	reset_button.pressed.connect(_reset_map)
	debug_button.pressed.connect(_generate_debug_map)

	# View option checkbox logic
	rivers_check.button_pressed = show_rivers
	rivers_check.toggled.connect(func(press):
		show_rivers = press
		map_renderer.queue_redraw()
	)

	borders_check.button_pressed = show_borders
	borders_check.toggled.connect(func(press):
		show_borders = press
		map_renderer.queue_redraw()
	)

	vectors_check.button_pressed = show_plate_vel
	vectors_check.toggled.connect(func(press):
		show_plate_vel = press
		map_renderer.queue_redraw()
	)

	# Layer selector items
	layer_selector.clear()
	layer_selector.add_item("Biomes (Final)", 0)
	layer_selector.set_item_metadata(0, "biomes")
	layer_selector.add_item("Elevation Map", 1)
	layer_selector.set_item_metadata(1, "elevation")
	layer_selector.add_item("Tectonic Plates", 2)
	layer_selector.set_item_metadata(2, "plates")
	layer_selector.add_item("Temperature", 3)
	layer_selector.set_item_metadata(3, "temp")
	layer_selector.add_item("Moisture (Rain Shadows)", 4)
	layer_selector.set_item_metadata(4, "moisture")
	layer_selector.add_item("Water Accumulation", 5)
	layer_selector.set_item_metadata(5, "water")
	layer_selector.item_selected.connect(func(idx):
		active_layer = layer_selector.get_item_metadata(idx)
		_update_legend()
		map_renderer.queue_redraw()
	)
	layer_selector.select(0)

	# Populate Sliders
	_add_slider(scroll_vbox, "Grid Width", 15, 120, map_width, func(v): map_width = int(v))
	_add_slider(scroll_vbox, "Grid Height", 15, 90, map_height, func(v): map_height = int(v))
	_add_seed_input(scroll_vbox)
	_add_slider(scroll_vbox, "Tectonic Plates", 2, 24, num_plates, func(v): num_plates = int(v))
	_add_slider(scroll_vbox, "Ocean Level", 0.15, 0.60, ocean_level, func(v): ocean_level = v, 2)
	_add_slider(scroll_vbox, "Noise Scale", 0.02, 0.15, noise_scale, func(v): noise_scale = v, 3)
	_add_slider(scroll_vbox, "Erosion Drops", 0, 120000, erosion_iterations, func(v): erosion_iterations = int(v), 0, 5000)
	_add_slider(scroll_vbox, "Erosion Rate", 0.01, 0.15, erosion_rate, func(v): erosion_rate = v, 2)
	_add_slider(scroll_vbox, "Wind Angle", 0, 360, wind_angle_deg, func(v): wind_angle_deg = v, 0)

	# Setup MUD Interaction Command Line programmatically
	var mud_vbox = inspector_panel.get_node("Margin/VBox")
	
	interaction_input_container = VBoxContainer.new()
	interaction_input_container.add_theme_constant_override("separation", 4)
	interaction_input_container.visible = false
	mud_vbox.add_child(interaction_input_container)
	
	var input_hbox = HBoxContainer.new()
	input_hbox.add_theme_constant_override("separation", 6)
	interaction_input_container.add_child(input_hbox)
	
	interaction_line_edit = LineEdit.new()
	interaction_line_edit.placeholder_text = "Type command (e.g. chop briar patch with axe)"
	interaction_line_edit.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	interaction_line_edit.add_theme_font_size_override("font_size", 10)
	input_hbox.add_child(interaction_line_edit)
	
	interaction_send_btn = Button.new()
	interaction_send_btn.text = "Send"
	interaction_send_btn.add_theme_font_size_override("font_size", 10)
	input_hbox.add_child(interaction_send_btn)
	
	var submit_cmd = func():
		var cmd = interaction_line_edit.text.strip_edges()
		if cmd != "":
			if map_gen.is_player_in_holding():
				last_interaction_result = map_gen.interact_in_room(cmd)
			else:
				last_interaction_result = map_gen.interact_on_cell(selected_cell_idx, cmd)
			interaction_line_edit.text = ""
			_update_inspector()
	
	interaction_line_edit.text_submitted.connect(func(_new_text): submit_cmd.call())
	interaction_send_btn.pressed.connect(submit_cmd)

	inspector_panel.visible = false

# UI Population Helpers
func _add_slider(parent: Control, label_text: String, min_v: float, max_v: float, initial_v: float, callable: Callable, decimals = 0, step = 1.0):
	var vbox = VBoxContainer.new()
	vbox.add_theme_constant_override("separation", 2)
	parent.add_child(vbox)

	var hbox = HBoxContainer.new()
	vbox.add_child(hbox)

	var label = Label.new()
	label.text = label_text
	label.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	label.add_theme_font_size_override("font_size", 12)
	label.add_theme_color_override("font_color", Color("#cbd5e1"))
	hbox.add_child(label)

	var val_label = Label.new()
	val_label.text = String.num(initial_v, decimals)
	val_label.add_theme_font_size_override("font_size", 12)
	val_label.add_theme_color_override("font_color", Color("#38bdf8"))
	hbox.add_child(val_label)

	var slider = HSlider.new()
	slider.min_value = min_v
	slider.max_value = max_v
	slider.value = initial_v
	slider.step = step
	slider.value_changed.connect(func(v):
		val_label.text = String.num(v, decimals)
		callable.call(v)
	)
	vbox.add_child(slider)

func _add_seed_input(parent: Control):
	var vbox = VBoxContainer.new()
	vbox.add_theme_constant_override("separation", 2)
	parent.add_child(vbox)

	var label = Label.new()
	label.text = "Map Seed"
	label.add_theme_font_size_override("font_size", 12)
	label.add_theme_color_override("font_color", Color("#cbd5e1"))
	vbox.add_child(label)

	var hbox = HBoxContainer.new()
	hbox.add_theme_constant_override("separation", 6)
	vbox.add_child(hbox)

	var input = LineEdit.new()
	input.text = str(map_seed)
	input.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	input.text_submitted.connect(func(t):
		if t.is_valid_int():
			map_seed = int(t)
	)
	hbox.add_child(input)

	var rand_btn = Button.new()
	rand_btn.text = "Rand"
	rand_btn.pressed.connect(func():
		var r = randi() % 99999
		input.text = str(r)
		map_seed = r
	)
	hbox.add_child(rand_btn)

func _update_status():
	if current_step == 0:
		status_label.text = "Status: Empty Grid"
	else:
		status_label.text = "Status: Map Generated"

func _get_cell_at_position(pos: Vector2) -> int:
	if map_gen == null or map_gen.get_cell_count() == 0:
		return -1
		
	var estimated_col = int(pos.x / spacing_scale)
	var estimated_row = int(pos.y / (hex_size * 1.5))
	
	var best_idx = -1
	var min_dist = 999999.0
	
	# Check 5x5 cells around estimate to ensure perfect coverage
	for r in range(estimated_row - 2, estimated_row + 3):
		for c in range(estimated_col - 2, estimated_col + 3):
			if c >= 0 and c < map_width and r >= 0 and r < map_height:
				var idx = r * map_width + c
				var cell_pos = map_gen.get_cell_position(idx) * spacing_scale
				var dist = cell_pos.distance_to(pos)
				if dist < min_dist and dist < hex_size * 1.05:
					min_dist = dist
					best_idx = idx
	return best_idx

func _update_inspector():
	if selected_cell_idx == -1 or map_gen == null or map_gen.get_cell_count() == 0:
		inspector_panel.visible = false
		if interaction_input_container:
			interaction_input_container.visible = false
		return

	inspector_panel.visible = true
	
	# Clear previous contents
	for child in inspector_list.get_children():
		inspector_list.remove_child(child)
		child.queue_free()

	if map_gen.is_player_in_holding():
		if interaction_input_container:
			interaction_input_container.visible = true
		_render_mud_panel()
	else:
		var data = map_gen.get_cell_data(selected_cell_idx)
		if interaction_input_container:
			var is_land = data.get("elevation", 0.0) >= ocean_level
			interaction_input_container.visible = is_land
		_render_hex_info_panel(data)

func _render_hex_info_panel(data: Dictionary):
	# Coordinate title
	inspector_title.text = "Hex Info [ID: %d]" % data["index"]

	# Grid Coordinates
	_add_info_label(inspector_list, "Offset Coordinates:", "Col %d, Row %d" % [data["x"], data["y"]])

	# Feature Overlay
	if current_step >= 2:
		var overlay_type = data.get("overlay", "none")
		if overlay_type != "none":
			_add_info_label(inspector_list, "Feature Overlay:", overlay_type.replace("_", " ").capitalize(), Color("#fb923c"))

	# Rivers / Drainage
	if current_step >= 5:
		_add_info_label(inspector_list, "Water Drainage Area:", "%.1f cells" % data["water_accumulation"])
		var river_status = "Main Channel" if data["is_river"] else "None"
		_add_info_label(inspector_list, "River Stream:", river_status)

	# Temperature
	if current_step >= 6:
		_add_info_label(inspector_list, "Temperature (T):", "%.2f" % data["temperature"])

	# Biome
	if current_step >= 7:
		var b_id = data["biome"]
		var b_name = biome_names.get(b_id, "Unknown")
		if b_id == 0 and map_gen.is_shallow_water(selected_cell_idx):
			b_name = biome_names[13]
		_add_info_label(inspector_list, "Assigned Biome:", b_name, Color("#38bdf8"))

		# Region Names
		var land_name = data.get("landmass_name", "")
		if land_name != "":
			_add_info_label(inspector_list, "Landmass Name:", land_name, Color("#a855f7"))

		var biome_reg_name = data.get("biome_region_name", "")
		if biome_reg_name != "":
			_add_info_label(inspector_list, "Local Region:", biome_reg_name, Color("#22c55e"))

		var micro_reg_name = data.get("micro_region_name", "")
		if micro_reg_name != "":
			_add_info_label(inspector_list, "Landmark / Hollow:", micro_reg_name, Color("#eab308"))

		var mtn_range_name = data.get("mountain_range_name", "")
		if mtn_range_name != "":
			_add_info_label(inspector_list, "Mountain Range:", mtn_range_name, Color("#ef4444"))

		var riv_name = data.get("river_name", "")
		if riv_name != "":
			_add_info_label(inspector_list, "River System:", riv_name, Color("#3b82f6"))
	else:
		_add_info_label(inspector_list, "Biome:", "Uninitialized")

	# Holdings list
	if current_step >= 8:
		if data.has("holding_ids") and data["holding_ids"].size() > 0:
			var divider = ColorRect.new()
			divider.custom_minimum_size = Vector2(0, 1)
			divider.color = Color(0.3, 0.35, 0.45, 0.3)
			inspector_list.add_child(divider)
			
			var h_label = Label.new()
			h_label.text = "Local Holdings:"
			h_label.add_theme_font_size_override("font_size", 12)
			h_label.add_theme_color_override("font_color", Color("#cbd5e1"))
			inspector_list.add_child(h_label)
			
			for holding_id in data["holding_ids"]:
				var h_data = map_gen.get_holding_data(holding_id)
				if h_data.is_empty():
					continue
				
				# Container for this holding item (header + collapsible area)
				var h_item_container = VBoxContainer.new()
				inspector_list.add_child(h_item_container)
				
				var h_header_box = HBoxContainer.new()
				h_header_box.add_theme_constant_override("separation", 4)
				h_item_container.add_child(h_header_box)
				
				# Collapsible Toggle Button
				var toggle_btn = Button.new()
				toggle_btn.text = "▶"
				toggle_btn.flat = true
				toggle_btn.add_theme_font_size_override("font_size", 9)
				toggle_btn.custom_minimum_size = Vector2(16, 16)
				toggle_btn.size_flags_vertical = Control.SIZE_SHRINK_CENTER
				
				var has_tags = h_data["tags"].size() > 0
				if not has_tags:
					toggle_btn.disabled = true
					toggle_btn.text = "•"
				
				h_header_box.add_child(toggle_btn)
				
				var details_lbl = Label.new()
				details_lbl.text = "%s (%s)" % [h_data["name"], h_data["type"]]
				details_lbl.add_theme_font_size_override("font_size", 11)
				details_lbl.add_theme_color_override("font_color", Color("#38bdf8"))
				details_lbl.size_flags_horizontal = Control.SIZE_EXPAND_FILL
				h_header_box.add_child(details_lbl)
				
				var enter_btn = Button.new()
				enter_btn.text = "Enter"
				enter_btn.add_theme_font_size_override("font_size", 10)
				enter_btn.pressed.connect(func():
					if map_gen.enter_holding(holding_id):
						_update_inspector()
				)
				h_header_box.add_child(enter_btn)
				
				# Collapsible tags box
				if has_tags:
					var tags_container = MarginContainer.new()
					tags_container.add_theme_constant_override("margin_left", 20)
					tags_container.visible = false
					h_item_container.add_child(tags_container)
					
					var tags_lbl = Label.new()
					tags_lbl.text = "Tags: " + ", ".join(h_data["tags"])
					tags_lbl.add_theme_font_size_override("font_size", 10)
					tags_lbl.add_theme_color_override("font_color", Color("#34d399"))
					tags_lbl.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
					tags_container.add_child(tags_lbl)
					
					toggle_btn.pressed.connect(func():
						tags_container.visible = not tags_container.visible
						toggle_btn.text = "▼" if tags_container.visible else "▶"
					)

	# Last action log
	if last_interaction_result != "":
		var divider_log = ColorRect.new()
		divider_log.custom_minimum_size = Vector2(0, 1)
		divider_log.color = Color(0.3, 0.35, 0.45, 0.3)
		inspector_list.add_child(divider_log)
		
		var log_lbl = Label.new()
		log_lbl.text = "> " + last_interaction_result
		log_lbl.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
		log_lbl.add_theme_font_size_override("font_size", 11)
		
		if "successfully" in last_interaction_result:
			log_lbl.add_theme_color_override("font_color", Color("#4ade80")) # bright green
		else:
			log_lbl.add_theme_color_override("font_color", Color("#f87171")) # soft red
		inspector_list.add_child(log_lbl)

	# Divider for interactables
	var divider_int = ColorRect.new()
	divider_int.custom_minimum_size = Vector2(0, 1)
	divider_int.color = Color(0.3, 0.35, 0.45, 0.3)
	inspector_list.add_child(divider_int)

	# Interactables header
	var int_header = Label.new()
	int_header.text = "Hex Interactables:"
	int_header.add_theme_font_size_override("font_size", 11)
	int_header.add_theme_color_override("font_color", Color("#e2e8f0"))
	inspector_list.add_child(int_header)

	var interactables = data.get("interactables", [])
	if interactables.size() > 0:
		for obj in interactables:
			var obj_box = HBoxContainer.new()
			inspector_list.add_child(obj_box)

			var status_lbl = Label.new()
			if obj["is_depleted"]:
				status_lbl.text = "[Depleted] "
				status_lbl.add_theme_color_override("font_color", Color("#64748b"))
			else:
				status_lbl.text = "[Active] "
				status_lbl.add_theme_color_override("font_color", Color("#34d399"))
			status_lbl.add_theme_font_size_override("font_size", 10)
			obj_box.add_child(status_lbl)

			var name_lbl = Label.new()
			name_lbl.text = obj["name"]
			name_lbl.add_theme_font_size_override("font_size", 10)
			name_lbl.add_theme_color_override("font_color", Color("#e2e8f0") if not obj["is_depleted"] else Color("#64748b"))
			obj_box.add_child(name_lbl)

			var action_lbl = Label.new()
			if not obj["is_depleted"]:
				action_lbl.text = " (Verbs: " + ", ".join(obj["allowed_actions"]) + ")"
				action_lbl.add_theme_color_override("font_color", Color("#a7f3d0"))
			else:
				action_lbl.text = " (Depleted)"
				action_lbl.add_theme_color_override("font_color", Color("#475569"))
			action_lbl.add_theme_font_size_override("font_size", 9)
			obj_box.add_child(action_lbl)

			name_lbl.tooltip_text = obj["description"]
	else:
		var no_obj = Label.new()
		no_obj.text = "  No objects of interest on this tile."
		no_obj.add_theme_font_size_override("font_size", 9)
		no_obj.add_theme_color_override("font_color", Color("#64748b"))
		inspector_list.add_child(no_obj)

func _render_mud_panel():
	var room = map_gen.get_player_current_room_data()
	if not room.get("in_holding", false):
		map_gen.exit_holding()
		last_interaction_result = ""
		_update_inspector()
		return

	# Title
	inspector_title.text = "Holding: %s" % room["holding_name"]

	# Room Name
	var room_name_lbl = Label.new()
	room_name_lbl.text = room["room_name"]
	room_name_lbl.add_theme_font_size_override("font_size", 13)
	room_name_lbl.add_theme_color_override("font_color", Color("#fb923c"))
	inspector_list.add_child(room_name_lbl)

	# Room Tags
	if room["room_tags"].size() > 0:
		var tags_lbl = Label.new()
		tags_lbl.text = "Tags: " + ", ".join(room["room_tags"])
		tags_lbl.add_theme_font_size_override("font_size", 10)
		tags_lbl.add_theme_color_override("font_color", Color("#34d399"))
		inspector_list.add_child(tags_lbl)

	# Divider
	var div = ColorRect.new()
	div.custom_minimum_size = Vector2(0, 1)
	div.color = Color(0.3, 0.35, 0.45, 0.3)
	inspector_list.add_child(div)

	# Room Description
	var desc_lbl = Label.new()
	desc_lbl.text = room["room_description"]
	desc_lbl.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	desc_lbl.add_theme_font_size_override("font_size", 11)
	desc_lbl.add_theme_color_override("font_color", Color("#cbd5e1"))
	inspector_list.add_child(desc_lbl)

	# Last action log
	if last_interaction_result != "":
		var log_lbl = Label.new()
		log_lbl.text = "> " + last_interaction_result
		log_lbl.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
		log_lbl.add_theme_font_size_override("font_size", 11)
		
		# Color based on whether success or error
		if "successfully" in last_interaction_result:
			log_lbl.add_theme_color_override("font_color", Color("#4ade80")) # bright green
		else:
			log_lbl.add_theme_color_override("font_color", Color("#f87171")) # soft red
		inspector_list.add_child(log_lbl)

	# Divider for interactables
	var div_int = ColorRect.new()
	div_int.custom_minimum_size = Vector2(0, 1)
	div_int.color = Color(0.3, 0.35, 0.45, 0.3)
	inspector_list.add_child(div_int)

	# Interactables header
	var int_header = Label.new()
	int_header.text = "Interactables:"
	int_header.add_theme_font_size_override("font_size", 11)
	int_header.add_theme_color_override("font_color", Color("#e2e8f0"))
	inspector_list.add_child(int_header)

	var interactables = room.get("interactables", [])
	if interactables.size() > 0:
		for obj in interactables:
			var obj_box = HBoxContainer.new()
			inspector_list.add_child(obj_box)

			var status_lbl = Label.new()
			if obj["is_depleted"]:
				status_lbl.text = "[Depleted] "
				status_lbl.add_theme_color_override("font_color", Color("#64748b"))
			else:
				status_lbl.text = "[Active] "
				status_lbl.add_theme_color_override("font_color", Color("#34d399"))
			status_lbl.add_theme_font_size_override("font_size", 10)
			obj_box.add_child(status_lbl)

			var name_lbl = Label.new()
			name_lbl.text = obj["name"]
			name_lbl.add_theme_font_size_override("font_size", 10)
			name_lbl.add_theme_color_override("font_color", Color("#e2e8f0") if not obj["is_depleted"] else Color("#64748b"))
			obj_box.add_child(name_lbl)

			var action_lbl = Label.new()
			if not obj["is_depleted"]:
				action_lbl.text = " (Verbs: " + ", ".join(obj["allowed_actions"]) + ")"
				action_lbl.add_theme_color_override("font_color", Color("#a7f3d0"))
			else:
				action_lbl.text = " (Depleted)"
				action_lbl.add_theme_color_override("font_color", Color("#475569"))
			action_lbl.add_theme_font_size_override("font_size", 9)
			obj_box.add_child(action_lbl)

			# Show description on hover or tooltip
			name_lbl.tooltip_text = obj["description"]
	else:
		var no_obj = Label.new()
		no_obj.text = "  No objects of interest here."
		no_obj.add_theme_font_size_override("font_size", 9)
		no_obj.add_theme_color_override("font_color", Color("#64748b"))
		inspector_list.add_child(no_obj)

	# Divider before exit info
	var div_exits = ColorRect.new()
	div_exits.custom_minimum_size = Vector2(0, 1)
	div_exits.color = Color(0.3, 0.35, 0.45, 0.3)
	inspector_list.add_child(div_exits)

	# Exit info
	var exits = room["exits"]
	var valid_exits = []
	for dir_name in exits.keys():
		if exits[dir_name] != -1:
			valid_exits.append(dir_name.capitalize())
	
	var exits_lbl = Label.new()
	if valid_exits.size() > 0:
		exits_lbl.text = "Obvious exits: " + ", ".join(valid_exits)
	else:
		exits_lbl.text = "Obvious exits: None"
	exits_lbl.add_theme_font_size_override("font_size", 10)
	exits_lbl.add_theme_color_override("font_color", Color("#94a3b8"))
	inspector_list.add_child(exits_lbl)

	# Traversal Buttons grid
	var grid = GridContainer.new()
	grid.columns = 3
	grid.size_flags_horizontal = Control.SIZE_SHRINK_CENTER
	grid.add_theme_constant_override("h_separation", 6)
	grid.add_theme_constant_override("v_separation", 6)
	inspector_list.add_child(grid)

	var buttons_layout = [
		{"label": "Up", "dir": 4},   {"label": "North", "dir": 0}, {"label": "In", "dir": 6},
		{"label": "West", "dir": 3}, {"label": "", "dir": -1},    {"label": "East", "dir": 2},
		{"label": "Down", "dir": 5}, {"label": "South", "dir": 1}, {"label": "Out", "dir": 7}
	]

	for btn_info in buttons_layout:
		if btn_info["label"] == "":
			var spacer = Control.new()
			grid.add_child(spacer)
			continue

		var btn = Button.new()
		btn.text = btn_info["label"]
		btn.custom_minimum_size = Vector2(64, 26)
		btn.add_theme_font_size_override("font_size", 9)
		
		var dir_enum_name = btn_info["label"].to_lower()
		var next_room_idx = exits.get(dir_enum_name, -1)
		if next_room_idx == -1:
			btn.disabled = true
		else:
			btn.pressed.connect(func():
				if map_gen.move_player_dir(btn_info["dir"]):
					last_interaction_result = ""
					_update_inspector()
			)
		grid.add_child(btn)

	# Exit button
	var exit_btn = Button.new()
	exit_btn.text = "Return to Map"
	exit_btn.add_theme_font_size_override("font_size", 10)
	exit_btn.pressed.connect(func():
		map_gen.exit_holding()
		last_interaction_result = ""
		_update_inspector()
	)
	inspector_list.add_child(exit_btn)

func _add_info_label(parent: Control, label_text: String, value_text: String, value_color = Color("#f8fafc")):
	var hbox = HBoxContainer.new()
	parent.add_child(hbox)
	
	var label = Label.new()
	label.text = label_text
	label.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	label.add_theme_font_size_override("font_size", 11)
	label.add_theme_color_override("font_color", Color("#94a3b8"))
	hbox.add_child(label)

	var value = Label.new()
	value.text = value_text
	value.add_theme_font_size_override("font_size", 11)
	value.add_theme_color_override("font_color", value_color)
	hbox.add_child(value)

func _update_legend():
	# Clear previous legend items
	for child in legend_list.get_children():
		legend_list.remove_child(child)
		child.queue_free()

	if active_layer == "biomes" and current_step >= 7:
		var order = [0, 13, 14, 1, 15, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12]
		for b_id in order:
			if biome_names.has(b_id):
				_add_legend_item(legend_list, biome_names[b_id], biome_colors[b_id])
	elif active_layer == "elevation":
		_add_legend_item(legend_list, "High Peaks (>= 0.85)", Color("#f8fafc"))
		_add_legend_item(legend_list, "Low Ranges (0.7-0.85)", Color("#b0b5bd"))
		_add_legend_item(legend_list, "Hills / Uplift (0.35-0.7)", Color("#918b82"))
		_add_legend_item(legend_list, "Plains / Land (0.05-0.35)", Color("#719c62"))
		_add_legend_item(legend_list, "Sandy Coast (0.0-0.05)", Color("#dec595"))
		_add_legend_item(legend_list, "Shallow Waters (<0.0)", Color("#1d3b5a"))
		_add_legend_item(legend_list, "Deep Basins (< -0.15)", Color("#0f1b29"))
	elif active_layer == "temp":
		_add_legend_item(legend_list, "Hot (>0.7)", Color("#d7191c"))
		_add_legend_item(legend_list, "Warm (0.4-0.7)", Color("#fdae61"))
		_add_legend_item(legend_list, "Cool (0.2-0.4)", Color("#abd9e9"))
		_add_legend_item(legend_list, "Freezing (<0.2)", Color("#2c7bb6"))
	elif active_layer == "moisture":
		_add_legend_item(legend_list, "Super Wet / Rain Forest (>0.7)", Color("#018571"))
		_add_legend_item(legend_list, "Moist / Forest (0.4-0.7)", Color("#80cdc1"))
		_add_legend_item(legend_list, "Dry / Grasslands (0.2-0.4)", Color("#dfc27d"))
		_add_legend_item(legend_list, "Arid / Desert (<0.2)", Color("#a6611a"))
	else:
		var empty_lbl = Label.new()
		empty_lbl.text = "No legend available for current layer/step."
		empty_lbl.add_theme_font_size_override("font_size", 10)
		empty_lbl.add_theme_color_override("font_color", Color("#64748b"))
		legend_list.add_child(empty_lbl)

func _add_legend_item(parent: Control, text_label: String, color: Color):
	var hbox = HBoxContainer.new()
	hbox.add_theme_constant_override("separation", 8)
	parent.add_child(hbox)

	var rect = ColorRect.new()
	rect.color = color
	rect.custom_minimum_size = Vector2(14, 14)
	rect.size_flags_vertical = Control.SIZE_SHRINK_CENTER
	hbox.add_child(rect)

	var label = Label.new()
	label.text = text_label
	label.add_theme_font_size_override("font_size", 10)
	label.add_theme_color_override("font_color", Color("#cbd5e1"))
	hbox.add_child(label)

