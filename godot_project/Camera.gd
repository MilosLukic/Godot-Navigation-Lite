extends Camera


var staticObst = preload("res://StaticBoxPillar.tscn")
var dynamicObst = preload("res://DynamicRoundPillar.tscn")

func _process(_delta):
	if Input.is_action_just_pressed("add_static"):
		manage_click(staticObst)
	if Input.is_action_just_pressed("add_dynamic"):
		manage_click(dynamicObst)
	

func manage_click(collision_template):
	var m_pos = get_viewport().get_mouse_position()
	var collision = raycast_from_mouse(m_pos, 1048575)
	if collision and collision.collider and collision.collider.get_collision_layer() & 1:
		add_collision(collision.position, collision_template)
	elif collision and collision.collider:
		remove_collision(collision.collider)

func add_collision(position, collision_template):
	var newStatObst = collision_template.instance()
	var new_transform = newStatObst.get_transform()
	new_transform.origin = position
	newStatObst.set_global_transform(new_transform)
	$"../DetourNavigation".add_child(newStatObst)
	
	
func remove_collision(collision):
	collision.get_parent().queue_free()
	

func raycast_from_mouse(m_pos, collision_mask):
	var ray_start = project_ray_origin(m_pos)
	var ray_end = ray_start + project_ray_normal(m_pos) * 150
	var space_state = get_world().direct_space_state
	return space_state.intersect_ray(ray_start, ray_end, [], collision_mask)
