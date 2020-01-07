#ifndef NAVIGATION_MESH_H
#define NAVIGATION_MESH_H

#include <list>
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

#include "helpers.h"
#include "navmesh_parameters.h"
#include "tilecache_helpers.h"
#include "DetourNavMesh.h"
#include "DetourNavMeshBuilder.h"
#include "DetourTileCache.h"
#include "navigation_query.h"
#include "Recast.h"
#include "filemanager.h"


namespace godot {
	struct NavMeshProcess;
	class DetourNavigationMesh : public Spatial {
		GODOT_CLASS(DetourNavigationMesh, Spatial);
	protected:
		std::vector<Ref<Mesh>> *input_meshes;
		std::vector<Transform> *input_transforms;
		std::vector<AABB> *input_aabbs;
	public:
		DetourNavigationMesh();
		~DetourNavigationMesh();

		void _init();
		void _exit_tree();
		void _ready();
		static void _register_methods();

		void build_navmesh();

		void _on_renamed(Variant v);
		char* get_cache_file_path();


		bool alloc();
		void release_navmesh();
		bool unsigned_int();

		dtNavMesh* load_mesh();
		void save_mesh();

		void build_debug_mesh();
		Dictionary find_path(Variant from, Variant to);
		void _notification(int p_what);
		Ref<Material> get_debug_navigation_material();
		virtual dtTileCache* get_tile_cache() { return nullptr; };
		MeshInstance* debug_mesh_instance = nullptr;

		DetourNavigationQuery *nav_query = nullptr;
		DetourNavigationQueryFilter *query_filter = nullptr;

		AABB bounding_box;
		dtNavMesh* detour_navmesh = nullptr;
		Ref<ArrayMesh> debug_mesh;
		Transform global_transform;
		Ref<NavmeshParameters> navmesh_parameters;
		std::string navmesh_name;

		void init_mesh_data(
			std::vector<Ref<Mesh>> *meshes, std::vector<Transform> *transforms,
			std::vector<AABB> *aabbs, Transform g_transform
		) {
			global_transform = g_transform;
			input_aabbs = aabbs;
			input_transforms = transforms;
			input_meshes = meshes;
		}

		dtNavMesh* get_detour_navmesh() {
			return detour_navmesh;
		}

		void init_navigation_mesh_values();

		void clear_debug_mesh();

		void clear_navmesh();

		Ref<ArrayMesh> get_debug_mesh();
	};

}
#endif