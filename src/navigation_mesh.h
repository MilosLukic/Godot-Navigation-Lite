#ifndef NAVIGATION_MESH_H
#define NAVIGATION_MESH_H

#include <list>
#include <vector>
#include <string>
#include <iostream>
#include <Godot.hpp>
#include <Spatial.hpp>
#include <Geometry.hpp>
#include <WeakRef.hpp>
#include <Node.hpp>
#include <Ref.hpp>
#include <SpatialMaterial.hpp>
#include <SceneTree.hpp>
#include <MeshInstance.hpp>
#include <Mesh.hpp>
#include <ArrayMesh.hpp>
#include <CollisionShape.hpp>

#include "helpers.h"
#include "navmesh_parameters.h"
#include "tilecache_helpers.h"
#include "navmesh_generator.h"
#include "DetourNavMesh.h"
#include "DetourNavMeshBuilder.h"
#include "DetourTileCache.h"
#include "navigation_query.h"
#include "Recast.h"
#include "serializer.h"

namespace godot
{
class DetourNavigationMesh : public Spatial
{
	GODOT_CLASS(DetourNavigationMesh, Spatial);

private:
	bool _is_being_deleted = false;

protected:
	int collision_mask;
	Color debug_mesh_color;

public:
	SETGET(input_meshes_storage, Array);
	SETGET(input_transforms_storage, Array);
	SETGET(input_aabbs_storage, Array);
	SETGET(collision_ids_storage, Array);
	SETGET(serialized_navmesh_data, PoolByteArray);
	SETGET(uuid, String);
	bool debug_navmesh_dirty = true;

	DetourNavigationMesh();
	~DetourNavigationMesh();

	void _init();
	void _exit_tree();
	void _ready();
	static void _register_methods();

	void build_navmesh();

	bool alloc();
	void release_navmesh();
	bool unsigned_int();

	bool load_mesh();
	void save_mesh();

	void store_inputs();
	bool load_inputs();

	void build_debug_mesh(bool force_build);
	Dictionary find_path(Variant from, Variant to);
	void _notification(int p_what);
	DetourNavigationMeshGenerator *init_generator(Transform global_transform);

	Ref<Material> get_debug_navigation_material();
	virtual dtTileCache *get_tile_cache() { return nullptr; };

	int get_collision_mask()
	{
		return collision_mask;
	}

	void set_collision_mask(int cm);

	MeshInstance *debug_mesh_instance = nullptr;

	DetourNavigationQuery *nav_query = nullptr;
	DetourNavigationQueryFilter *query_filter = nullptr;

	DetourNavigationMeshGenerator *generator = nullptr;

	AABB bounding_box;
	dtNavMesh *detour_navmesh = nullptr;
	Ref<ArrayMesh> debug_mesh = nullptr;
	Transform global_transform;
	Ref<NavmeshParameters> navmesh_parameters = nullptr;
	std::string navmesh_name = "default";

	void set_generator(DetourNavigationMeshGenerator *g)
	{
		generator = g;
	}

	DetourNavigationMeshGenerator *get_generator()
	{
		return generator;
	}

	dtNavMesh *get_detour_navmesh()
	{
		return detour_navmesh;
	}

	Color get_debug_mesh_color()
	{
		return debug_mesh_color;
	}

	void set_debug_mesh_color(Color dmc)
	{
		debug_mesh_color = dmc;
		if (debug_mesh_instance != nullptr && debug_mesh_instance->get_material_override() != nullptr)
		{
			((Ref<SpatialMaterial>)(debug_mesh_instance->get_material_override()))->set_albedo(debug_mesh_color);
		}
	}

	void init_navigation_mesh_values();

	void clear_debug_mesh();

	void clear_navmesh();

	Ref<ArrayMesh> get_debug_mesh();
};

} // namespace godot
#endif