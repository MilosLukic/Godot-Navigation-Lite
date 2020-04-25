#include "tilecache_navmesh.h"
#include "navigation.h"

using namespace godot;

void NavMeshProcess::process(struct dtNavMeshCreateParams *params,
							 unsigned char *polyAreas, unsigned short *polyFlags)
{
	for (int i = 0; i < params->polyCount; i++)
	{
		if (polyAreas[i] != RC_NULL_AREA)
		{
			polyFlags[i] = RC_WALKABLE_AREA;
		}
	}
}

DetourNavigationMeshCached::DetourNavigationMeshCached()
{
}

DetourNavigationMeshCached::~DetourNavigationMeshCached()
{
	if (tile_cache != nullptr)
	{
		dtFreeTileCache(tile_cache);
	}

	if (tile_cache_compressor != nullptr)
	{
		delete tile_cache_compressor;
	}

	if (mesh_process != nullptr)
	{
		delete mesh_process;
	}

	if (generator != nullptr)
	{
		delete generator;
		generator = nullptr;
		DetourNavigationMesh::generator = nullptr;
	}

	if (navmesh_parameters.is_valid())
	{
		navmesh_parameters.unref();
	}
}

void DetourNavigationMeshCached::_register_methods()
{
	register_method("_ready", &DetourNavigationMeshCached::_ready);
	register_method("_notification", &DetourNavigationMeshCached::_notification);
	register_method("_exit_tree", &DetourNavigationMeshCached::_exit_tree);
	register_method("bake_navmesh", &DetourNavigationMeshCached::build_navmesh);
	register_method("clear_navmesh", &DetourNavigationMeshCached::clear_navmesh);
	register_method("find_path", &DetourNavigationMeshCached::find_path);
	register_method("add_box_obstacle", &DetourNavigationMeshCached::add_box_obstacle);
	register_method("add_cylinder_obstacle", &DetourNavigationMeshCached::add_cylinder_obstacle);
	register_method("remove_obstacle", &DetourNavigationMeshCached::remove_obstacle);
	register_method("update_obstacle", &DetourNavigationMeshCached::refresh_obstacle);

	register_property<DetourNavigationMeshCached, int>("collision_mask", &DetourNavigationMeshCached::set_collision_mask, &DetourNavigationMeshCached::get_collision_mask, 1,
													   GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_DEFAULT, GODOT_PROPERTY_HINT_LAYERS_3D_PHYSICS);

	register_property<DetourNavigationMeshCached, int>("dynamic_objects_collision_mask", &DetourNavigationMeshCached::set_dynamic_collision_mask, &DetourNavigationMeshCached::get_dynamic_collision_mask, 1,
													   GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_DEFAULT, GODOT_PROPERTY_HINT_LAYERS_3D_PHYSICS);

	register_property<DetourNavigationMeshCached, Array>("input_meshes_storage", &DetourNavigationMeshCached::set_input_meshes_storage, &DetourNavigationMeshCached::get_input_meshes_storage, Array(),
														 GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_STORAGE, GODOT_PROPERTY_HINT_NONE);
	register_property<DetourNavigationMeshCached, Array>("input_transforms_storage", &DetourNavigationMeshCached::set_input_transforms_storage, &DetourNavigationMeshCached::get_input_transforms_storage, Array(),
														 GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_STORAGE, GODOT_PROPERTY_HINT_NONE);
	register_property<DetourNavigationMeshCached, Array>("input_aabbs_storage", &DetourNavigationMeshCached::set_input_aabbs_storage, &DetourNavigationMeshCached::get_input_aabbs_storage, Array(),
														 GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_STORAGE, GODOT_PROPERTY_HINT_NONE);
	register_property<DetourNavigationMeshCached, Array>("collision_ids_storage", &DetourNavigationMeshCached::set_collision_ids_storage, &DetourNavigationMeshCached::get_collision_ids_storage, Array(),
														 GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_STORAGE, GODOT_PROPERTY_HINT_NONE);

	register_property<DetourNavigationMeshCached, PoolByteArray>("serialized_navmesh_data", &DetourNavigationMeshCached::set_serialized_navmesh_data, &DetourNavigationMeshCached::get_serialized_navmesh_data, PoolByteArray(),
																 GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_STORAGE, GODOT_PROPERTY_HINT_NONE);

	register_property<DetourNavigationMeshCached, Color>(
		"debug_mesh_color", &DetourNavigationMeshCached::set_debug_mesh_color, &DetourNavigationMeshCached::get_debug_mesh_color, Color(0.1f, 1.0f, 0.7f, 0.4f));

	register_property<DetourNavigationMeshCached, Ref<ArrayMesh>>("debug_mesh", &DetourNavigationMeshCached::debug_mesh, nullptr,
																  GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_STORAGE, GODOT_PROPERTY_HINT_NONE);
	register_property<DetourNavigationMeshCached, Ref<CachedNavmeshParameters>>("parameters", &DetourNavigationMeshCached::set_navmesh_parameters, &DetourNavigationMeshCached::get_navmesh_parameters, Ref<CachedNavmeshParameters>(),
																				GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_DEFAULT, GODOT_PROPERTY_HINT_RESOURCE_TYPE, "Resource");
}

