#include "navigation.h"
#define QUICKHULL_IMPLEMENTATION
#include "quickhull.h"

using namespace godot;

void DetourNavigationMeshInstance::_register_methods() {
	register_method("_ready", &DetourNavigationMeshInstance::_ready);
	register_method("create_cached_navmesh", &DetourNavigationMeshInstance::create_cached_navmesh);
	register_method("create_navmesh", &DetourNavigationMeshInstance::create_navmesh);

	register_property<DetourNavigationMeshInstance, int>(
		"parsed_geometry_type", &DetourNavigationMeshInstance::set_parsed_geometry_type, &DetourNavigationMeshInstance::get_parsed_geometry_type, (int) PARSED_GEOMETRY_STATIC_COLLIDERS,
		GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_DEFAULT, GODOT_PROPERTY_HINT_ENUM, "Mesh instances,Static bodies"
	);
}

DetourNavigationMeshInstance::DetourNavigationMeshInstance(){
}

DetourNavigationMeshInstance::~DetourNavigationMeshInstance(){
}

void DetourNavigationMeshInstance::_init() {
	// initialize any variables here
	set_parsed_geometry_type(PARSED_GEOMETRY_STATIC_COLLIDERS);
}

void DetourNavigationMeshInstance::_ready() {
	parsed_geometry_type = PARSED_GEOMETRY_MESH_INSTANCES;
	collision_mask |= 1;

	for (int i = 0; i < get_child_count(); ++i) {
		DetourNavigationMesh* navmesh_p = Object::cast_to<DetourNavigationMesh>(get_child(i));
		if (navmesh_p != NULL) {
			build_navmesh(navmesh_p);
			continue;
		}
		DetourNavigationMeshCached* navmesh_pc = Object::cast_to<DetourNavigationMeshCached>(get_child(i));
		if (navmesh_pc != NULL) {
			build_navmesh(navmesh_pc);
			continue;
		}
	}
}

void DetourNavigationMeshInstance::convert_static_bodies(
	StaticBody *static_body, std::vector<Ref<Mesh>>* meshes, std::vector<Transform>* transforms, std::vector<AABB>* aabbs
) {
	for (int i = 0; i < static_body->get_child_count(); ++i) {
		CollisionShape* collision_shape = Object::cast_to<CollisionShape>(static_body->get_child(i));
		if (collision_shape == NULL) {
			continue;
		}

		Transform transform = collision_shape->get_global_transform();

		Ref<Mesh> mesh;
		Ref<Shape> s = collision_shape->get_shape();

		BoxShape* box = Object::cast_to<BoxShape>(*s);
		if (box) {
			Ref<CubeMesh> cube_mesh;
			cube_mesh.instance();
			cube_mesh->set_size(box->get_extents() * 2.0);
			mesh = cube_mesh;
		}

		CapsuleShape* capsule = Object::cast_to<CapsuleShape>(*s);
		if (capsule) {
			Ref<CapsuleMesh> capsule_mesh;
			capsule_mesh.instance();
			capsule_mesh->set_radius(capsule->get_radius());
			capsule_mesh->set_mid_height(capsule->get_height() / 2.0);
			mesh = capsule_mesh;
		}

		CylinderShape* cylinder = Object::cast_to<CylinderShape>(*s);
		if (cylinder) {
			Ref<CylinderMesh> cylinder_mesh;
			cylinder_mesh.instance();
			cylinder_mesh->set_height(cylinder->get_height());
			cylinder_mesh->set_bottom_radius(cylinder->get_radius());
			cylinder_mesh->set_top_radius(cylinder->get_radius());
			mesh = cylinder_mesh;
		}

		SphereShape* sphere = Object::cast_to<SphereShape>(*s);
		if (sphere) {
			Ref<SphereMesh> sphere_mesh;
			sphere_mesh.instance();
			sphere_mesh->set_radius(sphere->get_radius());
			sphere_mesh->set_height(sphere->get_radius() * 2.0);
			mesh = sphere_mesh;
		}

		ConcavePolygonShape* concave_polygon = Object::cast_to<ConcavePolygonShape>(*s);
		if (concave_polygon) {
			PoolVector3Array p_faces = concave_polygon->get_faces();
			Ref<ArrayMesh> array_mesh;
			array_mesh.instance();
			Array arrays;
			arrays.resize(ArrayMesh::ARRAY_MAX);
			arrays[ArrayMesh::ARRAY_VERTEX] = p_faces;

			array_mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays);
			mesh = array_mesh;
		}
		ConvexPolygonShape* convex_polygon = Object::cast_to<ConvexPolygonShape>(*s);
		if (convex_polygon) {
			PoolVector3Array varr = convex_polygon->get_points();
			/* Build a hull here */
			qh_vertex_t* vertices = new qh_vertex_t[varr.size()];

			for (int i = 0; i < varr.size(); ++i) {
				vertices[i].z = varr[i].z;
				vertices[i].x = varr[i].x;
				vertices[i].y = varr[i].y;
			}

			qh_mesh_t qh_mesh = qh_quickhull3d(vertices, varr.size());

			PoolVector3Array faces;
			faces.resize(qh_mesh.nindices);
			PoolVector3Array::Write w = faces.write();
			for (int j = 0; j < qh_mesh.nindices; j += 3) {
				w[j].x = qh_mesh.vertices[qh_mesh.indices[j]].x;
				w[j].y = qh_mesh.vertices[qh_mesh.indices[j]].y;
				w[j].z = qh_mesh.vertices[qh_mesh.indices[j]].z;
				w[j + 1].x = qh_mesh.vertices[qh_mesh.indices[j + 1]].x;
				w[j + 1].y = qh_mesh.vertices[qh_mesh.indices[j + 1]].y;
				w[j + 1].z = qh_mesh.vertices[qh_mesh.indices[j + 1]].z;
				w[j + 2].x = qh_mesh.vertices[qh_mesh.indices[j + 2]].x;
				w[j + 2].y = qh_mesh.vertices[qh_mesh.indices[j + 2]].y;
				w[j + 2].z = qh_mesh.vertices[qh_mesh.indices[j + 2]].z;
			}

			qh_free_mesh(qh_mesh);
			Ref<ArrayMesh> array_mesh;
			array_mesh.instance();
			Array arrays;
			arrays.resize(ArrayMesh::ARRAY_MAX);
			arrays[ArrayMesh::ARRAY_VERTEX] = faces;

			array_mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays);
			mesh = array_mesh;
		}

		if (mesh.is_valid()) {
			MeshInstance* mi = MeshInstance::_new();
			mi->set_mesh(mesh);
			aabbs->push_back(mi->get_aabb());
			meshes->push_back(mesh);
			transforms->push_back(transform);
			mi->set_mesh(NULL);
			mi->queue_free();
			mi->free();

		}
	}
}

