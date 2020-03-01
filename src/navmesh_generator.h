#ifndef NAVMESH_GENERATOR_H
#define NAVMESH_GENERATOR_H

#include <list>
#include <algorithm>
#include <vector>
#include <string>
#include <iostream>
#include <Godot.hpp>
#include <Spatial.hpp>
#include <Geometry.hpp>
#include <Node.hpp>
#include <Ref.hpp>
#include <SpatialMaterial.hpp>
#include <SceneTree.hpp>
#include <MeshInstance.hpp>
#include <Mesh.hpp>
#include <ArrayMesh.hpp>

#include "navmesh_parameters.h"
#include "helpers.h"
#include "tilecache_helpers.h"
#include "DetourNavMesh.h"
#include "DetourNavMeshBuilder.h"
#include "DetourTileCache.h"
#include "Recast.h"


namespace godot {
	class DetourNavigationMeshGenerator{
	public:
		DetourNavigationMeshGenerator();
		~DetourNavigationMeshGenerator();


		std::vector<int64_t>* collision_ids;
		std::vector<Ref<Mesh>>* input_meshes;
		std::vector<Transform>* input_transforms;
		std::vector<AABB>* input_aabbs;

		Ref<NavmeshParameters> navmesh_parameters;
		Transform global_transform;
		dtNavMesh* detour_navmesh;

		SETGET(initialized, bool);
		SETGET(num_tiles_x, int);
		SETGET(num_tiles_z, int);

		void init_mesh_data(
			std::vector<Ref<Mesh>>* meshes, std::vector<Transform>* transforms,
			std::vector<AABB>* aabbs, Transform g_transform, std::vector<int64_t>* c_ids
		) {
			global_transform = g_transform;
			input_aabbs = aabbs;
			input_transforms = transforms;
			input_meshes = meshes;
			collision_ids = c_ids;
		};

		void build();
		void joint_build();
		bool alloc();
		bool init(dtNavMeshParams* params);

		virtual dtTileCache* get_tile_cache() { return nullptr; }

		AABB bounding_box;

		dtNavMesh* get_detour_navmesh() {
			return detour_navmesh;
		}

		void set_tile_number(int xSize, int zSize) {

			num_tiles_x = (xSize + navmesh_parameters->get_tile_size() - 1) / navmesh_parameters->get_tile_size();
			num_tiles_z = (zSize + navmesh_parameters->get_tile_size() - 1) / navmesh_parameters->get_tile_size();
		};


		unsigned int build_tiles(int x1, int y1, int x2, int y2);

		void init_rc_config(rcConfig& config, Vector3& bmin, Vector3& bmax);

		bool build_tile(int x, int z);

		bool init_heightfield_context(
			rcConfig& config, rcCompactHeightfield* compact_heightfield,
			rcContext* ctx, std::vector<float>& points, std::vector<int>& indices
		);

		bool init_tile_data(
			rcConfig& config, Vector3& bmin, Vector3& bmax, std::vector<float>& points,
			std::vector<int>& indices
		);

		void get_tile_bounding_box(
			int x, int z, Vector3& bmin, Vector3& bmax
		);

		void remove_collision_shape(int64_t collision_id);

		void recalculate_tiles(AABB changes_bounding_box);

		void add_meshdata(
			int mesh_index, std::vector<float>& p_verticies, std::vector<int>& p_indices
		);

		Ref<ArrayMesh> get_debug_mesh();
		void release_navmesh();

	};

}
#endif