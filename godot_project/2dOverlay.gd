extends ColorRect

class Line:

	var id
	var color
	var thickness
	var a = Vector2()
	var b = Vector2()

var lines = []
var camera_node

func _ready():

	camera_node = $"../Camera"

func _draw():

	for line in lines:

		draw_line(line.a, line.b, line.color, line.thickness)

func _process(delta):

	update()

func draw_line3D(id, vector_a, vector_b, color, thickness):
	for line in lines:
		if line.id == id:
			line.color = color
			line.a = camera_node.unproject_position(vector_a)
			line.b = camera_node.unproject_position(vector_b)
			line.thickness = thickness
			return
	var new_line = Line.new()
	new_line.id = id
	new_line.color = color
	new_line.a = camera_node.unproject_position(vector_a)
	new_line.b = camera_node.unproject_position(vector_b)
	new_line.thickness = thickness

	lines.append(new_line)

func remove_lines():
	lines = []
