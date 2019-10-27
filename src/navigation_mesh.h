#ifndef NAVIGATION_MESH_H
#define NAVIGATION_MESH_H

#include <list>
#include <vector>
#include <string>
#include <iostream>
#include <Godot.hpp>
#include <MeshInstance.hpp>
#include <Mesh.hpp>
#include <ArrayMesh.hpp>
#include "helpers.h"
#include "DetourNavMesh.h"
#include "DetourNavMeshBuilder.h"
#include "Recast.h"


namespace godot {

	class DetourNavigationMesh{

	public:
		DetourNavigationMesh();
		~DetourNavigationMesh();

		bool alloc();
		bool init(dtNavMeshParams* params);
		void release_navmesh();
		bool unsigned_int();

		SETGET(initialized, bool)
		SETGET(num_tiles_x, int)
		SETGET(num_tiles_z, int)
		SETGET(tile_size, int);
		SETGET(cell_size, real_t);
		SETGET(cell_height, real_t);

		SETGET(partition_type, int);
		SETGET(agent_height, real_t);
		SETGET(agent_radius, real_t);
		SETGET(agent_max_climb, real_t);
		SETGET(agent_max_slope, real_t);
		SETGET(region_min_size, real_t);
		SETGET(region_merge_size, real_t);
		SETGET(edge_max_length, real_t);
		SETGET(edge_max_error, real_t);
		SETGET(detail_sample_distance, real_t);
		SETGET(detail_sample_max_error, real_t);

		SETGET(padding, Vector3)
		AABB bounding_box;
		dtNavMesh* detour_navmesh;
		Ref<ArrayMesh> debug_mesh;

		dtNavMesh* get_detour_navmesh() {
			return detour_navmesh;
		}

		void clear_debug_mesh() {
			debug_mesh.unref();
		}

		inline real_t get_tile_edge_length() const {
			return ((real_t)tile_size * cell_size);
		};

		void set_tile_number(int xSize, int zSize) {
			num_tiles_x = (xSize + tile_size - 1) / tile_size;
			num_tiles_z = (zSize + tile_size - 1) / tile_size;
		};

		unsigned int build_tiles(
			const Transform& xform, 
			const std::list<MeshInstance*>& mesh_instances,
			int x1, int y1, int x2, int y2
		);

		bool build_tile(
			const Transform& xform,
			const std::list<MeshInstance*>& mesh_instances,
			int x, int z
		);

		void DetourNavigationMesh::get_tile_bounding_box(
			int x, int z, Vector3& bmin, Vector3& bmax
		);

		void add_meshdata(
			MeshInstance* mesh_instance, std::vector<float>& p_verticies, std::vector<int>& p_indices
		);

		Ref<ArrayMesh> get_debug_mesh();

	};

}
#endif