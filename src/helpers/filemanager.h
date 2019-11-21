#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include <math.h>
#include <stdio.h>
#include <string.h>
#include "DetourNavMesh.h"


namespace godot {

	class FileManager {
	public:
		static void saveAll(const char* path, const dtNavMesh* mesh);
		static dtNavMesh* loadAll(const char* path);
	};
}

#endif