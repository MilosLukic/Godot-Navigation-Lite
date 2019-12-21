#include "tilecache_navmesh.h"

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

	tile_cache = 0;
	tile_cache_alloc = new LinearAllocator(64000);
	tile_cache_compressor = new FastLZCompressor();
	mesh_process = new NavMeshProcess(this);
}

DetourNavigationMeshCached::~DetourNavigationMeshCached() {
	dtFreeTileCache(get_tile_cache());

	delete tile_cache_compressor;
	delete mesh_process;
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