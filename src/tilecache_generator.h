#ifndef TILECACHE_GENERATOR_H
#define TILECACHE_GENERATOR_H

#include "navmesh_generator.h"


namespace godot {
	class DetourNavigationMeshCacheGenerator : DetourNavigationMeshGenerator {
		dtTileCache* tile_cache;
		dtTileCacheAlloc* tile_cache_alloc;
		dtTileCacheCompressor* tile_cache_compressor;
		dtTileCacheMeshProcess* mesh_process;
		friend struct NavMeshProcess;
	public:
		DetourNavigationMeshCacheGenerator();
		~DetourNavigationMeshCacheGenerator();
		/* TILE CACHE */
		int max_obstacles;
		int max_layers;

		std::vector<int> tile_queue;
		std::vector<Vector3> offmesh_vertices;
		std::vector<float> offmesh_radius;
		std::vector<unsigned short> offmesh_flags;
		std::vector<unsigned char> offmesh_areas;
		std::vector<unsigned char> offmesh_dir;

		unsigned int build_tiles(
			int x1, int z1, int x2, int z2
		);
		bool build_tile(int x, int z);

		/* Tile cache */
		void init_values();
		bool alloc_tile_cache();
		bool init_tile_cache(dtTileCacheParams* param);


		dtTileCache* get_tile_cache() {
			return tile_cache;
		}
		dtTileCacheCompressor* get_tile_cache_compressor() {
			return tile_cache_compressor;
		}
	};

}
#endif