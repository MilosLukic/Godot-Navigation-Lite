#ifndef TILECACHE_NAVMESH_H
#define TILECACHE_NAVMESH_H

#include "navigation_mesh.h"
#include "tilecache_generator.h"



namespace godot {
	struct NavMeshProcess : public dtTileCacheMeshProcess
	{
		virtual void process(struct dtNavMeshCreateParams* params,
			unsigned char* polyAreas, unsigned short* polyFlags);
	};

	class DetourNavigationMeshCached: public DetourNavigationMesh {
		GODOT_CLASS(DetourNavigationMeshCached, Spatial);
		/* TILE CACHE */
		friend struct NavMeshProcess;
	protected:
		int dynamic_collision_mask;
	public:
		void set_dynamic_collision_mask(int mask);
		int get_dynamic_collision_mask() {
			return dynamic_collision_mask;
		}

		DetourNavigationMeshCacheGenerator* generator = nullptr;
		dtTileCache* tile_cache = nullptr;
		dtTileCacheAlloc* tile_cache_alloc = nullptr;
		dtTileCacheCompressor* tile_cache_compressor = nullptr;
		NavMeshProcess* mesh_process = nullptr;

		DetourNavigationMeshCached();
		~DetourNavigationMeshCached();

		Dictionary dynamic_obstacles;
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

		unsigned int add_box_obstacle(Vector3 pos, Vector3 extents, float rotationY);


		dtTileCache* get_tile_cache() {
			return tile_cache;
		}

		int max_obstacles = 0;
		int max_layers = 0;

		Ref<CachedNavmeshParameters> navmesh_parameters = nullptr;

		std::vector<int> tile_queue;
		std::vector<Vector3> offmesh_vertices;
		std::vector<float> offmesh_radius;
		std::vector<unsigned short> offmesh_flags;
		std::vector<unsigned char> offmesh_areas;
		std::vector<unsigned char> offmesh_dir;

		unsigned int add_cylinder_obstacle(Vector3 pos, float radius, float height);
		void remove_obstacle(int id);

	};

}
#endif