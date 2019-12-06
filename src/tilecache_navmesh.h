#ifndef TILECACHE_NAVMESH_H
#define TILECACHE_NAVMESH_H

#include "navigation_mesh.h"


namespace godot {
	struct NavMeshProcess;
	class DetourNavigationMeshCached: public DetourNavigationMesh {
		GODOT_CLASS(DetourNavigationMeshCached, Spatial);
		/* TILE CACHE */
		dtTileCache* tile_cache;
		dtTileCacheAlloc* tile_cache_alloc;
		dtTileCacheCompressor* tile_cache_compressor;
		dtTileCacheMeshProcess* mesh_process;
		friend struct NavMeshProcess;
	public:
		DetourNavigationMeshCached();
		~DetourNavigationMeshCached();

		int max_obstacles;
		int max_layers;

		std::vector<int> tile_queue;
		std::vector<Vector3> offmesh_vertices;
		std::vector<float> offmesh_radius;
		std::vector<unsigned short> offmesh_flags;
		std::vector<unsigned char> offmesh_areas;
		std::vector<unsigned char> offmesh_dir;

		unsigned int add_obstacle(const Vector3& pos, real_t radius, real_t height);
		void remove_obstacle(unsigned int id);



	};

}
#endif