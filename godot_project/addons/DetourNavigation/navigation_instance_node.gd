tool
extends EditorPlugin

var _menu_button = null
var _nodes_to_free_on_exit = []
const CREATE_CACHED_NAVMESH = 0
const CREATE_NAVMESH = 1
var currently_selected = null
onready var new_scr = preload("res://bin/detour_navigation.gdns")

func _enter_tree():
	# When this plugin node enters tree, add the custom type
	add_custom_type("DetourNavigationMeshInstance", "Spatial", preload("res://addons/DetourNavigation/navigation_mesh_instance.gd"), preload("res://addons/DetourNavigation/heart_icon.png"))

	var editor_interface = get_editor_interface()
	var base_control = editor_interface.get_base_control()
	
	_menu_button = MenuButton.new()
	_menu_button.text = "Navigation Mesh"
	_menu_button.get_popup().add_item("Create navmesh", CREATE_NAVMESH)
	_menu_button.get_popup().add_item("Create Cached Navmesh", CREATE_CACHED_NAVMESH)
	_menu_button.get_popup().connect("id_pressed", self, "_on_menu_id_pressed")
	add_control_to_container(CONTAINER_SPATIAL_EDITOR_MENU, _menu_button)
	_menu_button.hide()
	_nodes_to_free_on_exit.append(_menu_button)

func handles(object):
	if object.has_method("create_navmesh"):
		object.set_script(new_scr)
		_menu_button.show()
		currently_selected = object
	elif _menu_button:
		currently_selected = null
		_menu_button.hide()


func _exit_tree():
	# When the plugin node exits the tree, remove the custom type
	remove_custom_type("DetourNavigationMeshInstance")

	remove_control_from_container(CONTAINER_TOOLBAR, _menu_button)
	for node in _nodes_to_free_on_exit:
		node.queue_free()


func _on_menu_id_pressed(id):
	if id == CREATE_CACHED_NAVMESH:
		print("Creatingcached  navmesh...")
		currently_selected.create_cached_navmesh()
		print("What is happening...")
	elif id == CREATE_NAVMESH:
		print("Creating navmesh...")
		currently_selected.create_navmesh()
		#var cached_navmesh = preload("res://bin/detour_navigation_mesh_cached.gdns")
		#var instanced_navmesh = cached_navmesh.new()
		#currently_selected.add_child(instanced_navmesh)
		#instanced_navmesh.set_owner(get_editor_interface().get_edited_scene_root())
		#print("What is happening...")
