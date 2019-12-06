#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include <math.h>
#include <stdio.h>
#include <string.h>
#include "DetourNavMesh.h"
#include "tilecache_helpers.h"
#include "DetourTileCache.h"


namespace godot {

	class FileManager {
	public:
		static void saveNavigationMesh(const char* path, const dtNavMesh* mesh);
		static dtNavMesh* loadNavigationMesh(const char* path);
		static void saveNavigationMeshCached(const char* path, const dtTileCache* m_tileCache, const dtNavMesh* m_navMesh);
		static void loadNavigationMeshCached(const char* path, dtTileCache* m_tileCache, dtNavMesh* m_navMesh);
	};
}

#endif