#include "navigation_mesh.h"
#include "navigation_query.h"
#include "navigation.h"

using namespace godot;

void DetourNavigationMesh::_register_methods()
{
	register_method("_ready", &DetourNavigationMesh::_ready);
	register_method("_notification", &DetourNavigationMesh::_notification);
	register_method("_exit_tree", &DetourNavigationMesh::_exit_tree);
	register_method("bake_navmesh", &DetourNavigationMesh::build_navmesh);
	register_method("save_navmesh", &DetourNavigationMesh::save_mesh);
	register_method("clear_navmesh", &DetourNavigationMesh::clear_navmesh);
	register_method("find_path", &DetourNavigationMesh::find_path);

	register_property<DetourNavigationMesh, int>("collision_mask", &DetourNavigationMesh::set_collision_mask, &DetourNavigationMesh::get_collision_mask, 1,
												 GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_DEFAULT, GODOT_PROPERTY_HINT_LAYERS_3D_PHYSICS);

	register_property<DetourNavigationMesh, Array>("input_meshes_storage", &DetourNavigationMesh::set_input_meshes_storage, &DetourNavigationMesh::get_input_meshes_storage, Array(),
												   GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_STORAGE, GODOT_PROPERTY_HINT_NONE);
	register_property<DetourNavigationMesh, Array>("input_transforms_storage", &DetourNavigationMesh::set_input_transforms_storage, &DetourNavigationMesh::get_input_transforms_storage, Array(),
												   GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_STORAGE, GODOT_PROPERTY_HINT_NONE);
	register_property<DetourNavigationMesh, Array>("input_aabbs_storage", &DetourNavigationMesh::set_input_aabbs_storage, &DetourNavigationMesh::get_input_aabbs_storage, Array(),
												   GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_STORAGE, GODOT_PROPERTY_HINT_NONE);
	register_property<DetourNavigationMesh, Array>("collision_ids_storage", &DetourNavigationMesh::set_collision_ids_storage, &DetourNavigationMesh::get_collision_ids_storage, Array(),
												   GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_STORAGE, GODOT_PROPERTY_HINT_NONE);
	register_property<DetourNavigationMesh, String>("uuid", &DetourNavigationMesh::set_uuid, &DetourNavigationMesh::get_uuid, "",
													GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_STORAGE, GODOT_PROPERTY_HINT_NONE);

	register_property<DetourNavigationMesh, PoolByteArray>("serialized_navmesh_data", &DetourNavigationMesh::set_serialized_navmesh_data, &DetourNavigationMesh::get_serialized_navmesh_data, PoolByteArray(),
														   GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_STORAGE, GODOT_PROPERTY_HINT_NONE);

	register_property<DetourNavigationMesh, Color>(
		"debug_mesh_color", &DetourNavigationMesh::set_debug_mesh_color, &DetourNavigationMesh::get_debug_mesh_color, Color(0.1f, 1.0f, 0.7f, 0.4f));

	register_property<DetourNavigationMesh, Ref<NavmeshParameters>>("parameters", &DetourNavigationMesh::navmesh_parameters, Ref<NavmeshParameters>(),
																	GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_DEFAULT, GODOT_PROPERTY_HINT_RESOURCE_TYPE, "Resource");
	register_property<DetourNavigationMesh, Ref<ArrayMesh>>("debug_mesh", &DetourNavigationMesh::debug_mesh, nullptr,
															GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_STORAGE, GODOT_PROPERTY_HINT_NONE);
}

void DetourNavigationMesh::_init()
{
	if (OS::get_singleton()->is_stdout_verbose())
	{
		Godot::print("Navigation mesh init function called.");
	}
	collision_mask = 1;
	debug_mesh_color = Color(0.1f, 1.0f, 0.7f, 0.4f);
}

void DetourNavigationMesh::_exit_tree()
{
}

void DetourNavigationMesh::_ready()
{
	if (OS::get_singleton()->is_stdout_verbose())
	{
		Godot::print("Navigation mesh ready function called.");
	}
}

