#ifndef TILECACHE_GENERATOR_H
#define TILECACHE_GENERATOR_H

#include "navmesh_generator.h"

namespace godot
{
struct NavMeshProcess : public dtTileCacheMeshProcess
{
	virtual void process(struct dtNavMeshCreateParams *params,
						 unsigned char *polyAreas, unsigned short *polyFlags);
};
class DetourNavigationMeshCacheGenerator : public DetourNavigationMeshGenerator
{
	dtTileCache *tile_cache = nullptr;
	dtTileCacheAlloc *tile_cache_alloc = nullptr;
	dtTileCacheCompressor *tile_cache_compressor = nullptr;
	friend struct NavMeshProcess;
	NavMeshProcess *mesh_process = nullptr;

public:
	DetourNavigationMeshCacheGenerator();
	~DetourNavigationMeshCacheGenerator();
	void build();
	Ref<CachedNavmeshParameters> navmesh_parameters = nullptr;

	std::vector<int> tile_queue;
	std::vector<Vector3> offmesh_vertices;
	std::vector<float> offmesh_radius;
	std::vector<unsigned short> offmesh_flags;
	std::vector<unsigned char> offmesh_areas;
	std::vector<unsigned char> offmesh_dir;

	unsigned int build_tiles(
		int x1, int z1, int x2, int z2);
	bool build_tile(int x, int z);

	/* Tile cache */
	void init_values();
	bool alloc_tile_cache();
	bool init_tile_cache(dtTileCacheParams *param);

	void recalculate_tiles();

	void set_navmesh_parameters(Ref<CachedNavmeshParameters> np)
	{
		if (navmesh_parameters.is_valid())
		{
			navmesh_parameters.unref();
		}
		navmesh_parameters = np;
		DetourNavigationMeshGenerator::navmesh_parameters = np;
	}

	void set_tile_cache(dtTileCache *tc)
	{
		tile_cache = tc;
	}

	void set_mesh_process(NavMeshProcess *nmp)
	{
		mesh_process = nmp;
	}

	dtTileCache *get_tile_cache()
	{
		return tile_cache;
	}

	NavMeshProcess *get_mesh_process()
	{
		return mesh_process;
	}

	dtTileCacheCompressor *get_tile_cache_compressor()
	{
		return tile_cache_compressor;
	}
};

} // namespace godot
#endif