void DetourNavigationMeshCached::_init()
{
	DetourNavigationMesh::_init();
	dynamic_collision_mask = 2;
}

void DetourNavigationMeshCached::_exit_tree()
{
}

void DetourNavigationMeshCached::_ready()
{
}

void DetourNavigationMeshCached::set_dynamic_collision_mask(int cm)
{
	dynamic_collision_mask = cm;
	Node *parent = get_parent();
	DetourNavigation *navigation = Object::cast_to<DetourNavigation>(parent);

	if (navigation)
	{
		navigation->recalculate_masks();
	}
}
void DetourNavigationMeshCached::build_navmesh()
{
	DetourNavigation *dtmi = Object::cast_to<DetourNavigation>(get_parent());
	if (dtmi == NULL)
	{
		return;
	}
	clear_navmesh();
	dtmi->build_navmesh_cached(this);
	save_mesh();
	store_inputs();
}

void DetourNavigationMeshCached::clear_navmesh()
{
	if (generator != nullptr)
	{
		delete generator;
		generator = nullptr;
		DetourNavigationMesh::generator = nullptr;
	}

	DetourNavigationMesh::clear_navmesh();
	release_navmesh();
}

void DetourNavigationMeshCached::release_navmesh()
{
	DetourNavigationMesh::release_navmesh();

	if (tile_cache != nullptr)
	{
		dtFreeTileCache(tile_cache);
		tile_cache = nullptr;
	}

	if (tile_cache_compressor != nullptr)
	{
		delete tile_cache_compressor;
		tile_cache_compressor = nullptr;
	}

	if (mesh_process != nullptr)
	{
		delete mesh_process;
		mesh_process = nullptr;
	}
}

/**
 * Loads mesh from file and helper meshes from storage
 * Builds a debug mesh if debug navigation is checked or if 
 * its open in editor
 * 
 * @return true if success
 */
bool DetourNavigationMeshCached::load_mesh()
{
	detour_navmesh = dtAllocNavMesh();
	tile_cache = dtAllocTileCache();
	mesh_process = new NavMeshProcess();

	init_generator(((Spatial *)get_parent())->get_global_transform());
	generator->detour_navmesh = detour_navmesh;
	generator->set_mesh_process(mesh_process);
	generator->set_tile_cache(tile_cache);
	bool success = Serializer::deserializeNavigationMeshCached(serialized_navmesh_data, tile_cache, detour_navmesh, mesh_process);

	if (!success)
	{
		dtFreeTileCache(tile_cache);
		dtFreeNavMesh(detour_navmesh);
		delete mesh_process;

		mesh_process = nullptr;
		tile_cache = nullptr;
		generator->set_tile_cache(nullptr);
		generator->detour_navmesh = nullptr;
		detour_navmesh = nullptr;
		ERR_PRINT("No baked navmesh found for " + get_name());
		return false;
	}
	else
	{
		if (!load_inputs())
		{
			dtFreeTileCache(tile_cache);
			dtFreeNavMesh(detour_navmesh);
			delete mesh_process;

			mesh_process = nullptr;
			tile_cache = nullptr;
			generator->set_tile_cache(nullptr);
			generator->detour_navmesh = nullptr;
			detour_navmesh = nullptr;
			return false;
		}
		if (get_tree()->is_debugging_navigation_hint() || Engine::get_singleton()->is_editor_hint())
		{
			build_debug_mesh(false);
		}
		return true;
	}
}

