#ifndef SERIALIZER_H
#define SERIALIZER_H

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <Godot.hpp>
#include <PoolArrays.hpp>
#include "DetourNavMesh.h"
#include "tilecache_helpers.h"
#include "DetourTileCache.h"

namespace godot
{

class Serializer
{
public:
	static dtNavMesh *deserializeNavigationMesh(PoolByteArray byte_data);
	static PoolByteArray serializeNavigationMesh(const dtNavMesh *mesh);
	static PoolByteArray serializeNavigationMeshCached(const dtTileCache *m_tileCache, const dtNavMesh *m_navMesh);
	static bool deserializeNavigationMeshCached(PoolByteArray byte_data, dtTileCache *m_tileCache, dtNavMesh *m_navMesh, dtTileCacheMeshProcess *m_tmproc);
};
} // namespace godot

#endif