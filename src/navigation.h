#ifndef NAVIGATION_H
#define NAVIGATION_H

#include <list>
#include <string>
#include <algorithm>
#include <iostream>
#include <Godot.hpp>
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


namespace godot {

	class DetourNavigationMeshInstance : public Spatial {
		GODOT_CLASS(DetourNavigationMeshInstance, Spatial)
		enum ParsedGeometryType {
			PARSED_GEOMETRY_MESH_INSTANCES = 0,
			PARSED_GEOMETRY_STATIC_COLLIDERS
		};
	private:
		/* geometry_source can be 0 = static bodies, 1 = meshes */
		uint32_t collision_mask;
		SETGET(parsed_geometry_type, int);

		void collect_mesh_instances(
			Array& geometries,
			std::vector<Ref<Mesh>>& meshes, 
			std::vector<Transform>& transforms, 
			std::vector<AABB>& aabbs
		);

		void convert_static_bodies(
			StaticBody* static_body, 
			std::vector<Ref<Mesh>>& meshes, 
			std::vector<Transform>& transforms, 
			std::vector<AABB>& aabbs
		);
		
	public:
		static void _register_methods();

		DetourNavigationMeshInstance();
		~DetourNavigationMeshInstance();

		void _init();
		void _ready();

		void create_navmesh();
		void create_cached_navmesh();
		void build_navmesh(DetourNavigationMesh *navigation);
		void _notification(int p_what);

	};

}
#endif