void DetourNavigationMeshCached::save_mesh()
{
	serialized_navmesh_data = Serializer::serializeNavigationMeshCached(tile_cache, get_detour_navmesh());
}

unsigned int DetourNavigationMeshCached::add_box_obstacle(
	Vector3 pos, Vector3 extents, float rotationY)
{
	dtObstacleRef ref = 0;
	if (detour_navmesh == nullptr)
	{
		return (unsigned int)ref;
	}
	if (dtStatusFailed(
			tile_cache->addBoxObstacle(&pos.coord[0], &extents.coord[0], rotationY, &ref)))
	{
		ERR_PRINT("Can't add obstacle");
		return 0;
	}
	return (unsigned int)ref;
}

unsigned int DetourNavigationMeshCached::add_cylinder_obstacle(
	Vector3 pos, float radius, float height)
{

	dtObstacleRef ref = 0;
	if (detour_navmesh == nullptr)
	{
		return (unsigned int)ref;
	}
	if (dtStatusFailed(
			tile_cache->addObstacle(&pos.coord[0], radius, height, &ref)))
	{
		ERR_PRINT("Can't add obstacle");
		return 0;
	}
	return (unsigned int)ref;
}

void DetourNavigationMeshCached::remove_obstacle(int id)
{
	/* TODO implement navmesh changes queue */
	if (dtStatusFailed(tile_cache->removeObstacle(id)))
		ERR_PRINT("failed to remove obstacle");
}

Dictionary DetourNavigationMeshCached::find_path(Variant from, Variant to)
{

	if (!nav_query)
	{
		nav_query = new DetourNavigationQuery();
		nav_query->init(get_detour_navmesh(), get_global_transform());
		query_filter = new DetourNavigationQueryFilter();
	}

	Dictionary result = nav_query->find_path((Vector3)from, (Vector3)to, Vector3(50.0f, 50.f, 50.f), query_filter);
	return result;
}

DetourNavigationMeshCacheGenerator *DetourNavigationMeshCached::init_generator(Transform global_transform)
{
	DetourNavigationMeshCacheGenerator *dtnavmesh_gen =
		new DetourNavigationMeshCacheGenerator();

	std::vector<Ref<Mesh>> *meshes = new std::vector<Ref<Mesh>>();
	std::vector<Transform> *transforms = new std::vector<Transform>();
	std::vector<AABB> *aabbs = new std::vector<AABB>();
	std::vector<int64_t> *cids = new std::vector<int64_t>();

	dtnavmesh_gen->init_mesh_data(meshes, transforms, aabbs,
								  global_transform, cids);

	dtnavmesh_gen->set_navmesh_parameters(navmesh_parameters);
	set_generator(dtnavmesh_gen);
	return dtnavmesh_gen;
}

/**
 * Rebuilds all the tiles that were marked as dirty
 */
