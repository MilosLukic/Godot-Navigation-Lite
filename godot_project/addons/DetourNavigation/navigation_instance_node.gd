tool
extends EditorPlugin

func _enter_tree():
	# When this plugin node enters tree, add the custom type
	add_custom_type("DetourNavigationMeshInstance", "Spatial", preload("res://addons/DetourNavigation/navigation_mesh_instance.gd"), preload("res://addons/DetourNavigation/heart_icon.png"))

func _exit_tree():
	# When the plugin node exits the tree, remove the custom type
	remove_custom_type("DetourNavigationMeshInstance")
