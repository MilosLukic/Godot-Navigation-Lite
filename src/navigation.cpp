#include "navigation.h"
#define QUICKHULL_IMPLEMENTATION
#include "quickhull.h"

using namespace godot;

void DetourNavigation::_register_methods() {
	register_method("_ready", &DetourNavigation::_ready);
	register_method("_process", &DetourNavigation::_process);
	register_method("create_cached_navmesh", &DetourNavigation::create_cached_navmesh);
	register_method("create_navmesh", &DetourNavigation::create_navmesh);
	register_method("_exit_tree", &DetourNavigation::_exit_tree);
	register_method("_enter_tree", &DetourNavigation::_enter_tree);
	register_method("_on_node_added", &DetourNavigation::_on_node_added);
	register_method("_on_node_removed", &DetourNavigation::_on_node_removed);

	register_property<DetourNavigation, int>(
		"parsed_geometry_type", &DetourNavigation::set_parsed_geometry_type, &DetourNavigation::get_parsed_geometry_type, (int) PARSED_GEOMETRY_STATIC_COLLIDERS,
		GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_DEFAULT, GODOT_PROPERTY_HINT_ENUM, "Static bodies,Mesh instances"
	);
	register_property<DetourNavigation, int>("collision_mask", &DetourNavigation::set_collision_mask, &DetourNavigation::get_collision_mask, 1, 
		GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_DEFAULT, GODOT_PROPERTY_HINT_LAYERS_3D_PHYSICS);

	register_property<DetourNavigation, bool>("cache_objects", &DetourNavigation::cache_objects, true);
	register_property<DetourNavigation, int>("cache_objects_collision_mask", &DetourNavigation::set_cache_collision_mask, &DetourNavigation::get_cache_collision_mask, 1,
		GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_DEFAULT, GODOT_PROPERTY_HINT_LAYERS_3D_PHYSICS);
}

DetourNavigation::DetourNavigation(){
	if (OS::get_singleton()->is_stdout_verbose()) {
		Godot::print("Navigation constructor function called.");
	}
	set_collision_mask(1);
	set_cache_collision_mask(2);
	set_parsed_geometry_type(PARSED_GEOMETRY_STATIC_COLLIDERS);
}

DetourNavigation::~DetourNavigation(){
}


void DetourNavigation::_exit_tree() {
	if (!Engine::get_singleton()->is_editor_hint()) {
		get_tree()->disconnect("node_added", this, "_on_node_added");
		get_tree()->disconnect("node_removed", this, "_on_node_removed");
	}
}

void DetourNavigation::_enter_tree() {
	if (!Engine::get_singleton()->is_editor_hint()) {
		get_tree()->connect("node_added", this, "_on_node_added");
		get_tree()->connect("node_removed", this, "_on_node_removed");
	}
}

void DetourNavigation::_init() {
	if (OS::get_singleton()->is_stdout_verbose()) {
		Godot::print("Navigation init function called.");
	}
	dyn_bodies_to_add = new std::vector<StaticBody*>();
}

void DetourNavigation::_ready() {
	if (OS::get_singleton()->is_stdout_verbose()) {
		Godot::print("Navigation ready function called.");
	}

	for (int i = 0; i < get_child_count(); ++i) {
		DetourNavigationMeshCached* navmesh_pc = Object::cast_to<DetourNavigationMeshCached>(get_child(i));
		if (navmesh_pc != nullptr) {
			navmesh_pc->load_mesh();
			cached_navmeshes.push_back(navmesh_pc);
		}

		DetourNavigationMesh* navmesh_p = Object::cast_to<DetourNavigationMesh>(get_child(i));
		if (navmesh_p != nullptr) {
			navmesh_p->load_mesh();
		}
	}
	if (Engine::get_singleton()->is_editor_hint()) {
		set_process(false);
	}

}
void DetourNavigation::update_tilecache() {
	for (int i = 0; i < get_child_count(); ++i) {
		DetourNavigationMeshCached* navmesh_pc = Object::cast_to<DetourNavigationMeshCached>(get_child(i));
		if (navmesh_pc != nullptr) {
			navmesh_pc->get_tile_cache()->update(0.1f, navmesh_pc->get_detour_navmesh());

			if (get_tree()->is_debugging_navigation_hint()) {
				navmesh_pc->build_debug_mesh();
			}
		}
	}

}
void DetourNavigation::add_box_obstacle_to_all(int64_t instance_id, Vector3 position, Vector3 extents, float rotationY) {
	
	for (int i = 0; i < get_child_count(); ++i) {
		DetourNavigationMeshCached* navmesh_pc = Object::cast_to<DetourNavigationMeshCached>(get_child(i));
		if (navmesh_pc != nullptr) {
			unsigned int ref_id = navmesh_pc->add_box_obstacle(position, extents, rotationY);

			navmesh_pc->dynamic_obstacles[instance_id] = (Variant) ref_id;
		}

	}
}

void DetourNavigation::remove_obstacle(CollisionShape* collision_shape) {

	Ref<Shape> s = collision_shape->get_shape();

	for (int i = 0; i < get_child_count(); ++i) {
		DetourNavigationMeshCached* navmesh_pc = Object::cast_to<DetourNavigationMeshCached>(get_child(i));
		if (navmesh_pc != nullptr) {
			navmesh_pc->remove_obstacle(navmesh_pc->dynamic_obstacles[collision_shape->get_instance_id()]);
		}

	}
}