DetourNavigationMesh::DetourNavigationMesh()
{
}

DetourNavigationMesh::~DetourNavigationMesh()
{
	_is_being_deleted = true;
	release_navmesh();

	if (navmesh_parameters.is_valid())
	{
		navmesh_parameters.unref();
	}
	if (generator != nullptr)
	{
		delete generator;
		generator = nullptr;
	}
}

void DetourNavigationMesh::set_collision_mask(int cm)
{
	collision_mask = cm;

	Node *parent = get_parent();
	DetourNavigation *navigation = Object::cast_to<DetourNavigation>(parent);

	if (navigation)
	{
		navigation->recalculate_masks();
	}
}

void DetourNavigationMesh::release_navmesh()
{
	if (detour_navmesh != nullptr)
	{
		dtFreeNavMesh(detour_navmesh);
		detour_navmesh = nullptr;
	}
	clear_debug_mesh();
	if (nav_query != nullptr)
	{
		delete nav_query;
		nav_query = nullptr;
	}
	if (query_filter != nullptr)
	{
		delete query_filter;
		query_filter = nullptr;
	}
}

void DetourNavigationMesh::clear_debug_mesh()
{
	if (debug_mesh.is_valid())
	{
		debug_mesh.unref();
	}

	if (debug_mesh_instance != nullptr)
	{

		if (!_is_being_deleted)
		{
			debug_mesh_instance->queue_free();
		}
		debug_mesh_instance = nullptr;
	}
}

void DetourNavigationMesh::build_navmesh()
{
	DetourNavigation *dtmi = Object::cast_to<DetourNavigation>(get_parent());
	if (dtmi == NULL)
	{
		return;
	}
	clear_navmesh();
	dtmi->build_navmesh(this);
	save_mesh();
}

void DetourNavigationMesh::clear_navmesh()
{
	if (generator != nullptr)
	{
		delete generator;
		generator = nullptr;
	}

	input_aabbs_storage.clear();
	input_transforms_storage.clear();
	input_meshes_storage.clear();
	collision_ids_storage.clear();
	serialized_navmesh_data.resize(0);
	release_navmesh();
}

Ref<ArrayMesh> DetourNavigationMesh::get_debug_mesh()
{
	if (debug_mesh.is_valid() || detour_navmesh == nullptr)
	{
		return debug_mesh;
	}
	std::list<Vector3> lines;
	const dtNavMesh *navm = detour_navmesh;
	for (int i = 0; i < navm->getMaxTiles(); i++)
	{
		const dtMeshTile *tile = navm->getTile(i);
		if (!tile || !tile->header)
		{
			continue;
		}
		for (int j = 0; j < tile->header->polyCount; j++)
		{
			dtPoly *poly = tile->polys + j;
			if (poly->getType() != DT_POLYTYPE_OFFMESH_CONNECTION)
			{
				for (int k = 0; k < poly->vertCount; k++)
				{
					lines.push_back(
						*reinterpret_cast<const Vector3 *>(
							&tile->verts[poly->verts[k] * 3]));
					lines.push_back(
						*reinterpret_cast<const Vector3 *>(
							&tile->verts[poly->verts[(k + 1) % poly->vertCount] * 3]));
				}
			}
			else if (poly->getType() == DT_POLYTYPE_OFFMESH_CONNECTION)
			{
				const dtOffMeshConnection *con =
					&tile->offMeshCons[j - tile->header->offMeshBase];
				const float *va = &tile->verts[poly->verts[0] * 3];
				const float *vb = &tile->verts[poly->verts[1] * 3];

				Vector3 p0 = *reinterpret_cast<const Vector3 *>(va);
				Vector3 p1 = *reinterpret_cast<const Vector3 *>(&con[0]);
				Vector3 p2 = *reinterpret_cast<const Vector3 *>(&con[3]);
				Vector3 p3 = *reinterpret_cast<const Vector3 *>(vb);
				lines.push_back(p0);
				lines.push_back(p1);
				lines.push_back(p1);
				lines.push_back(p2);
				lines.push_back(p2);
				lines.push_back(p3);
			}
		}
	}

	debug_mesh = Ref<ArrayMesh>(ArrayMesh::_new());
	PoolVector3Array varr;
	varr.resize(static_cast<int>(lines.size()));
	PoolVector3Array::Write w = varr.write();
	int idx = 0;
	for (Vector3 vec : lines)
	{
		w[idx++] = vec;
	}

	Array arr;
	arr.resize(Mesh::ARRAY_MAX);
	arr[Mesh::ARRAY_VERTEX] = varr;

	debug_mesh->add_surface_from_arrays(Mesh::PRIMITIVE_LINES, arr);
	return debug_mesh;
}

