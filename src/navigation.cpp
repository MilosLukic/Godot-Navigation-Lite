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
	Godot::print("Navigation constructor called");
	set_parsed_geometry_type(PARSED_GEOMETRY_STATIC_COLLIDERS);
}

DetourNavigationMeshInstance::~DetourNavigationMeshInstance(){
	Godot::print("deconstruct navigation");
}

void DetourNavigationMeshInstance::_init() {
	Godot::print("Navigation inited");
}

void DetourNavigationMeshInstance::_ready() {
	parsed_geometry_type = PARSED_GEOMETRY_MESH_INSTANCES;
	collision_mask |= 1;
	Godot::print("Navigation ready called.");


	for (int i = 0; i < get_child_count(); ++i) {
		DetourNavigationMeshCached* navmesh_pc = Object::cast_to<DetourNavigationMeshCached>(get_child(i));
		if (navmesh_pc != nullptr) {
			Godot::print("building cached");
			build_navmesh(navmesh_pc);
			return;
		}

		DetourNavigationMesh* navmesh_p = Object::cast_to<DetourNavigationMesh>(get_child(i));
		if (navmesh_p != nullptr) {
			Godot::print("building regular");
			build_navmesh(navmesh_p);
			return;
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
	// If geometry source is MeshInstances, just collect them 
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
	/*DetourNavigationMeshCached *cached_navmesh = DetourNavigationMeshCached::_new();
	add_child(cached_navmesh);
	cached_navmesh->set_owner(get_tree()->get_edited_scene_root());*/
}

DetourNavigationMesh *DetourNavigationMeshInstance::create_navmesh(Ref<NavmeshParameters> np) {
	DetourNavigationMesh *navmesh = DetourNavigationMesh::_new();
	navmesh->navmesh_parameters = np;
	add_child(navmesh);
	navmesh->set_owner(get_tree()->get_edited_scene_root());
	return navmesh;
}

void DetourNavigationMeshInstance::build_navmesh(DetourNavigationMesh* navmesh) {
	std::vector<Ref<Mesh>>* meshes = new std::vector<Ref<Mesh>>();
	std::vector<Transform>* transforms = new std::vector<Transform>();
	std::vector<AABB>* aabbs = new std::vector<AABB>();
	DetourNavigationMeshInstance::collect_mesh_instances(get_children(), meshes, transforms, aabbs);

	DetourNavigationMeshGenerator* dtnavmesh_gen = new DetourNavigationMeshGenerator();
	dtnavmesh_gen->init_mesh_data(meshes, transforms, aabbs, get_global_transform());
	dtnavmesh_gen->navmesh_parameters = navmesh->navmesh_parameters;
	dtnavmesh_gen->build();
	navmesh->detour_navmesh = dtnavmesh_gen->detour_navmesh;
	navmesh->build_debug_mesh();

	delete dtnavmesh_gen;
	for (Ref<ArrayMesh> m : *meshes) {
		if (m.is_valid()) {
			m.unref();
		}
	}
	delete meshes;
	delete transforms;
	delete aabbs;
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
