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
		static void deleteFile(const char* path);
		static void moveFile(const char* path, const char* new_path);
		static void createDirectory(const char* path);
		static void saveNavigationMeshCached(const char* path, const dtTileCache* m_tileCache, const dtNavMesh* m_navMesh);
		static bool loadNavigationMeshCached(const char* path, dtTileCache* m_tileCache, dtNavMesh* m_navMesh, dtTileCacheMeshProcess* m_tmproc);
	};
}

#endif