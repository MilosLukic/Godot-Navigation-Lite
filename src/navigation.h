#ifndef NAVIGATION_H
#define NAVIGATION_H

#include <list>
#include <string>
#include <algorithm>
#include <iostream>
#include <Godot.hpp>
#include <Spatial.hpp>
#include <Node.hpp>
#include <SceneTree.hpp>
#include <MeshInstance.hpp>
#include <SpatialMaterial.hpp>
#include "navigation_query.h"
#include "navigation_mesh.h"
#include "Recast.h"
#include "DetourNavMesh.h"
#include "RecastAlloc.h"
#include "RecastAssert.h"
#include "helpers.h"


namespace godot {

	class DetourNavigationMeshInstance : public Spatial {
		GODOT_CLASS(DetourNavigationMeshInstance, Spatial)

	private:
		float time_passed;

	public:
		DetourNavigationMesh *navmesh;
		static void _register_methods();

		DetourNavigationMeshInstance();
		~DetourNavigationMeshInstance();

		void _init(); // our initializer called by Godot

		void _ready();
		void build_mesh();
		void find_path();

		Ref<Material> get_debug_navigation_material();

		enum partition_t {
			PARTITION_WATERSHED,
			PARTITION_MONOTONE,
		};
	};

}
#endif