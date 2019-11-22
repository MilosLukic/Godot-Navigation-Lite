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
			if (polyAreas[i] != RC_NULL_AREA)
				polyFlags[i] = RC_WALKABLE_AREA;
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
		nav->clear_debug_mesh();
		nav->get_debug_mesh();
	}
};

using namespace godot;

DetourNavigationMeshCached::DetourNavigationMeshCached() {
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
	for (Ref<ArrayMesh> m : input_meshes) {
		m.unref();
	}

	clear_debug_mesh();
	release_navmesh();
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

	dtTileCache* tile_cache = get_tile_cache();
	tile_cache->removeTile(nav->getTileRefAt(x, z, 0), NULL, NULL);

	rcConfig config;
	config.cs = cell_size;
	config.ch = cell_height;
	config.walkableSlopeAngle = agent_max_slope;
	config.walkableHeight = (int)ceil(agent_height / config.ch);
	config.walkableClimb = (int)floor(agent_max_climb / config.ch);
	config.walkableRadius = (int)ceil(agent_radius / config.cs);
	config.maxEdgeLen = (int)(edge_max_length / config.cs);
	config.maxSimplificationError = edge_max_error;
	config.minRegionArea = (int)sqrtf(region_min_size);
	config.mergeRegionArea = (int)sqrtf(region_merge_size);
	config.maxVertsPerPoly = 6;
	config.tileSize = tile_size;
	config.borderSize = config.walkableRadius + 3;
	config.width = config.tileSize + config.borderSize * 2;
	config.height = config.tileSize + config.borderSize * 2;
	config.detailSampleDist =
		detail_sample_distance < 0.9f ? 0.0f : cell_size * detail_sample_distance;
	config.detailSampleMaxError = cell_height * detail_sample_max_error;
	rcVcopy(config.bmin, &bmin.coord[0]);
	rcVcopy(config.bmax, &bmax.coord[0]);

	config.bmin[0] -= config.borderSize * config.cs;
	config.bmin[2] -= config.borderSize * config.cs;
	config.bmax[0] += config.borderSize * config.cs;
	config.bmax[2] += config.borderSize * config.cs;

	/* Set the tile AABB */
	AABB expbox(bmin, bmax - bmin);
	expbox.position.x -= config.borderSize * config.cs;
	expbox.position.z -= config.borderSize * config.cs;
	expbox.size.x += 2.0 * config.borderSize * config.cs;
	expbox.size.z += 2.0 * config.borderSize * config.cs;

	std::vector<float> points;
	std::vector<int> indices;

	Transform base = global_transform.inverse();

	for (int i = 0; i < input_meshes.size(); i++) {
		if (!input_meshes[i].is_valid()) {
			continue;
		}
		Transform mxform = base * input_transforms[i];
		AABB mesh_aabb = mxform.xform(input_aabbs[i]);
		if (!mesh_aabb.intersects_inclusive(expbox) &&
			!expbox.encloses(mesh_aabb)) {
			continue;
		}
		add_meshdata(i, points, indices);
	}

	if (points.size() == 0 || indices.size() == 0) {
		Godot::print("No mesh points and indices found...");
		return true;
	}

	// Create heightfield
	rcHeightfield* heightfield = rcAllocHeightfield();
	if (!heightfield) {
		ERR_PRINT("Failed to allocate height field");
		return false;
	}
	rcContext* ctx = new rcContext(true);
	if (!rcCreateHeightfield(
		ctx, *heightfield,
		config.width, config.height, config.bmin,
		config.bmax, config.cs, config.ch)
		) {
		ERR_PRINT("Failed to create height field");
		return false;
	}

	int ntris = indices.size() / 3;
	std::vector<unsigned char> tri_areas;
	tri_areas.resize(ntris);
	memset(&tri_areas[0], 0, ntris);

	rcMarkWalkableTriangles(
		ctx, config.walkableSlopeAngle, &points[0],
		points.size() / 3, &indices[0], ntris, &tri_areas[0]
	);

	rcRasterizeTriangles(
		ctx, &points[0], points.size() / 3, &indices[0],
		&tri_areas[0], ntris, *heightfield, config.walkableClimb
	);
	rcFilterLowHangingWalkableObstacles(
		ctx, config.walkableClimb, *heightfield
	);

	rcFilterLedgeSpans(
		ctx, config.walkableHeight, config.walkableClimb, *heightfield
	);
	rcFilterWalkableLowHeightSpans(
		ctx, config.walkableHeight, *heightfield
	);

	rcCompactHeightfield* compact_heightfield = rcAllocCompactHeightfield();
	if (!compact_heightfield) {
		ERR_PRINT("Failed to allocate compact height field.");
		return false;
	}
	if (!rcBuildCompactHeightfield(ctx, config.walkableHeight, config.walkableClimb,
		*heightfield, *compact_heightfield)) {
		ERR_PRINT("Could not build compact height field.");
		return false;
	}
	if (!rcErodeWalkableArea(ctx, config.walkableRadius, *compact_heightfield)) {
		ERR_PRINT("Could not erode walkable area.");
		return false;
	}
	if (partition_type == 0) { // TODO: Use enum
		if (!rcBuildDistanceField(ctx, *compact_heightfield))
			return false;
		if (!rcBuildRegions(
			ctx, *compact_heightfield, config.borderSize,
			config.minRegionArea, config.mergeRegionArea)
			) {
			return false;
		}
	}
	else if (!rcBuildRegionsMonotone(
		ctx, *compact_heightfield, config.borderSize,
		config.minRegionArea, config.mergeRegionArea)
		) {
		return false;
	}

	/* Tile cache */
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