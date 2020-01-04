#include "tilecache_navmesh.h"
#include "navigation.h"


static const int DEFAULT_MAX_OBSTACLES = 1024;
static const int DEFAULT_MAX_LAYERS = 16;

struct godot::NavMeshProcess : public dtTileCacheMeshProcess {
	godot::DetourNavigationMeshCached* nav;
	inline explicit NavMeshProcess(godot::DetourNavigationMeshCached* mesh) :
		nav(mesh) {}
	virtual void process(struct dtNavMeshCreateParams* params,
		unsigned char* polyAreas, unsigned short* polyFlags) {
		/* Add proper flags and offmesh connections here */
		for (int i = 0; i < params->polyCount; i++) {
			if (polyAreas[i] != RC_NULL_AREA) {
				polyFlags[i] = RC_WALKABLE_AREA;
			}
				
		}
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
		}
	}
};

using namespace godot;


DetourNavigationMeshCached::DetourNavigationMeshCached() {
	max_obstacles = DEFAULT_MAX_OBSTACLES;
	max_layers = DEFAULT_MAX_LAYERS;

	mesh_process = new NavMeshProcess(this);
}

DetourNavigationMeshCached::~DetourNavigationMeshCached() {
	Godot::print("hoooo");
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
	Godot::print("ready 1");
	get_tree()->connect("node_renamed", this, "_on_renamed");
	navmesh_name = get_name().utf8().get_data();
	Godot::print("ready");
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
	Godot::print("Saving cached navmesh...");
	FileManager::createDirectory(".navcache");
	Godot::print(get_cache_file_path());
	FileManager::saveNavigationMeshCached(get_cache_file_path(), tile_cache, get_detour_navmesh());
	Godot::print("Cached navmesh successfully saved.");
}


void DetourNavigationMeshCached::_on_renamed(Variant v) {
	char* previous_path = get_cache_file_path();
	navmesh_name = get_name().utf8().get_data();
	char* current_path = get_cache_file_path();
	Godot::print("renaming");
	FileManager::moveFile(previous_path, current_path);
}


unsigned int DetourNavigationMeshCached::add_obstacle(const Vector3& pos,
	real_t radius, real_t height) {
	/* Need to test how this works and why this needed at all */
	/* TODO implement navmesh changes queue */
	//	while (tile_cache->isObstacleQueueFull())
	//		tile_cache->update(1, navMesh_);
	dtObstacleRef ref = 0;

	if (dtStatusFailed(
		tile_cache->addObstacle(&pos.coord[0], radius, height, &ref))) {
		ERR_PRINT("can't add obstacle");
		return 0;
	}
	return (unsigned int)ref;
}
void DetourNavigationMeshCached::remove_obstacle(unsigned int id) {
	/* Need to test how this works and why this needed at all */
	/* TODO implement navmesh changes queue */
	//	while (tile_cache->isObstacleQueueFull())
	//		tile_cache->update(1, navMesh_);
	if (dtStatusFailed(tile_cache->removeObstacle(id)))
		ERR_PRINT("failed to remove obstacle");
}

