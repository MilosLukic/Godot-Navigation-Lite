#ifndef NAVIGATION_H
#define NAVIGATION_H

#include <list>
#include <string>
#include <algorithm>
#include <iostream>
#include <Godot.hpp>
#include <Engine.hpp>
#include <Geometry.hpp>
#include <Spatial.hpp>
#include <Node.hpp>
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


namespace godot {

	class DetourNavigation : public Spatial {
		GODOT_CLASS(DetourNavigation, Spatial)
		enum ParsedGeometryType {
			PARSED_GEOMETRY_STATIC_COLLIDERS = 0,
			PARSED_GEOMETRY_MESH_INSTANCES = 1
			
		};
	private:
		/* geometry_source can be 0 = static bodies, 1 = meshes */
		SETGET(collision_mask, int);
		SETGET(parsed_geometry_type, int);

		SETGET(cache_objects, bool);
		SETGET(cache_collision_mask, int);

		void collect_mesh_instances(
			Array& geometries,
			std::vector<Ref<Mesh>> *meshes, 
			std::vector<Transform> *transforms, 
			std::vector<AABB> *aabbs
		);


		void convert_static_bodies(
			StaticBody* static_body, 
			std::vector<Ref<Mesh>>* meshes, 
			std::vector<Transform>* transforms, 
			std::vector<AABB>* aabbs
		);
		
		struct DynamicObstacle {
			StaticBody* static_body;
			int obstacle_id;
		};
		std::map<int, int>;

	public:
		static void _register_methods();

		DetourNavigation();
		~DetourNavigation();

		void _exit_tree();

		void _enter_tree();

		void _init();
		void _ready();

		DetourNavigationMeshCached *create_cached_navmesh(Ref<CachedNavmeshParameters> np);

		DetourNavigationMesh *create_navmesh(Ref<NavmeshParameters> np);

		std::vector<DetourNavigationMeshCached *> cached_navmeshes;
		std::vector<StaticBody *> to_add_obstacles;
		std::vector<StaticBody *> to_remove_obstacles;

		void build_navmesh(DetourNavigationMesh *navigation);
		void build_navmesh_cached(DetourNavigationMeshCached* navmesh);
		void _notification(int p_what);

		void _on_node_added(Variant node);
		void _on_node_removed(Variant node);

	};

}
#endif