extends Node2D

@onready var main = get_parent().get_parent()

func _draw() -> void:
	if main == null or main.map_gen == null or main.map_gen.get_cell_count() == 0:
		return
	
	var map_gen = main.map_gen
	var hex_size = main.hex_size
	var spacing_scale = main.spacing_scale
	var ocean_level = main.ocean_level
	var map_seed = main.map_seed
	var current_step = main.current_step
	var active_layer = main.active_layer
	var show_rivers = main.show_rivers
	var show_borders = main.show_borders
	var show_plate_vel = main.show_plate_vel
	var hovered_cell_idx = main.hovered_cell_idx
	var selected_cell_idx = main.selected_cell_idx
	var biome_colors = main.biome_colors
	
	var elevations = map_gen.get_elevations()
	var overlays = map_gen.get_overlays()
	var moistures = map_gen.get_moistures()
	var temperatures = map_gen.get_temperatures()
	var biomes = map_gen.get_biomes()
	var plates = map_gen.get_plates()
	var water_accum = map_gen.get_water_accumulations()

	var plate_colors = []
	if current_step >= 1:
		var n_plates = map_gen.get_num_plates()
		var rng_col = RandomNumberGenerator.new()
		rng_col.seed = map_seed + 100
		for p in range(n_plates):
			plate_colors.append(Color.from_hsv(rng_col.randf(), 0.7, 0.85))

	for i in range(map_gen.get_cell_count()):
		var screen_pos = map_gen.get_cell_position(i) * spacing_scale
		var elev = elevations[i]
		var color = Color("#1e293b")

		match active_layer:
			"biomes":
				if current_step >= 7:
					var b_id = biomes[i]
					if b_id == 0 and map_gen.is_shallow_water(i):
						color = biome_colors[13]
					else:
						color = biome_colors.get(b_id, Color("#1e293b"))
				elif current_step >= 2:
					if elev < ocean_level and map_gen.is_shallow_water(i):
						color = biome_colors[13]
					else:
						color = _get_elevation_color(elev, ocean_level)
			"elevation":
				if current_step >= 2:
					color = _get_elevation_color(elev, ocean_level)
				else:
					color = Color("#1e293b")
			"plates":
				if current_step >= 1 and plates[i] >= 0 and plates[i] < plate_colors.size():
					color = plate_colors[plates[i]]
				else:
					color = Color("#1e293b")
			"temp":
				if current_step >= 6:
					var t = temperatures[i]
					color = Color("#2c7bb6").lerp(Color("#fdae61"), t).lerp(Color("#d7191c"), clamp((t-0.5)*2, 0.0, 1.0) if t > 0.5 else 0.0)
				else:
					color = Color("#1e293b")
			"moisture":
				if current_step >= 6:
					var m = moistures[i]
					color = Color("#dfc27d").lerp(Color("#f5f5f5"), m * 1.5).lerp(Color("#018571"), clamp((m-0.5)*2.0, 0.0, 1.0) if m > 0.5 else 0.0)
				else:
					color = Color("#1e293b")
			"water":
				if current_step >= 5:
					if elev < ocean_level:
						color = Color("#0f1b29")
					else:
						var accum = water_accum[i]
						var t = clamp(accum / 25.0, 0.0, 1.0)
						color = Color("#3f3322").lerp(Color("#3b82f6"), t)
				else:
					color = Color("#1e293b")

		var pts = _get_hex_points(screen_pos, hex_size)
		draw_polygon(pts, PackedColorArray([color]))

		if show_borders:
			var border_pts = pts
			border_pts.append(pts[0])
			draw_polyline(border_pts, Color(0, 0, 0, 0.1), 1.0, true)

		# Draw mountain symbol or forest symbol if applicable
		var overlay = overlays[i] if current_step >= 2 else "none"
		var drew_hill = false
		if overlay == "high_mountain":
			_draw_mountain_symbol(screen_pos, hex_size, Color("#80858c"))
		elif overlay == "low_mountain":
			_draw_low_mountain_symbol(screen_pos, hex_size, Color("#80858c"))
		elif overlay == "hill" and current_step >= 5:
			_draw_hill_symbol(screen_pos, hex_size, Color("#8b867a"))
			drew_hill = true
		
		# Draw forest / waves if not mountain / hill
		if overlay != "high_mountain" and overlay != "low_mountain":
			if current_step >= 7 and biomes[i] in [4, 7, 8, 11, 12]:
				if drew_hill:
					_draw_tree_symbol(screen_pos + Vector2(hex_size * 0.25, hex_size * 0.1), hex_size * 0.28, Color("#1a472a"))
				else:
					_draw_forest_symbols(screen_pos, hex_size, Color("#1a472a"))
			elif current_step >= 2 and elev < ocean_level and map_gen.is_shallow_water(i):
				if biomes[i] != 14: # Not Lake
					_draw_wave_symbol(screen_pos, hex_size, Color("#ffffff", 0.22))

		# Draw plate velocities
		if active_layer == "plates" and show_plate_vel and current_step >= 1:
			var p_id = plates[i]
			if p_id >= 0:
				var vel = map_gen.get_plate_velocity(p_id)
				if vel.length() > 0.05:
					var hash_val = (i * 73856093) ^ (p_id * 19349663)
					if (hash_val % 18) == 0:
						_draw_arrow(screen_pos, vel * 16.0, Color(1, 1, 1, 0.8))

	# Draw Rivers Overlay
	if show_rivers and current_step >= 5:
		var river_paths = map_gen.get_river_paths()
		for path in river_paths:
			var screen_pts = PackedVector2Array()
			for pt in path:
				screen_pts.append(pt * spacing_scale)
			if screen_pts.size() > 1:
				draw_polyline(screen_pts, Color("#3b82f6"), 2.2, true)

	# Draw hover outline
	if hovered_cell_idx != -1:
		var screen_pos = map_gen.get_cell_position(hovered_cell_idx) * spacing_scale
		var pts = _get_hex_points(screen_pos, hex_size)
		pts.append(pts[0])
		draw_polyline(pts, Color(1, 1, 1, 0.7), 2.0, true)

	# Draw selection outline
	if selected_cell_idx != -1:
		var screen_pos = map_gen.get_cell_position(selected_cell_idx) * spacing_scale
		var pts = _get_hex_points(screen_pos, hex_size)
		pts.append(pts[0])
		draw_polyline(pts, Color(0.95, 0.75, 0.15, 1.0), 3.0, true)