void DetourNavigation::_process() {
	for (StaticBody * static_body : *dyn_bodies_to_add) {

		for (int i = 0; i < static_body->get_child_count(); ++i) {
			CollisionShape* collision_shape = Object::cast_to<CollisionShape>(static_body->get_child(i));
			if (collision_shape == NULL) {
				continue;
			}
			Transform transform = collision_shape->get_global_transform();

			Ref<Shape> s = collision_shape->get_shape();

			BoxShape* box = Object::cast_to<BoxShape>(*s);
			if (box) {
				add_box_obstacle_to_all(collision_shape->get_instance_id(), transform.get_origin(), box->get_extents(), transform.basis.get_euler().y);
			}
		}
	
	
	}

	update_tilecache();
	dyn_bodies_to_add->clear();

}

void DetourNavigation::_on_node_added(Variant node) {
	CollisionShape* collision_shape = Object::cast_to<CollisionShape>(node.operator Object * ());
	if (collision_shape) {
		StaticBody* static_body = Object::cast_to<StaticBody>(collision_shape->get_parent());
		if (static_body && static_body->get_collision_layer() & cache_collision_mask) {
			dyn_bodies_to_add->push_back(static_body);
			set_process(true);
		}
	}

}

void DetourNavigation::_on_node_removed(Variant node) {
	CollisionShape* collision_shape = Object::cast_to<CollisionShape>(node.operator Object * ());
	if (collision_shape) {
		StaticBody* static_body = Object::cast_to<StaticBody>(collision_shape->get_parent());
		if (static_body && static_body->get_collision_layer() & cache_collision_mask) {
			remove_obstacle(collision_shape);
		}
	}
}



DetourNavigationMeshCached *DetourNavigation::create_cached_navmesh(Ref<CachedNavmeshParameters> np) {
	DetourNavigationMeshCached *cached_navmesh = DetourNavigationMeshCached::_new();
	cached_navmesh->navmesh_parameters = np;
	add_child(cached_navmesh);
	cached_navmesh->set_owner(get_tree()->get_edited_scene_root());
	cached_navmesh->set_name("CachedDetourNavigationMesh");
	return cached_navmesh;
}

DetourNavigationMesh *DetourNavigation::create_navmesh(Ref<NavmeshParameters> np) {
	DetourNavigationMesh *navmesh = DetourNavigationMesh::_new();
	navmesh->navmesh_parameters = np;
	add_child(navmesh);
	navmesh->set_owner(get_tree()->get_edited_scene_root());
	navmesh->set_name("DetourNavigationMesh");
	return navmesh;
}


void DetourNavigation::build_navmesh(DetourNavigationMesh* navmesh) {
	std::vector<Ref<Mesh>>* meshes = new std::vector<Ref<Mesh>>();
	std::vector<Transform>* transforms = new std::vector<Transform>();
	std::vector<AABB>* aabbs = new std::vector<AABB>();
	DetourNavigation::collect_mesh_instances(get_children(), meshes, transforms, aabbs);

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

void DetourNavigation::build_navmesh_cached(DetourNavigationMeshCached* navmesh) {
	std::vector<Ref<Mesh>>* meshes = new std::vector<Ref<Mesh>>();
	std::vector<Transform>* transforms = new std::vector<Transform>();
	std::vector<AABB>* aabbs = new std::vector<AABB>();
	DetourNavigation::collect_mesh_instances(get_children(), meshes, transforms, aabbs);


	DetourNavigationMeshCacheGenerator* dtnavmesh_gen = new DetourNavigationMeshCacheGenerator();
	dtnavmesh_gen->init_mesh_data(meshes, transforms, aabbs, get_global_transform());
	dtnavmesh_gen->navmesh_parameters = navmesh->navmesh_parameters;

	dtnavmesh_gen->build();
	navmesh->detour_navmesh = dtnavmesh_gen->detour_navmesh;
	navmesh->tile_cache = dtnavmesh_gen->get_tile_cache();
	navmesh->tile_cache_compressor = dtnavmesh_gen->get_tile_cache_compressor();
	navmesh->mesh_process = dtnavmesh_gen->get_mesh_process();

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

void DetourNavigation::_notification(int p_what) {

}


void DetourNavigation::convert_static_bodies(
	StaticBody* static_body, std::vector<Ref<Mesh>>* meshes, std::vector<Transform>* transforms, std::vector<AABB>* aabbs
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

void DetourNavigation::collect_mesh_instances(Array& geometries, std::vector<Ref<Mesh>>* meshes, std::vector<Transform>* transforms, std::vector<AABB>* aabbs) {
	// If geometry source is MeshInstances, just collect them 
	int geom_size = geometries.size();
	for (int i = 0; i < geom_size; i++) {
		if (parsed_geometry_type == PARSED_GEOMETRY_MESH_INSTANCES) {
			MeshInstance* mesh_instance = Object::cast_to<MeshInstance>(geometries[i]);
			if (mesh_instance) {
				meshes->push_back(mesh_instance->get_mesh());
				transforms->push_back(mesh_instance->get_global_transform());
				aabbs->push_back(mesh_instance->get_aabb());
			}
		}
		else {
			StaticBody* static_body = Object::cast_to<StaticBody>(geometries[i]);

			if (static_body && static_body->get_collision_layer() & collision_mask) {
				convert_static_bodies(static_body, meshes, transforms, aabbs);
			}
		}
		Spatial* spatial = Object::cast_to<Spatial>(geometries[i]);
		if (spatial) {
			collect_mesh_instances(spatial->get_children(), meshes, transforms, aabbs);
		}
	}
}
