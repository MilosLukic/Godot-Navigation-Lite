#include "tilecache_generator.h"
#include "tilecache_navmesh.h"


using namespace godot;

DetourNavigationMeshCacheGenerator::DetourNavigationMeshCacheGenerator() {
	tile_cache_alloc = new LinearAllocator(64000);
	tile_cache_compressor = new FastLZCompressor();
	mesh_process = new NavMeshProcess();
}

DetourNavigationMeshCacheGenerator::~DetourNavigationMeshCacheGenerator() {
	if (navmesh_parameters.is_valid()){
		navmesh_parameters.unref();
	}
}

void DetourNavigationMeshCacheGenerator::build() {
	DetourNavigationMeshGenerator::navmesh_parameters = navmesh_parameters;

	joint_build();
	dtTileCacheParams tile_cache_params;
	memset(&tile_cache_params, 0, sizeof(tile_cache_params));
	rcVcopy(tile_cache_params.orig, &bounding_box.position[0]);
	tile_cache_params.ch = navmesh_parameters->get_cell_height();
	tile_cache_params.cs = navmesh_parameters->get_cell_size();
	tile_cache_params.width = navmesh_parameters->get_tile_size();
	tile_cache_params.height = navmesh_parameters->get_tile_size();
	tile_cache_params.maxSimplificationError = navmesh_parameters->get_edge_max_error();
	tile_cache_params.maxTiles = get_num_tiles_x() * get_num_tiles_z() * navmesh_parameters->get_max_layers();
	tile_cache_params.maxObstacles = navmesh_parameters->get_max_obstacles();
	tile_cache_params.walkableClimb = navmesh_parameters->get_agent_max_climb();
	tile_cache_params.walkableHeight = navmesh_parameters->get_agent_height();
	tile_cache_params.walkableRadius = navmesh_parameters->get_agent_radius();

	if (!alloc_tile_cache())
		return;
	if (!init_tile_cache(&tile_cache_params))
		return;
	unsigned int result = build_tiles(
		0, 0, get_num_tiles_x() - 1, get_num_tiles_z() - 1
	);
}

unsigned int DetourNavigationMeshCacheGenerator::build_tiles(
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

bool DetourNavigationMeshCacheGenerator::build_tile(int x, int z) {
	Vector3 bmin, bmax;
	get_tile_bounding_box(x, z, bmin, bmax);
	dtNavMesh* nav = get_detour_navmesh();

	/* Tile Cache */
	dtTileCache* tile_cache = get_tile_cache();

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
		tile_cache->removeTile(tile_cache->getTileRef(tile_cache->getTileAt(x, z, i)), NULL, NULL);

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

		if (tile_data_size <= 0){
			return false;
		}

		dtCompressedTileRef tileRef;
		int status = tile_cache->addTile(tile_data, tile_data_size,
			DT_COMPRESSEDTILE_FREE_DATA, &tileRef);

		if (dtStatusFailed((dtStatus)status)) {
			dtFree(tile_data);
			tile_data = NULL;
			ERR_PRINT("Failed to add tile cache tile.");
			return false;
		}

		int st = tile_cache->buildNavMeshTilesAt(x, z, nav);
		if (dtStatusFailed(st)){
			ERR_PRINT("Failed building navmesh tile.");
		}
	}
	return true;
}


/* Tile Cache */
bool DetourNavigationMeshCacheGenerator::alloc_tile_cache() {
	tile_cache = dtAllocTileCache();
	if (!tile_cache) {
		ERR_PRINT("Could not allocate tile cache");
		release_navmesh();
		return false;
	}
	return true;
}

bool DetourNavigationMeshCacheGenerator::init_tile_cache(dtTileCacheParams* params) {
	if (dtStatusFailed(tile_cache->init(params, tile_cache_alloc,
		tile_cache_compressor, mesh_process))) {
		ERR_PRINT("Could not initialize tile cache");
		release_navmesh();
		return false;
	}
	return true;
}

/**
 * Rebuilds all the tiles that were marked as dirty
 */
void DetourNavigationMeshCacheGenerator::recalculate_tiles() {
	if (dirty_tiles == nullptr) {
		return;
	}
	for (int i = 0; i < get_num_tiles_x(); i++) {
		for (int j = 0; j < get_num_tiles_z(); j++) {
			if (dirty_tiles[i][j] == 1) {
				build_tile(i, j);
				dirty_tiles[i][j] = 0;
			}
		}
	}

	// get_tile_cache()->update(0, get_detour_navmesh());
	Godot::print("Done rebuilding cached.");
}