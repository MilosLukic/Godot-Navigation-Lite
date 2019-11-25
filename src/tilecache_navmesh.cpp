#include "tilecache_navmesh.h"

static const int DEFAULT_TILE_SIZE = 64;
static const float DEFAULT_CELL_SIZE = 0.3f;
static const float DEFAULT_CELL_HEIGHT = 0.2f;
static const float DEFAULT_AGENT_HEIGHT = 2.0f;
static const float DEFAULT_AGENT_RADIUS = 0.6f;
static const float DEFAULT_AGENT_MAX_CLIMB = 0.9f;
static const float DEFAULT_AGENT_MAX_SLOPE = 45.0f;
static const float DEFAULT_REGION_MIN_SIZE = 8.0f;
static const float DEFAULT_REGION_MERGE_SIZE = 20.0f;
static const float DEFAULT_EDGE_MAX_LENGTH = 12.0f;
static const float DEFAULT_EDGE_MAX_ERROR = 1.3f;
static const float DEFAULT_DETAIL_SAMPLE_DISTANCE = 6.0f;
static const float DEFAULT_DETAIL_SAMPLE_MAX_ERROR = 1.0f;
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
	init_values();
}

void DetourNavigationMeshCached::init_values() {
	bounding_box = godot::AABB();
	tile_size = DEFAULT_TILE_SIZE;
	cell_size = DEFAULT_CELL_SIZE;
	cell_height = DEFAULT_CELL_HEIGHT;

	agent_height = DEFAULT_AGENT_HEIGHT;
	agent_radius = DEFAULT_AGENT_RADIUS;
	agent_max_climb = DEFAULT_AGENT_MAX_CLIMB;
	agent_max_slope = DEFAULT_AGENT_MAX_SLOPE;

	region_min_size = DEFAULT_REGION_MIN_SIZE;
	region_merge_size = DEFAULT_REGION_MERGE_SIZE;
	edge_max_length = DEFAULT_EDGE_MAX_LENGTH;
	edge_max_error = DEFAULT_EDGE_MAX_ERROR;
	detail_sample_distance = DEFAULT_DETAIL_SAMPLE_DISTANCE;
	detail_sample_max_error = DEFAULT_DETAIL_SAMPLE_MAX_ERROR;
	padding = Vector3(1.f, 1.f, 1.f);

	/* TILE Cache */
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

unsigned int DetourNavigationMeshCached::build_tiles(
	int x1, int z1, int x2, int z2
) {
	unsigned ret = 0;
	for (int z = z1; z <= z2; z++) {
		for (int x = x1; x <= x2; x++) {
			if (build_tile(x, z)) {
				ret++;
			}
		}
	}

	get_tile_cache()->update(0, get_detour_navmesh());
	return ret;
}

bool DetourNavigationMeshCached::build_tile(int x, int z) {
	Vector3 bmin, bmax;
	get_tile_bounding_box(x, z, bmin, bmax);
	dtNavMesh* nav = get_detour_navmesh();

	/* Tile Cache */
	dtTileCache* tile_cache = get_tile_cache();
	tile_cache->removeTile(nav->getTileRefAt(x, z, 0), NULL, NULL);

	// nav->removeTile(nav->getTileRefAt(x, z, 0), NULL, NULL); // <- removed due to tilecache

	rcConfig config;
	init_rc_config(config, bmin, bmax);

	std::vector<float> points;
	std::vector<int> indices;
	if (init_tile_data(config, bmin, bmax, points, indices)) {
		return true;
	}

	rcContext* ctx = new rcContext(true);
	rcCompactHeightfield* compact_heightfield = rcAllocCompactHeightfield();

	if (!init_heightfield_context(config, compact_heightfield, ctx, points, indices)) {
		return false;
	}

	rcHeightfieldLayerSet* heightfield_layer_set = rcAllocHeightfieldLayerSet();
	if (!heightfield_layer_set) {
		ERR_PRINT("Could not allocate height field layer set");
		return false;
	}
	if (!rcBuildHeightfieldLayers(ctx, *compact_heightfield, config.borderSize,
		config.walkableHeight, *heightfield_layer_set)) {
		ERR_PRINT("Could not build heightfield layers");
		return false;
	}

	for (int i = 0; i < heightfield_layer_set->nlayers; i++) {
		dtTileCacheLayerHeader header;
		header.magic = DT_TILECACHE_MAGIC;
		header.version = DT_TILECACHE_VERSION;
		header.tx = x;
		header.ty = z;
		header.tlayer = i;
		rcHeightfieldLayer* layer = &heightfield_layer_set->layers[i];
		rcVcopy(header.bmin, layer->bmin);
		rcVcopy(header.bmax, layer->bmax);
		header.width = (unsigned char)layer->width;
		header.height = (unsigned char)layer->height;
		header.minx = (unsigned char)layer->minx;
		header.maxx = (unsigned char)layer->maxx;
		header.miny = (unsigned char)layer->miny;
		header.maxy = (unsigned char)layer->maxy;
		header.hmin = (unsigned short)layer->hmin;
		header.hmax = (unsigned short)layer->hmax;
		unsigned char* tile_data;
		int tile_data_size;

		if (dtStatusFailed(dtBuildTileCacheLayer(
			get_tile_cache_compressor(), &header, layer->heights, layer->areas,
			layer->cons, &tile_data, &tile_data_size))) {
			ERR_PRINT("Failed to build tile cache layers");
			return false;
		}
		dtCompressedTileRef tileRef;
		int status = tile_cache->addTile(tile_data, tile_data_size,
			DT_COMPRESSEDTILE_FREE_DATA, &tileRef);
		if (dtStatusFailed((dtStatus)status)) {
			dtFree(tile_data);
			tile_data = NULL;
		}
		tile_cache->buildNavMeshTilesAt(x, z, nav);
	}

	return true;
}


/* Tile Cache */
bool DetourNavigationMeshCached::alloc_tile_cache() {
	tile_cache = dtAllocTileCache();
	if (!tile_cache) {
		ERR_PRINT("Could not allocate tile cache");
		release_navmesh();
		return false;
	}
	return true;
}

bool DetourNavigationMeshCached::init_tile_cache(dtTileCacheParams* params) {
	if (dtStatusFailed(tile_cache->init(params, tile_cache_alloc,
		tile_cache_compressor, mesh_process))) {
		ERR_PRINT("Could not initialize tile cache");
		release_navmesh();
		return false;
	}
	return true;
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