void DetourNavigationMeshCached::recalculate_tiles()
{
	if (generator->dirty_tiles == nullptr)
	{
		return;
	}

	Ref<BoxShape> tile_box = BoxShape::_new();
	float tile_edge_length = navmesh_parameters->get_tile_edge_length();
	tile_box->set_extents(Vector3(tile_edge_length * 0.52f, bounding_box.get_size().y, tile_edge_length * 0.52f));

	Ref<PhysicsShapeQueryParameters> query = PhysicsShapeQueryParameters::_new();
	query->set_shape(tile_box);
	Transform t = Transform();
	Array all_results;

	query->set_collision_mask(get_dynamic_collision_mask());
	std::vector<int> to_rebuild_x;
	std::vector<int> to_rebuild_z;

	for (int i = 0; i < generator->get_num_tiles_x(); i++)
	{
		for (int j = 0; j < generator->get_num_tiles_z(); j++)
		{
			if (generator->dirty_tiles[i][j] == 1)
			{
				t.origin = Vector3(
					(i + 0.5) * tile_edge_length + bounding_box.position.x,
					0.5 * (bounding_box.position.y + bounding_box.size.y),
					(j + 0.5) * tile_edge_length + bounding_box.position.z);

				query->set_transform(t);

				Array results = get_world()->get_direct_space_state()->intersect_shape(query);
				for (int k = 0; k < results.size(); k++)
				{
					Dictionary result = results[k];
					PhysicsBody *physics_body = Object::cast_to<PhysicsBody>(result["collider"]);
					if (physics_body)
					{
						for (int l = 0; l < physics_body->get_child_count(); ++l)
						{
							CollisionShape *collision_shape = Object::cast_to<CollisionShape>(physics_body->get_child(l));
							if (collision_shape)
							{
								collision_shapes_to_refresh.push_back(collision_shape);
							}
						}
					}
				}

				generator->build_tile(i, j);
				to_rebuild_x.push_back(i);
				to_rebuild_z.push_back(j);
				generator->dirty_tiles[i][j] = 0;
			}
		}
	}
	tile_box.unref();
	query.unref();
	refresh_obstacles();
	collision_shapes_to_refresh.clear();

	for (int i = 0; i < to_rebuild_x.size(); i++)
	{
		generator->build_tile(to_rebuild_x[i], to_rebuild_z[i]);
	}

	do
	{
		update_tilecache();
	} while (!tilecache_up_to_date);
}

/**
 * Update tilecache and set if debug mesh needs to be updated
 */
void DetourNavigationMeshCached::update_tilecache()
{
	bool previous_value = tilecache_up_to_date;

	get_tile_cache()->update(0, get_detour_navmesh(), &tilecache_up_to_date);
	if (!tilecache_up_to_date || tilecache_up_to_date && previous_value != true)
	{
		debug_navmesh_dirty = true;
	}
}

/**
 * Finds affected obstacles and refreshes them
 */
void DetourNavigationMeshCached::refresh_obstacles()
{
	for (int i = 0; i < collision_shapes_to_refresh.size(); i++)
	{
		refresh_obstacle(collision_shapes_to_refresh[i]);
	}
}

void DetourNavigationMeshCached::refresh_obstacle(CollisionShape *collision_shape)
{
	if (!dynamic_obstacles.has(collision_shape->get_instance_id()))
	{
		return;
	}
	remove_obstacle(dynamic_obstacles[collision_shape->get_instance_id()]);

	Transform transform = collision_shape->get_global_transform();

	Ref<Shape> s = collision_shape->get_shape();
	int obst_id = 0;
	if (s->get_class() == "BoxShape")
	{
		Ref<BoxShape> box = Object::cast_to<BoxShape>(*s);
		obst_id = add_box_obstacle(
			transform.get_origin(),
			box->get_extents() * transform.get_basis().get_scale(),
			transform.basis.orthonormalized().get_euler().y);
		box.unref();
	}
	else if (s->get_class() == "CylinderShape")
	{
		Ref<CylinderShape> cylinder = Object::cast_to<CylinderShape>(*s);
		obst_id = add_cylinder_obstacle(
			transform.get_origin() - Vector3(0.f,
											 cylinder->get_height() * 0.5f, 0.f),
			cylinder->get_radius() * std::max(
										 transform.get_basis().get_scale().x,
										 transform.get_basis().get_scale().z),
			cylinder->get_height() * transform.get_basis().get_scale().y);
		cylinder.unref();
	}
	else
	{
		dynamic_obstacles.erase(collision_shape->get_instance_id());
	}
	s.unref();
	dynamic_obstacles[collision_shape->get_instance_id()] = obst_id;
}