func _get_elevation_color(elev: float, ocean_level: float) -> Color:
	if elev < ocean_level:
		var t = clamp((elev - -0.2) / (ocean_level - -0.2), 0.0, 1.0)
		return Color("#0f1b29").lerp(Color("#1d3b5a"), t)
	else:
		var t = clamp((elev - ocean_level) / (1.2 - ocean_level), 0.0, 1.0)
		if t < 0.05:
			return Color("#dec595")
		elif t < 0.35:
			return Color("#719c62").lerp(Color("#94b37d"), (t - 0.05) / 0.30)
		elif t < 0.7:
			return Color("#94b37d").lerp(Color("#918b82"), (t - 0.35) / 0.35)
		else:
			return Color("#918b82").lerp(Color("#f8fafc"), (t - 0.7) / 0.3)

func _get_hex_points(center: Vector2, hex_size: float) -> PackedVector2Array:
	var pts = PackedVector2Array()
	for i in range(6):
		var angle_deg = 60.0 * i - 30.0
		var angle_rad = deg_to_rad(angle_deg)
		pts.append(center + Vector2(cos(angle_rad), sin(angle_rad)) * hex_size)
	return pts

func _draw_arrow(start: Vector2, direction: Vector2, color: Color) -> void:
	var end = start + direction
	draw_line(start, end, color, 2.0, true)
	var angle = direction.angle()
	var size = 4.0
	var tip1 = end + Vector2.from_angle(angle + PI * 0.82) * size
	var tip2 = end + Vector2.from_angle(angle - PI * 0.82) * size
	draw_line(end, tip1, color, 1.8, true)
	draw_line(end, tip2, color, 1.8, true)

func _draw_wave_symbol(center: Vector2, size: float, color: Color) -> void:
	var wave_w = size * 0.45
	var wave_h = size * 0.08
	var p0 = center + Vector2(-wave_w * 0.5, -size * 0.1)
	var p1 = center + Vector2(-wave_w * 0.15, -size * 0.1 - wave_h)
	var p2 = center + Vector2(wave_w * 0.15, -size * 0.1 + wave_h)
	var p3 = center + Vector2(wave_w * 0.5, -size * 0.1)
	draw_polyline(PackedVector2Array([p0, p1, p2, p3]), color, 1.3, true)
	
	var q0 = center + Vector2(-wave_w * 0.3, size * 0.12)
	var q1 = center + Vector2(-wave_w * 0.05, size * 0.12 - wave_h)
	var q2 = center + Vector2(wave_w * 0.2, size * 0.12 + wave_h)
	var q3 = center + Vector2(wave_w * 0.4, size * 0.12)
	draw_polyline(PackedVector2Array([q0, q1, q2, q3]), color, 1.3, true)

