#include "tilecache_navmesh.h"
#include "navigation.h"


static const int DEFAULT_MAX_OBSTACLES = 1024;
static const int DEFAULT_MAX_LAYERS = 16;




using namespace godot;

void NavMeshProcess::process(struct dtNavMeshCreateParams* params,
	unsigned char* polyAreas, unsigned short* polyFlags) {
	for (int i = 0; i < params->polyCount; i++) {
		if (polyAreas[i] != RC_NULL_AREA) {
			polyFlags[i] = RC_WALKABLE_AREA;
		}

	}
	/*
	params->offMeshConCount = nav->offmesh_radius.size();
	if (params->offMeshConCount > 0) {
		params->offMeshConVerts =
			reinterpret_cast<const float*>(&nav->offmesh_vertices[0]);
		params->offMeshConRad = &nav->offmesh_radius[0];
		params->offMeshConFlags = &nav->offmesh_flags[0];
		params->offMeshConAreas = &nav->offmesh_areas[0];
		params->offMeshConDir = &nav->offmesh_dir[0];
	}
	else {
		params->offMeshConVerts = NULL;
		params->offMeshConRad = NULL;
		params->offMeshConFlags = NULL;
		params->offMeshConAreas = NULL;
		params->offMeshConDir = NULL;
	}*/
}


DetourNavigationMeshCached::DetourNavigationMeshCached() {
	max_obstacles = DEFAULT_MAX_OBSTACLES;
	max_layers = DEFAULT_MAX_LAYERS;

}

DetourNavigationMeshCached::~DetourNavigationMeshCached() {
	if (tile_cache != nullptr) {
		dtFreeTileCache(tile_cache);
	}

	if (tile_cache_compressor != nullptr) {
		delete tile_cache_compressor;
	}

	if (mesh_process != nullptr) {
		delete mesh_process;
	}

}


void DetourNavigationMeshCached::_register_methods() {
	register_method("_ready", &DetourNavigationMeshCached::_ready);
	register_method("_notification", &DetourNavigationMeshCached::_notification);
	register_method("_exit_tree", &DetourNavigationMeshCached::_exit_tree);
	register_method("bake_navmesh", &DetourNavigationMeshCached::build_navmesh);
	register_method("_on_renamed", &DetourNavigationMeshCached::_on_renamed);
	register_method("clear_navmesh", &DetourNavigationMeshCached::clear_navmesh);
	register_method("find_path", &DetourNavigationMeshCached::find_path);
	register_method("add_box_obstacle", &DetourNavigationMeshCached::add_box_obstacle);
	register_method("add_cylynder_obstacle", &DetourNavigationMeshCached::add_cylynder_obstacle);
	register_method("remove_obstacle", &DetourNavigationMeshCached::remove_obstacle);
	register_property<DetourNavigationMeshCached, Ref<CachedNavmeshParameters>>("parameters", &DetourNavigationMeshCached::navmesh_parameters, Ref<CachedNavmeshParameters>(),
		GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_DEFAULT, GODOT_PROPERTY_HINT_RESOURCE_TYPE, "Resource");
}

void DetourNavigationMeshCached::_init() {
	// initialize any variables here
	DetourNavigationMesh::navmesh_parameters = navmesh_parameters;
}

void DetourNavigationMeshCached::_exit_tree() {
}


void DetourNavigationMeshCached::_ready() {
	get_tree()->connect("node_renamed", this, "_on_renamed");
	navmesh_name = get_name().utf8().get_data();

}

void DetourNavigationMeshCached::build_navmesh() {
	DetourNavigation* dtmi = Object::cast_to<DetourNavigation>(get_parent());
	if (dtmi == NULL) {
		return;
	}
	clear_navmesh();

	dtmi->build_navmesh_cached(this);
	save_mesh();
}


void DetourNavigationMeshCached::clear_navmesh() {
	release_navmesh();
}

dtNavMesh* DetourNavigationMeshCached::load_mesh() {
	detour_navmesh = dtAllocNavMesh();
	tile_cache = dtAllocTileCache();
	mesh_process = new NavMeshProcess();

	FileManager::loadNavigationMeshCached(get_cache_file_path(), tile_cache, detour_navmesh, mesh_process);
	if (detour_navmesh == 0) {
		return 0;
	}
	else {
		if (get_tree()->is_debugging_navigation_hint() || Engine::get_singleton()->is_editor_hint()) {
			build_debug_mesh();
		}
		return detour_navmesh;
	}
}

void DetourNavigationMeshCached::save_mesh() {
	FileManager::createDirectory(".navcache");
	FileManager::saveNavigationMeshCached(get_cache_file_path(), tile_cache, get_detour_navmesh());
}


void DetourNavigationMeshCached::_on_renamed(Variant v) {
	char* previous_path = get_cache_file_path();
	navmesh_name = get_name().utf8().get_data();
	char* current_path = get_cache_file_path();
	FileManager::moveFile(previous_path, current_path);
}


unsigned int DetourNavigationMeshCached::add_box_obstacle(
	Vector3 pos, Vector3 extents, float rotationY
){
	dtObstacleRef ref = 0;
	extents = extents;
	if (dtStatusFailed(
		tile_cache->addBoxObstacle(&pos.coord[0], &extents.coord[0], rotationY, &ref))) {
		ERR_PRINT("Can't add obstacle");
		return 0;
	}
	return (unsigned int) ref;
}

unsigned int DetourNavigationMeshCached::add_cylynder_obstacle(
	Vector3 pos, float radius, float height 
) {
	dtObstacleRef ref = 0;
	if (dtStatusFailed(
		tile_cache->addObstacle(&pos.coord[0], radius, height, &ref))) {
		ERR_PRINT("Can't add obstacle");
		return 0;
	}
	return (unsigned int) ref;
}

void DetourNavigationMeshCached::remove_obstacle(int id) {
	/* Need to test how this works and why this needed at all */
	/* TODO implement navmesh changes queue */
	//	while (tile_cache->isObstacleQueueFull())
	//		tile_cache->update(1, navMesh_);
	if (dtStatusFailed(tile_cache->removeObstacle(id)))
		ERR_PRINT("failed to remove obstacle");
}

Dictionary DetourNavigationMeshCached::find_path(Variant from, Variant to) {

	if (!nav_query) {
		nav_query = new DetourNavigationQuery();
		nav_query->init(get_detour_navmesh(), get_global_transform());
		query_filter = new DetourNavigationQueryFilter();
	}

	//Dictionary result = nav_query->find_path(Vector3(0.f, 0.f, 0.f), Vector3(11.f, 0.3f, 11.f), Vector3(50.0f, 3.f, 50.f), new DetourNavigationQueryFilter());
	Dictionary result = nav_query->find_path((Vector3) from, (Vector3) to, Vector3(50.0f, 50.f, 50.f), query_filter);
	return result;
}