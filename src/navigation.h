#ifndef NAVIGATION_H
#define NAVIGATION_H

#include <list>
#include <string>
#include <algorithm>
#include <iostream>
#include <thread>
#include <Godot.hpp>
#include <Engine.hpp>
#include <Spatial.hpp>
#include <SceneTree.hpp>
#include <MeshInstance.hpp>
#include <BoxShape.hpp>
#include <CapsuleShape.hpp>
#include <CylinderShape.hpp>
#include <SphereShape.hpp>
#include <ConcavePolygonShape.hpp>
#include <ConvexPolygonShape.hpp>
#include <CubeMesh.hpp>
#include <CapsuleMesh.hpp>
#include <CylinderMesh.hpp>
#include <SphereMesh.hpp>
#include <StaticBody.hpp>
#include <CollisionShape.hpp>

#include "navigation_mesh.h"
#include "tilecache_navmesh.h"
#include "Recast.h"
#include "DetourNavMesh.h"
#include "RecastAlloc.h"
#include "RecastAssert.h"
#include "helpers.h"
#include "navmesh_generator.h"
#include "tilecache_generator.h"

namespace godot
{

class DetourNavigation : public Spatial
{
	GODOT_CLASS(DetourNavigation, Spatial)
	enum ParsedGeometryType
	{
		PARSED_GEOMETRY_STATIC_COLLIDERS = 0,
		PARSED_GEOMETRY_MESH_INSTANCES = 1

	};

private:
	/* geometry_source can be 0 = static bodies, 1 = meshes */
	float aggregated_time_passed = 0.f;

	SETGET(parsed_geometry_type, int);
	SETGET(dynamic_objects, bool);
	SETGET(dynamic_collision_mask, int);
	SETGET(collision_mask, int);
	SETGET(auto_object_management, bool)

	void collect_geometry(Array geometries,
								std::vector<Ref<Mesh>> *meshes, std::vector<Transform> *transforms,
								std::vector<AABB> *aabbs, std::vector<int64_t> *collision_ids,
								DetourNavigationMesh *navmesh);

	void convert_collision_shape(CollisionShape *collision_shape,
								 std::vector<Ref<Mesh>> *meshes, std::vector<Transform> *transforms,
								 std::vector<AABB> *aabbs, std::vector<int64_t> *collision_ids);

	std::vector<PhysicsBody *> *dyn_bodies_to_add = nullptr;
	std::vector<StaticBody *> *static_bodies_to_add = nullptr;
	std::vector<int64_t> *collisions_to_remove = nullptr;

public:
	static void _register_methods();

	DetourNavigation();
	~DetourNavigation();

	void _exit_tree();

	void _on_tree_exiting();

	void _enter_tree();

	void _init();
	void _ready();

	void recalculate_masks();
	void fill_pointer_arrays();
	void manage_changes();
	void update_tilecache();
	void rebuild_dirty_debug_meshes();

	void add_box_obstacle_to_all(int64_t instance_id, Vector3 position,
								 Vector3 extents, float rotationY, int collision_layer);

	void add_cylinder_obstacle_to_all(int64_t instance_id, Vector3 position,
									  float radius, float height, int collision_layer);

	void remove_obstacle(CollisionShape *collision_shape);

	void save_collision_shapes(DetourNavigationMeshGenerator *generator);

	void _process(float passed);

	DetourNavigationMeshCached *create_cached_navmesh(
		Ref<CachedNavmeshParameters> np);

	DetourNavigationMesh *create_navmesh(Ref<NavmeshParameters> np);

	std::vector<DetourNavigationMesh *> *navmeshes = nullptr;
	std::vector<DetourNavigationMeshCached *> *cached_navmeshes = nullptr;

	void build_navmesh(DetourNavigationMesh *navigation);
	void build_navmesh_cached(DetourNavigationMeshCached *navmesh);
	void _notification(int p_what);
	void _on_node_renamed(Variant v);

	int process_large_mesh(MeshInstance *mesh_instance, int64_t collision_id,
						   std::vector<Ref<Mesh>> *meshes, std::vector<Transform> *transforms,
						   std::vector<AABB> *aabbs, std::vector<int64_t> *collision_ids);

	void _on_cache_collision_shape_added(Variant node);
	void _on_cache_collision_shape_removed(Variant node);

	void _on_collision_shape_added(Variant node);
	void _on_collision_shape_removed(Variant node);
	void recognize_stored_collision_shapes();
	void collect_mappings(Dictionary &mappings, Array element);
	void map_collision_shapes(DetourNavigationMesh *nm, Dictionary &mappings);
};

} // namespace godot
#endif