void DetourNavigationMeshInstance::collect_mesh_instances(Array& geometries, std::vector<Ref<Mesh>>* meshes, std::vector<Transform>* transforms, std::vector<AABB>* aabbs) {
	/* If geometry source is MeshInstances, just collect them */
	int geom_size = geometries.size();
	for (int i = 0; i < geom_size; i++) {
		if (parsed_geometry_type == PARSED_GEOMETRY_MESH_INSTANCES) {
			MeshInstance* mesh_instance = Object::cast_to<MeshInstance>(geometries[i]);
			if (mesh_instance) {
				Godot::print("Found mesh instance");
				meshes->push_back(mesh_instance->get_mesh());
				transforms->push_back(mesh_instance->get_global_transform());
				aabbs->push_back(mesh_instance->get_aabb());
			}
		}
		else {
			StaticBody* static_body = Object::cast_to<StaticBody>(geometries[i]);

			if (static_body && !(static_body->get_collision_layer() & collision_mask)) {
				Godot::print("Found static body");
				convert_static_bodies(static_body, meshes, transforms, aabbs);
			}
		}
		Spatial* spatial = Object::cast_to<Spatial>(geometries[i]);
		if (spatial) {
			collect_mesh_instances(spatial->get_children(), meshes, transforms, aabbs);
		}
	}
}

void DetourNavigationMeshInstance::create_cached_navmesh() {
	DetourNavigationMeshCached *cached_navmesh = DetourNavigationMeshCached::_new();
	add_child(cached_navmesh);
	cached_navmesh->set_owner(get_tree()->get_edited_scene_root());
}

void DetourNavigationMeshInstance::create_navmesh() {
	DetourNavigationMesh *navmesh = DetourNavigationMesh::_new();
	add_child(navmesh);
	navmesh->set_owner(get_tree()->get_edited_scene_root());
}

