extends Camera2D

var is_dragging = false

func _unhandled_input(event: InputEvent) -> void:
	# Camera Pan
	if event is InputEventMouseButton:
		if event.button_index == MOUSE_BUTTON_RIGHT or event.button_index == MOUSE_BUTTON_MIDDLE:
			if event.pressed:
				is_dragging = true
			else:
				is_dragging = false
		
		# Camera Zoom
		elif event.button_index == MOUSE_BUTTON_WHEEL_UP and event.pressed:
			_zoom_at_mouse(1.1)
		elif event.button_index == MOUSE_BUTTON_WHEEL_DOWN and event.pressed:
			_zoom_at_mouse(0.9)
			
	elif event is InputEventMouseMotion and is_dragging:
		position -= event.relative / zoom

func _zoom_at_mouse(factor: float) -> void:
	var mouse_pos = get_viewport().get_mouse_position()
	var prev_zoom = zoom
	var next_zoom = prev_zoom * factor
	next_zoom = next_zoom.clamp(Vector2(0.12, 0.12), Vector2(6.0, 6.0))
	if prev_zoom == next_zoom:
		return
		
	var global_mouse_pos = get_global_mouse_position()
	zoom = next_zoom
	position = global_mouse_pos - (mouse_pos - get_viewport().size / 2.0) / next_zoom