func _draw_mountain_symbol(center: Vector2, size: float, color: Color) -> void:
	var main_peak = center + Vector2(0, -size * 0.5)
	var main_left = center + Vector2(-size * 0.6, size * 0.4)
	var main_right = center + Vector2(size * 0.6, size * 0.4)
	
	var sec_peak = center + Vector2(-size * 0.35, -size * 0.25)
	var sec_left = center + Vector2(-size * 0.8, size * 0.4)
	var sec_right = center + Vector2(size * 0.1, size * 0.4)

	var sec_pts = PackedVector2Array([sec_peak, sec_right, sec_left])
	var sec_col = color.darkened(0.18)
	draw_polygon(sec_pts, PackedColorArray([sec_col]))
	
	var main_pts = PackedVector2Array([main_peak, main_right, main_left])
	draw_polygon(main_pts, PackedColorArray([color]))
	
	var cap_tip = main_peak
	var cap_left = main_peak + (main_left - main_peak) * 0.35
	var cap_right = main_peak + (main_right - main_peak) * 0.35
	var cap_mid = main_peak + Vector2(0, size * 0.18)
	var cap_pts = PackedVector2Array([cap_tip, cap_right, cap_mid, cap_left])
	draw_polygon(cap_pts, PackedColorArray([Color("#ffffff")]))

	var outline_color = Color(0, 0, 0, 0.25)
	draw_line(sec_left, sec_peak, outline_color, 1.2, true)
	draw_line(sec_peak, sec_right, outline_color, 1.2, true)
	draw_line(main_left, main_peak, outline_color, 1.5, true)
	draw_line(main_peak, main_right, outline_color, 1.5, true)
	draw_line(main_left, main_right, outline_color, 1.2, true)

func _draw_low_mountain_symbol(center: Vector2, size: float, color: Color) -> void:
	var s = size * 0.72
	var main_peak = center + Vector2(0, -s * 0.45)
	var main_left = center + Vector2(-s * 0.55, s * 0.35)
	var main_right = center + Vector2(s * 0.55, s * 0.35)
	
	var sec_peak = center + Vector2(-s * 0.32, -s * 0.22)
	var sec_left = center + Vector2(-s * 0.75, s * 0.35)
	var sec_right = center + Vector2(s * 0.1, s * 0.35)

	var sec_pts = PackedVector2Array([sec_peak, sec_right, sec_left])
	var sec_col = color.darkened(0.18)
	draw_polygon(sec_pts, PackedColorArray([sec_col]))
	
	var main_pts = PackedVector2Array([main_peak, main_right, main_left])
	draw_polygon(main_pts, PackedColorArray([color]))
	
	var outline_color = Color(0, 0, 0, 0.25)
	draw_line(sec_left, sec_peak, outline_color, 1.1, true)
	draw_line(sec_peak, sec_right, outline_color, 1.1, true)
	draw_line(main_left, main_peak, outline_color, 1.3, true)
	draw_line(main_peak, main_right, outline_color, 1.3, true)
	draw_line(main_left, main_right, outline_color, 1.1, true)

func _draw_tree_symbol(center: Vector2, size: float, color: Color) -> void:
	var top_tip = center + Vector2(0, -size * 0.4)
	var left = center + Vector2(-size * 0.3, size * 0.3)
	var right = center + Vector2(size * 0.3, size * 0.3)
	var trunk_top = center + Vector2(0, size * 0.3)
	var trunk_bottom = center + Vector2(0, size * 0.45)
	
	draw_line(trunk_top, trunk_bottom, Color("#5c4033"), 1.8, true)
	draw_polygon(PackedVector2Array([top_tip, right, left]), PackedColorArray([color]))
	draw_polyline(PackedVector2Array([left, top_tip, right, left]), Color(0, 0, 0, 0.2), 1.0, true)

func _draw_forest_symbols(center: Vector2, hex_r: float, color: Color) -> void:
	var size = hex_r * 0.38
	_draw_tree_symbol(center + Vector2(-hex_r * 0.22, hex_r * 0.1), size, color)
	_draw_tree_symbol(center + Vector2(hex_r * 0.18, -hex_r * 0.1), size * 1.1, color.darkened(0.12))

func _draw_hill_symbol(center: Vector2, size: float, color: Color) -> void:
	var center1 = center + Vector2(-size * 0.22, size * 0.05)
	var w1 = size * 0.55
	var h1 = size * 0.25
	_draw_hump(center1, w1, h1, color.darkened(0.12))
	
	var center2 = center + Vector2(0.18 * size, 0.18 * size)
	var w2 = size * 0.65
	var h2 = size * 0.3
	_draw_hump(center2, w2, h2, color)

func _draw_hump(c: Vector2, w: float, h: float, color: Color) -> void:
	var points = PackedVector2Array()
	var steps = 12
	for step in range(steps + 1):
		var t = float(step) / steps
		var x = -w * 0.5 + t * w
		var norm_x = (t - 0.5) * 2.0
		var y = -h * (1.0 - norm_x * norm_x)
		points.append(c + Vector2(x, y))
	
	points.append(c + Vector2(w * 0.5, 0))
	points.append(c + Vector2(-w * 0.5, 0))
	
	draw_polygon(points, PackedColorArray([color]))
	
	var curve_points = PackedVector2Array()
	for step in range(steps + 1):
		var t = float(step) / steps
		var x = -w * 0.5 + t * w
		var norm_x = (t - 0.5) * 2.0
		var y = -h * (1.0 - norm_x * norm_x)
		curve_points.append(c + Vector2(x, y))
	draw_polyline(curve_points, Color(0, 0, 0, 0.28), 1.3, true)
