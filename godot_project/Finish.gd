extends MeshInstance

var process_once = true
onready var start = $"../Start"
onready var camera = $"../../Camera"
onready var nav = $"../CachedDetourNavigationMesh"
onready var geom = $"../../GeomDraw"

var line_id = 0
var path = 0


var colors = [Color(0.5, 0, 0.7), Color(0.7, 0.5, 0), Color(1, 0.2, 0.7), Color(1, 0, 0), Color(1, 0, 1),
Color(1, 1, 0), Color(1, 0, 0), Color(0, 1, 1), Color(0, 1, 0), Color(0, 0, 1), Color(0, 0, 0)]

func _ready():
	"""
	var t = get_global_transform()
	t.origin.x = 191.8
	t.origin.z = 58.5
	
	set_global_transform(t)
	draw_path()
	
	t.origin.x = 189.5
	t.origin.z = 52.5
	
	set_global_transform(t)
	draw_path()"""
	

func _process(delta):
	if Input.is_action_just_pressed("left_click"):
		var m_pos = get_viewport().get_mouse_position()
		var new_origin = raycast_from_mouse(m_pos, 1)
		var t = get_global_transform()
		if new_origin:
			t.origin = new_origin["position"]
			set_global_transform(t)
			draw_path()
			
	if Input.is_action_just_pressed("right_click"):
		var m_pos = get_viewport().get_mouse_position()
		var new_origin = raycast_from_mouse(m_pos, 1)
		var t = start.get_global_transform()
		if new_origin:
			t.origin = new_origin["position"]
			start.set_global_transform(t)
			draw_path()
	
func draw_path():
	var start_point = start.get_transform().origin
	var end_point = get_transform().origin
	var path_origins = nav.find_path(start_point, end_point)["points"]
	print(path_origins)
	
	if not path_origins:
		return
	geom.remove_lines()
	line_id += 1
	geom.draw_line3D(line_id, start_point, path_origins[0], colors[path % len(colors)], 3)
	for i in range(1, path_origins.size()):
		line_id += 1
		geom.draw_line3D(line_id, path_origins[i-1], path_origins[i], colors[path % len(colors)], 3)
	path += 1

func raycast_from_mouse(m_pos, collision_mask):
	var ray_start = camera.project_ray_origin(m_pos)
	var ray_end = ray_start + camera.project_ray_normal(m_pos) * 500
	var space_state = get_world().direct_space_state
	return space_state.intersect_ray(ray_start, ray_end)
