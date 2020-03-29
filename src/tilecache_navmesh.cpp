#include "tilecache_navmesh.h"
#include "navigation.h"

static const int DEFAULT_MAX_OBSTACLES = 1024;
static const int DEFAULT_MAX_LAYERS = 16;

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
	max_obstacles = DEFAULT_MAX_OBSTACLES;
	max_layers = DEFAULT_MAX_LAYERS;
	dynamic_collision_mask = 2;
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
	register_method("_on_renamed", &DetourNavigationMeshCached::_on_renamed);

	register_property<DetourNavigationMeshCached, int>("collision_mask", &DetourNavigationMeshCached::set_collision_mask, &DetourNavigationMeshCached::get_collision_mask, 1,
													   GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_DEFAULT, GODOT_PROPERTY_HINT_LAYERS_3D_PHYSICS);

	register_property<DetourNavigationMeshCached, int>("dynamic_objects_collision_mask", &DetourNavigationMeshCached::set_dynamic_collision_mask, &DetourNavigationMeshCached::get_dynamic_collision_mask, 1,
													   GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_DEFAULT, GODOT_PROPERTY_HINT_LAYERS_3D_PHYSICS);

	register_property<DetourNavigationMeshCached, Array>("input_meshes_storage", &DetourNavigationMesh::set_input_meshes_storage, &DetourNavigationMesh::get_input_meshes_storage, Array(),
														 GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_STORAGE, GODOT_PROPERTY_HINT_NONE);
	register_property<DetourNavigationMeshCached, Array>("input_transforms_storage", &DetourNavigationMesh::set_input_transforms_storage, &DetourNavigationMesh::get_input_transforms_storage, Array(),
														 GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_STORAGE, GODOT_PROPERTY_HINT_NONE);
	register_property<DetourNavigationMeshCached, Array>("input_aabbs_storage", &DetourNavigationMesh::set_input_aabbs_storage, &DetourNavigationMesh::get_input_aabbs_storage, Array(),
														 GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_STORAGE, GODOT_PROPERTY_HINT_NONE);
	register_property<DetourNavigationMeshCached, Array>("collision_ids_storage", &DetourNavigationMesh::set_collision_ids_storage, &DetourNavigationMesh::get_collision_ids_storage, Array(),
														 GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_STORAGE, GODOT_PROPERTY_HINT_NONE);
	register_property<DetourNavigationMeshCached, String>("uuid", &DetourNavigationMeshCached::set_uuid, &DetourNavigationMeshCached::get_uuid, "",
													GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_STORAGE, GODOT_PROPERTY_HINT_NONE);

	register_property<DetourNavigationMeshCached, Color>(
		"debug_mesh_color", &DetourNavigationMeshCached::set_debug_mesh_color, &DetourNavigationMeshCached::get_debug_mesh_color, Color(0.1f, 1.0f, 0.7f, 0.4f));

	register_property<DetourNavigationMeshCached, Ref<CachedNavmeshParameters>>("parameters", &DetourNavigationMeshCached::set_navmesh_parameters, &DetourNavigationMeshCached::get_navmesh_parameters, Ref<CachedNavmeshParameters>(),
																				GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_DEFAULT, GODOT_PROPERTY_HINT_RESOURCE_TYPE, "Resource");
}

void DetourNavigationMeshCached::_init()
{
}

void DetourNavigationMeshCached::_exit_tree()
{
}

void DetourNavigationMeshCached::_ready()
{
	get_tree()->connect("node_renamed", this, "_on_renamed");
	navmesh_name = get_name().utf8().get_data();
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

	FileManager::loadNavigationMeshCached(get_cache_file_path(), tile_cache, detour_navmesh, mesh_process);
	if (detour_navmesh == 0)
	{
		return false;
	}
	else
	{
		if (!load_inputs())
		{
			return false;
		}

		if (get_tree()->is_debugging_navigation_hint() || Engine::get_singleton()->is_editor_hint())
		{
			build_debug_mesh();
		}
		return true;
	}
}

void DetourNavigationMeshCached::save_mesh()
{
	FileManager::createDirectory(".navcache");
	FileManager::saveNavigationMeshCached(get_cache_file_path(), tile_cache, get_detour_navmesh());
}

unsigned int DetourNavigationMeshCached::add_box_obstacle(
	Vector3 pos, Vector3 extents, float rotationY)
{
	dtObstacleRef ref = 0;
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

void DetourNavigationMeshCached::_on_renamed(Variant v)
{
	char *previous_path = get_cache_file_path();
	navmesh_name = get_name().utf8().get_data();
	char *current_path = get_cache_file_path();
	FileManager::moveFile(previous_path, current_path);
}