/**
 * Loads mesh from file and helper meshes from storage
 * Builds a debug mesh if debug navigation is checked or if 
 * its open in editor
 * 
 * @return true if success
 */
bool DetourNavigationMesh::load_mesh()
{
	dtNavMesh *dt_navmesh = Serializer::deserializeNavigationMesh(serialized_navmesh_data);
	if (dt_navmesh == 0)
	{
		ERR_PRINT("No baked navmesh found for " + get_name());
		return false;
	}
	else
	{
		detour_navmesh = dt_navmesh;
		init_generator(((Spatial *)get_parent())->get_global_transform());
		generator->detour_navmesh = dt_navmesh;

		if (!load_inputs())
		{
			detour_navmesh = nullptr;
			generator->detour_navmesh = nullptr;
			return false;
		}
		if (get_tree()->is_debugging_navigation_hint() || Engine::get_singleton()->is_editor_hint())
		{
			build_debug_mesh(false);
		}
		return true;
	}
}

/**
 * Saves the navigation mesh to a file (baking) 
 */
void DetourNavigationMesh::save_mesh()
{
	store_inputs();
	serialized_navmesh_data = Serializer::serializeNavigationMesh(get_detour_navmesh());
}

/**
 * Builds a debug mesh and adds it as a child to navigation mesh object
 */
void DetourNavigationMesh::build_debug_mesh(bool force_build)
{
	if (get_tree()->is_debugging_navigation_hint() || Engine::get_singleton()->is_editor_hint())
	{
		Ref<Material> material = nullptr;
		if (force_build && debug_mesh_instance != nullptr)
		{
			material = debug_mesh_instance->get_material_override();
			debug_mesh_instance->set_mesh(NULL);
			if (debug_mesh.is_valid())
			{
				debug_mesh.unref();
			}
			debug_mesh = get_debug_mesh();
		}
		else if (force_build || !debug_mesh.is_valid() || debug_mesh == nullptr)
		{
			clear_debug_mesh();
			material = get_debug_navigation_material();
			debug_mesh_instance = MeshInstance::_new();

			add_child(debug_mesh_instance);
			debug_mesh = get_debug_mesh();
		}
		else if (debug_mesh_instance == nullptr)
		{
			material = get_debug_navigation_material();
			debug_mesh_instance = MeshInstance::_new();
			add_child(debug_mesh_instance);
		}
		debug_mesh_instance->set_name("DebugMeshInstance");
		debug_mesh_instance->set_mesh(debug_mesh);
		debug_mesh_instance->set_material_override(material);
		debug_navmesh_dirty = false;
	}
}

/**
 * Create a navigation mesh material -
 * it is not exposed in godot, so we have to create it here again
 */
Ref<Material> DetourNavigationMesh::get_debug_navigation_material()
{
	Ref<SpatialMaterial> line_material = Ref<SpatialMaterial>(SpatialMaterial::_new());
	line_material->set_flag(SpatialMaterial::FLAG_UNSHADED, true);
	line_material->set_feature(SpatialMaterial::FEATURE_TRANSPARENT, true);
	line_material->set_flag(SpatialMaterial::FLAG_SRGB_VERTEX_COLOR, true);
	line_material->set_flag(SpatialMaterial::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);
	line_material->set_albedo(debug_mesh_color);

	return line_material;
}

/**
 * @returns array of the shortest path between two points
 */
