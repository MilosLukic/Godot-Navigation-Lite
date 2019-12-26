tool
extends EditorPlugin

var _navigation_menu_button = null
var _navmesh_menu_button = null

const CREATE_CACHED_NAVMESH = 0
const CREATE_NAVMESH = 1

const BAKE_NAVMESH = 0
const CLEAR_NAVMESH = 1

var currently_selected = null
onready var navigation_script = preload("res://bin/detour_navigation.gdns")
onready var navmesh_parameters = preload("res://bin/navmesh_parameters.gdns")

func _enter_tree():
	# When this plugin node enters tree, add the custom type
	add_custom_type(
		"DetourNavigation", 
		"Spatial", 
		load("res://addons/DetourNavigation/detour_navigation_bootstrap.gd"), 
		load("res://addons/DetourNavigation/icons/navigation_icon.svg"))
	
	add_custom_type(
		"DetourNavigationMesh", 
		"Spatial", 
		load("res://addons/DetourNavigation/detour_navigation_mesh_bootstrap.gd"), 
		load("res://addons/DetourNavigation/icons/navigation_mesh_instance_icon.svg"))

	var editor_interface = get_editor_interface()
	var base_control = editor_interface.get_base_control()
	
	# Add menu for navigation class
	_navigation_menu_button = MenuButton.new()
	_navigation_menu_button.text = "Navigation manager"
	_navigation_menu_button.get_popup().add_item("Create navmesh", CREATE_NAVMESH)
	_navigation_menu_button.get_popup().add_item("Create Cached Navmesh", CREATE_CACHED_NAVMESH)
	_navigation_menu_button.get_popup().connect("id_pressed", self, "_on_navigation_menu_id_pressed")
	add_control_to_container(CONTAINER_SPATIAL_EDITOR_MENU, _navigation_menu_button)
	_navigation_menu_button.hide()
	
	# Add menu for navigation mesh class
	_navmesh_menu_button = MenuButton.new()
	_navmesh_menu_button.text = "Navigation Mesh"
	_navmesh_menu_button.get_popup().add_item("Bake navmesh", BAKE_NAVMESH)
	_navmesh_menu_button.get_popup().add_item("Clear navmesh", CLEAR_NAVMESH)
	_navmesh_menu_button.get_popup().connect("id_pressed", self, "_on_navmesh_menu_id_pressed")
	add_control_to_container(CONTAINER_SPATIAL_EDITOR_MENU, _navmesh_menu_button)
	_navmesh_menu_button.hide()

func handles(object):
	if object is Node:
		_navigation_menu_button.hide()
		_navmesh_menu_button.hide()
		currently_selected = null
	
	if object.has_method("create_navmesh"):
		_navigation_menu_button.show()
		currently_selected = object
	elif object.has_method("bake_navmesh"):
		_navmesh_menu_button.show()
		currently_selected = object


func _exit_tree():
	# When the plugin node exits the tree, remove the custom type
	#remove_control_from_container(CONTAINER_SPATIAL_EDITOR_MENU, _navigation_menu_button)
	#remove_control_from_container(CONTAINER_SPATIAL_EDITOR_MENU, _navmesh_menu_button)

	_navmesh_menu_button.queue_free()
	_navmesh_menu_button = null
	
	_navigation_menu_button.queue_free()
	_navigation_menu_button = null

	remove_custom_type("DetourNavigation")
	remove_custom_type("DetourNavigationMesh")


func _on_navigation_menu_id_pressed(id):
	if id == CREATE_CACHED_NAVMESH:
		currently_selected.create_cached_navmesh(navmesh_parameters.new())
	elif id == CREATE_NAVMESH:
		currently_selected.create_navmesh(navmesh_parameters.new())

func _on_navmesh_menu_id_pressed(id):
	if id == BAKE_NAVMESH:
		currently_selected.bake_navmesh()
	elif id == CLEAR_NAVMESH:
		currently_selected.clear_navmesh()

