#ifndef TILECACHE_NAVMESH_H
#define TILECACHE_NAVMESH_H

#include <list>
#include <vector>
#include <string>
#include <iostream>
#include <Godot.hpp>
#include <MeshInstance.hpp>
#include <Mesh.hpp>
#include <ArrayMesh.hpp>
#include "navigation_mesh.h"
#include "helpers.h"
#include "tilecache_helpers.h"
#include "DetourNavMesh.h"
#include "DetourNavMeshBuilder.h"
#include "Recast.h"


namespace godot {
	struct NavMeshProcess;
	class DetourNavigationMeshCached: public DetourNavigationMesh {
		/* TILE CACHE */
		dtTileCache* tile_cache;
		dtTileCacheAlloc* tile_cache_alloc;
		dtTileCacheCompressor* tile_cache_compressor;
		dtTileCacheMeshProcess* mesh_process;
		friend struct NavMeshProcess;
	public:
		DetourNavigationMeshCached();
		~DetourNavigationMeshCached();

		/* TILE CACHE */
		int max_obstacles;
		int max_layers;

		std::vector<int> tile_queue;
		std::vector<Vector3> offmesh_vertices;
		std::vector<float> offmesh_radius;
		std::vector<unsigned short> offmesh_flags;
		std::vector<unsigned char> offmesh_areas;
		std::vector<unsigned char> offmesh_dir;

		unsigned int DetourNavigationMeshCached::build_tiles(
			int x1, int z1, int x2, int z2
		);
		bool DetourNavigationMeshCached::build_tile(int x, int z);

		/* Tile cache */
		bool alloc_tile_cache();
		bool init_tile_cache(dtTileCacheParams* param);
		unsigned int add_obstacle(const Vector3& pos, real_t radius, real_t height);
		void remove_obstacle(unsigned int id);

		dtTileCache* get_tile_cache() {
			return tile_cache;
		}
		dtTileCacheCompressor* get_tile_cache_compressor() {
			return tile_cache_compressor;
		}


	};

}
#endif