Dictionary DetourNavigationMesh::find_path(Variant from, Variant to)
{
	if (!nav_query)
	{
		nav_query = new DetourNavigationQuery();
		nav_query->init(get_detour_navmesh(), get_global_transform());
		query_filter = new DetourNavigationQueryFilter();
	}

	//Dictionary result = nav_query->find_path(Vector3(0.f, 0.f, 0.f), Vector3(11.f, 0.3f, 11.f), Vector3(50.0f, 3.f, 50.f), new DetourNavigationQueryFilter());
	Dictionary result = nav_query->find_path((Vector3)from, (Vector3)to, Vector3(50.0f, 50.f, 50.f), query_filter);
	return result;
}

/**
 * Stores meshes and their transforms upon bake
 * so they can be loaded instead of calculated next time
 */
void DetourNavigationMesh::store_inputs()
{
	if (generator == nullptr)
	{
		return;
	}
	for (int i = 0; i < input_meshes_storage.size(); i++)
	{
		if (((Ref<Mesh>)input_meshes_storage[i]).is_valid())
		{
			((Ref<Mesh>)input_meshes_storage[i]).unref();
		}
	}

	input_meshes_storage.resize(static_cast<int>(generator->input_meshes->size()));
	input_transforms_storage.resize(static_cast<int>(generator->input_meshes->size()));
	input_aabbs_storage.resize(static_cast<int>(generator->input_meshes->size()));
	collision_ids_storage.resize(static_cast<int>(generator->input_meshes->size()));

	for (int i = 0; i < generator->input_meshes->size(); i++)
	{
		input_meshes_storage[i] = (Variant(generator->input_meshes->at(i)));
		input_transforms_storage[i] = (Variant(generator->input_transforms->at(i)));
		input_aabbs_storage[i] = (Variant(generator->input_aabbs->at(i)));
		collision_ids_storage[i] = (Variant(generator->collision_ids->at(i)));
	}
}

/**
 * Tries to load inputs if they are stored to save computing time
 * @returns success status
 */
bool DetourNavigationMesh::load_inputs()
{
	if (generator == nullptr || input_meshes_storage.size() == 0 || input_meshes_storage.size() != input_transforms_storage.size())
	{
		return false;
	}

	generator->input_meshes->resize(input_meshes_storage.size());
	generator->input_transforms->resize(input_meshes_storage.size());
	generator->input_aabbs->resize(input_meshes_storage.size());
	generator->collision_ids->resize(input_meshes_storage.size());

	for (int i = 0; i < input_meshes_storage.size(); i++)
	{
		generator->input_meshes->at(i) = input_meshes_storage[i];
		generator->input_transforms->at(i) = input_transforms_storage[i];
		generator->input_aabbs->at(i) = input_aabbs_storage[i];
		generator->collision_ids->at(i) = collision_ids_storage[i];
		bounding_box.merge_with(
			generator->input_transforms->at(i).xform(generator->input_aabbs->at(i)));
		generator->bounding_box = bounding_box;
	}

	generator->setup_generator();
	return true;
}

/**
 * Initializes generator which acts as an API
 * between Godot calls and detour/recast building logic
 */
DetourNavigationMeshGenerator *DetourNavigationMesh::init_generator(Transform global_transform)
{
	DetourNavigationMeshGenerator *dtnavmesh_gen =
		new DetourNavigationMeshGenerator();

	std::vector<Ref<Mesh>> *meshes = new std::vector<Ref<Mesh>>();
	std::vector<Transform> *transforms = new std::vector<Transform>();
	std::vector<AABB> *aabbs = new std::vector<AABB>();
	std::vector<int64_t> *collision_ids = new std::vector<int64_t>();

	dtnavmesh_gen->init_mesh_data(meshes, transforms, aabbs,
								  global_transform, collision_ids);

	dtnavmesh_gen->navmesh_parameters = navmesh_parameters;
	set_generator(dtnavmesh_gen);
	return dtnavmesh_gen;
}

void DetourNavigationMesh::_notification(int p_what)
{
}