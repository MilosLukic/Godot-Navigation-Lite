extends ColorRect

var ids = []
var colors = []
var thickness = []
var a_s = []
var b_s = []

var lines = []
var camera_node = null

func _ready():
	camera_node = $"../Camera"

func _draw():
	for index in range(ids.size()):
		draw_line(a_s[index], b_s[index], colors[index], thickness[index])

func _process(_delta):
	update()

func draw_line3D(given_id, vector_a, vector_b, color, given_thickness):
	for index in range(ids.size()):
		if ids[index] == given_id:
			colors[index] = color
			a_s[index] = camera_node.unproject_position(vector_a)
			b_s[index] = camera_node.unproject_position(vector_b)
			thickness[index] = given_thickness
			return
			
	ids.append(given_id)
	colors.append(color)
	a_s.append(camera_node.unproject_position(vector_a))
	b_s.append(camera_node.unproject_position(vector_b))
	thickness.append(given_thickness)

func remove_lines():
	ids = []
	colors = []
	thickness = []
	a_s = []
	b_s = []

