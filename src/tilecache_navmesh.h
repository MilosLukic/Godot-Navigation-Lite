#ifndef TILECACHE_NAVMESH_H
#define TILECACHE_NAVMESH_H

#include "navigation_mesh.h"
#include "tilecache_generator.h"



namespace godot {
	class DetourNavigationMeshCached: public DetourNavigationMesh {
		GODOT_CLASS(DetourNavigationMeshCached, Spatial);
		/* TILE CACHE */
		friend struct NavMeshProcess;
	public:
		dtTileCache* tile_cache = nullptr;
		dtTileCacheAlloc* tile_cache_alloc = nullptr;
		dtTileCacheCompressor* tile_cache_compressor = nullptr;
		NavMeshProcess* mesh_process = nullptr;

		DetourNavigationMeshCached();
		~DetourNavigationMeshCached();
		
		void _init();
		void _exit_tree();
		void _ready();
		static void _register_methods();
		void build_navmesh();

		void clear_navmesh();
		Dictionary find_path(Variant from, Variant to);

		dtNavMesh* load_mesh();

		void save_mesh();

		void _on_renamed(Variant v);


		dtTileCache* get_tile_cache() {
			return tile_cache;
		}

		int max_obstacles;
		int max_layers;

		Ref<CachedNavmeshParameters> navmesh_parameters;

		std::vector<int> tile_queue;
		std::vector<Vector3> offmesh_vertices;
		std::vector<float> offmesh_radius;
		std::vector<unsigned short> offmesh_flags;
		std::vector<unsigned char> offmesh_areas;
		std::vector<unsigned char> offmesh_dir;

		unsigned int add_obstacle(Vector3 pos, float radius, float height);
		void remove_obstacle(int id);



	};

}
#endif