void DetourNavigationMeshInstance::build_navmesh(DetourNavigationMesh* navmesh) {
	((DetourNavigationMeshCached *)navmesh)->init_values();
	std::vector<Ref<Mesh>> *meshes = new std::vector<Ref<Mesh>>();
	std::vector<Transform> *transforms = new std::vector<Transform>();
	std::vector<AABB> *aabbs = new std::vector<AABB>();
	DetourNavigationMeshInstance::collect_mesh_instances(get_children(), meshes, transforms, aabbs);
	Godot::print("Building navmesh...");


	for (int i = 0; i < meshes->size(); i++){
		navmesh->bounding_box.merge_with(
			transforms->at(i).xform(aabbs->at(i))
		);
	}
	Godot::print(navmesh->bounding_box);
	navmesh->init_mesh_data(meshes, transforms, aabbs, get_global_transform());

	float tile_edge_length = navmesh->get_tile_edge_length();
	Vector3 bmin = navmesh->bounding_box.position;
	Godot::print(navmesh->bounding_box.size);
	Vector3 bmax = navmesh->bounding_box.position + navmesh->bounding_box.size;

	/* We define width and height of the grid */
	int gridH = 0, gridW = 0;
	Godot::print(std::to_string(bmin.coord[0]).c_str());
	Godot::print(std::to_string(bmax.coord[0]).c_str());
	Godot::print(std::to_string(navmesh->cell_size).c_str());
	rcCalcGridSize(
		&bmin.coord[0],
		&bmax.coord[0],
		navmesh->cell_size,
		&gridW,
		&gridH
	);

	Godot::print(std::to_string(gridW).c_str());
	Godot::print(std::to_string(gridH).c_str());
	navmesh->set_tile_number(gridW, gridH); 

	std::string tile_message = "Tile number set to x:" + std::to_string(navmesh->get_num_tiles_x());
	tile_message += ", z:" + std::to_string(navmesh->get_num_tiles_z());
	Godot::print(tile_message.c_str());

	/* Calculate how many bits are required to uniquely identify all tiles */
	unsigned int tile_bits = (unsigned int)ilog2(nextPow2(navmesh->get_num_tiles_x() * navmesh->get_num_tiles_z()));
	tile_bits = std::min((int) tile_bits, 14);

	/* Set dt navmesh parameters*/
	unsigned int poly_bits = 22 - tile_bits;
	unsigned int max_tiles = 1u << tile_bits;
	unsigned int max_polys = 1 << poly_bits;

	dtNavMeshParams params;
	params.tileWidth = tile_edge_length;
	params.tileHeight = tile_edge_length;
	params.maxTiles = max_tiles;
	params.maxPolys = max_polys;
	params.orig[0] = params.orig[1] = params.orig[2] = 0.f;

	
	/* Initialize and allocate place for detour navmesh instance */
	if (!navmesh->alloc()) {
		Godot::print("Failed to allocate detour navmesh.");
		return;
	}
	if (!navmesh->init(&params)) {
		Godot::print("Failed to initialize detour navmesh.");
		return;
	}
	Godot::print(std::to_string(navmesh->agent_height).c_str());
	DetourNavigationMeshCached* ccd = dynamic_cast<DetourNavigationMeshCached*>(navmesh);

	if (DetourNavigationMeshCached* cached_navm = dynamic_cast<DetourNavigationMeshCached*>(navmesh))
	{
		dtTileCacheParams tile_cache_params;
		memset(&tile_cache_params, 0, sizeof(tile_cache_params));
		rcVcopy(tile_cache_params.orig, &bmin.coord[0]);
		tile_cache_params.ch = cached_navm->cell_height;
		tile_cache_params.cs = cached_navm->cell_size;
		tile_cache_params.width = cached_navm->tile_size;
		tile_cache_params.height = cached_navm->tile_size;
		tile_cache_params.maxSimplificationError = cached_navm->edge_max_error;
		tile_cache_params.maxTiles =
			cached_navm->get_num_tiles_x() * cached_navm->get_num_tiles_z() * cached_navm->max_layers;
		tile_cache_params.maxObstacles = cached_navm->max_obstacles;
		tile_cache_params.walkableClimb = cached_navm->agent_max_climb;
		tile_cache_params.walkableHeight = cached_navm->agent_height;
		tile_cache_params.walkableRadius = cached_navm->agent_radius;
		if (!cached_navm->alloc_tile_cache())
			return;
		if (!cached_navm->init_tile_cache(&tile_cache_params))
			return;

		/* We start building tiles */
		unsigned int result = cached_navm->build_tiles(
			0, 0, navmesh->get_num_tiles_x() - 1, navmesh->get_num_tiles_z() - 1
		);
		cached_navm->build_debug_mesh();
	}
	else {
		unsigned int result = navmesh->build_tiles(
			0, 0, navmesh->get_num_tiles_x() - 1, navmesh->get_num_tiles_z() - 1
		);
		navmesh->build_debug_mesh();
	}

	Godot::print("Successfully initialized navmesh.");
	
}

void DetourNavigationMeshInstance::_notification(int p_what) {
	/*
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			if (get_tree()->is_debugging_navigation_hint()) {
				MeshInstance* dm = MeshInstance::_new();
				if (navmesh != nullptr) {
					dm->set_mesh(navmesh->get_debug_mesh());
				}					
				dm->set_material_override(get_debug_navigation_material());
				add_child(dm);
				debug_mesh_instance = dm;
			}
			set_process(true);
		} break;
		case NOTIFICATION_EXIT_TREE: {
			if (debug_mesh_instance) {
				debug_mesh_instance->free();
				debug_mesh_instance->queue_free();
				debug_mesh_instance = NULL;
			}
			// Tile cache
			set_process(false);
		} break;
		// Tile Cache
		case NOTIFICATION_PROCESS: {
			float delta = get_process_delta_time();
			DetourNavigationMeshCached* cached_navm = navmesh;
			if (cached_navm != nullptr) {
				dtTileCache* tile_cache = cached_navm->get_tile_cache();
				if (tile_cache) {
					tile_cache->update(delta, cached_navm->get_detour_navmesh());
					if (true) { // If debug
						Object::cast_to<MeshInstance>(debug_mesh_instance)
							->set_mesh(cached_navm->get_debug_mesh());
					}
						
				}
			}
		} break;
	}*/
}
