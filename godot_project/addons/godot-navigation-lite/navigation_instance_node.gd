tool
extends EditorPlugin


var _navigation_menu_button = null
var _navmesh_menu_button = null

const CREATE_CACHED_NAVMESH = 0
const CREATE_NAVMESH = 1

const BAKE_NAVMESH = 0
const CLEAR_NAVMESH = 1

var currently_selected = null
var navigation_script = null
var navmesh_parameters = null
var cached_navmesh_parameters = null
var cached_navmesh = null
var navmesh = null



func _enter_tree():
	navmesh_parameters = preload("res://addons/godot-navigation-lite/bin/navmesh_parameters.gdns")
	cached_navmesh_parameters = preload("res://addons/godot-navigation-lite/bin/cached_navmesh_parameters.gdns")
	navigation_script = preload("res://addons/godot-navigation-lite/bin/detour_navigation.gdns")

	add_custom_type(
		"DetourNavigation", 
		"Spatial", 
		preload("res://addons/godot-navigation-lite/detour_navigation_bootstrap.gd"), 
		get_editor_interface().get_base_control().get_icon("Navigation", "EditorIcons")
	)
	navmesh = preload("res://addons/godot-navigation-lite/bin/detour_navigation_mesh_cached.gdns")

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
	_navmesh_menu_button.queue_free()
	_navmesh_menu_button = null
	
	_navigation_menu_button.queue_free()
	_navigation_menu_button = null
	
	remove_custom_type("DetourNavigation")


func _on_navigation_menu_id_pressed(id):
	if id == CREATE_CACHED_NAVMESH:
		currently_selected.create_cached_navmesh(cached_navmesh_parameters.new())
	elif id == CREATE_NAVMESH:
		currently_selected.create_navmesh(navmesh_parameters.new())

func _on_navmesh_menu_id_pressed(id):
	if id == BAKE_NAVMESH:
		currently_selected.bake_navmesh()
	elif id == CLEAR_NAVMESH:
		currently_selected.clear_navmesh()

