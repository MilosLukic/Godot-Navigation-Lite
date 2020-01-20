extends Spatial
tool

onready var to_load_script = preload("res://addons/detour-navigation/bin/detour_navigation_mesh_cached.gdns")
func _ready():
	set_script(to_load_script)
