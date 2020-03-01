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
	register_method("_on_cache_object_added", &DetourNavigation::_on_cache_object_added);
	register_method("_on_cache_object_removed", &DetourNavigation::_on_cache_object_removed);
	register_method("_on_static_object_added", &DetourNavigation::_on_static_object_added);
	register_method("_on_static_object_removed", &DetourNavigation::_on_static_object_removed);
	register_method("_on_tree_exiting", &DetourNavigation::_on_tree_exiting);

	register_property<DetourNavigation, int>(
		"parsed_geometry_type", &DetourNavigation::set_parsed_geometry_type, &DetourNavigation::get_parsed_geometry_type, (int) PARSED_GEOMETRY_STATIC_COLLIDERS,
		GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_DEFAULT, GODOT_PROPERTY_HINT_ENUM, "Static bodies,Mesh instances"
	);

}

DetourNavigation::DetourNavigation(){
	if (OS::get_singleton()->is_stdout_verbose()) {
		Godot::print("Navigation constructor function called.");
	}
	set_parsed_geometry_type(PARSED_GEOMETRY_STATIC_COLLIDERS);
}

DetourNavigation::~DetourNavigation(){
}


void DetourNavigation::_exit_tree() {

}

void DetourNavigation::_on_tree_exiting() {
	if (!Engine::get_singleton()->is_editor_hint()) {
		get_tree()->disconnect("node_added", this, "_on_cache_object_added");
		get_tree()->disconnect("node_removed", this, "_on_cache_object_removed");
	}

	get_tree()->disconnect("node_added", this, "_on_static_object_added");
	get_tree()->disconnect("node_removed", this, "_on_static_object_removed");
}

void DetourNavigation::_enter_tree() {
	if (!Engine::get_singleton()->is_editor_hint()) {
		get_tree()->connect("node_added", this, "_on_cache_object_added");
		get_tree()->connect("node_removed", this, "_on_cache_object_removed");
	}
}

void DetourNavigation::_init() {
	if (OS::get_singleton()->is_stdout_verbose()) {
		Godot::print("Navigation init function called.");
	}
	dyn_bodies_to_add = new std::vector<StaticBody*>();
	static_bodies_to_add = new std::vector<StaticBody*>();
	collisions_to_remove = new std::vector<CollisionShape*>();
}

void DetourNavigation::_ready() {
	if (OS::get_singleton()->is_stdout_verbose()) {
		Godot::print("Navigation ready function called.");
	}
	int dynamic_collision_mask = 0;
	int collision_mask = 0;

	for (int i = 0; i < get_child_count(); ++i) {
		DetourNavigationMeshCached* navmesh_pc = Object::cast_to<DetourNavigationMeshCached>(get_child(i));
		if (navmesh_pc != nullptr) {
			cached_navmeshes.push_back(navmesh_pc);
			build_navmesh_cached(navmesh_pc);
			collision_mask |= navmesh_pc->collision_mask;
			dynamic_collision_mask |= navmesh_pc->dynamic_collision_mask;
			//navmesh_pc->load_mesh();
		}

		DetourNavigationMesh* navmesh_p = Object::cast_to<DetourNavigationMesh>(get_child(i));
		if (navmesh_p != nullptr) {
			navmeshes.push_back(navmesh_p);
			build_navmesh(navmesh_p);
			collision_mask |= navmesh_p->collision_mask;
			//navmesh_p->load_mesh();
		}
	}
	if (Engine::get_singleton()->is_editor_hint()) {
		set_process(false);
	}


	get_tree()->connect("node_added", this, "_on_static_object_added");
	get_tree()->connect("node_removed", this, "_on_static_object_removed");

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

void DetourNavigation::remove_static_object(CollisionShape* collision_shape) {

	Ref<Shape> s = collision_shape->get_shape();

	for (int i = 0; i < get_child_count(); ++i) {
		DetourNavigationMesh* navmesh_pc = Object::cast_to<DetourNavigationMesh>(get_child(i));
		if (navmesh_pc != nullptr) {
			Godot::print("removing static object");
		}

	}
}


void DetourNavigation::_process() {
	if (static_bodies_to_add->size() > 0) {
		for (int i = 0; i < get_child_count(); ++i) {
			DetourNavigationMesh* navmesh_pc = Object::cast_to<DetourNavigationMesh>(get_child(i));
			if (navmesh_pc != nullptr) {
				for (StaticBody* static_body : *static_bodies_to_add) {
					for (int j = 0; j < static_body->get_child_count(); ++j) {
						CollisionShape* collision_shape = Object::cast_to<CollisionShape>(static_body->get_child(j));
						if (collision_shape) {
							convert_collision_shape(
								collision_shape,
								navmesh_pc->generator->input_meshes,
								navmesh_pc->generator->input_transforms,
								navmesh_pc->generator->input_aabbs,
								navmesh_pc->generator->collision_ids
							);
						}
					}
				}

				navmesh_pc->generator->recalculate_tiles(AABB());
				navmesh_pc->build_debug_mesh();
			}

		}

		static_bodies_to_add->clear();
	}

	if (collisions_to_remove->size() > 0) {
		for (int i = 0; i < get_child_count(); ++i) {
			DetourNavigationMesh* navmesh_pc = Object::cast_to<DetourNavigationMesh>(get_child(i));
			if (navmesh_pc != nullptr) {
				for (CollisionShape* collision_shape : *collisions_to_remove) {
					navmesh_pc->generator->remove_collision_shape(collision_shape->get_instance_id());
				}
				navmesh_pc->generator->recalculate_tiles(AABB());
				navmesh_pc->build_debug_mesh();
			}

		}
		collisions_to_remove->clear();
	}

	if (dyn_bodies_to_add->size() > 0) {
		for (StaticBody* static_body : *dyn_bodies_to_add) {

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

	set_process(false);

}

void DetourNavigation::_on_cache_object_added(Variant node) {
	CollisionShape* collision_shape = Object::cast_to<CollisionShape>(node.operator Object * ());
	if (collision_shape) {
		StaticBody* static_body = Object::cast_to<StaticBody>(collision_shape->get_parent());
		if (static_body && static_body->get_collision_layer() & dynamic_collision_mask) {
			dyn_bodies_to_add->push_back(static_body);
			set_process(true);
		}
	}

}

void DetourNavigation::_on_cache_object_removed(Variant node) {
	CollisionShape* collision_shape = Object::cast_to<CollisionShape>(node.operator Object * ());
	if (collision_shape) {
		StaticBody* static_body = Object::cast_to<StaticBody>(collision_shape->get_parent());
		if (static_body && static_body->get_collision_layer() & dynamic_collision_mask) {
			remove_obstacle(collision_shape);
		}
	}
}

void DetourNavigation::_on_static_object_added(Variant node) {
	CollisionShape* collision_shape = Object::cast_to<CollisionShape>(node.operator Object * ());
	if (collision_shape) {
		StaticBody* static_body = Object::cast_to<StaticBody>(collision_shape->get_parent());
		if (static_body && !(static_body->get_collision_layer() & dynamic_collision_mask) &&
			(static_body->get_collision_layer() & collision_mask)) {
			Godot::print("added static");
			static_bodies_to_add->push_back(static_body);
			set_process(true);
		}
	}

}

void DetourNavigation::_on_static_object_removed(Variant node) {
	CollisionShape* collision_shape = Object::cast_to<CollisionShape>(node.operator Object * ());
	if (collision_shape) {
		StaticBody* static_body = Object::cast_to<StaticBody>(collision_shape->get_parent());
		if (static_body && !(static_body->get_collision_layer() & dynamic_collision_mask) && 
			(static_body->get_collision_layer() & collision_mask)) {
			Godot::print("removed static");
			collisions_to_remove->push_back(collision_shape);

			set_process(true);
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
	std::vector<int64_t>* collision_ids = new std::vector<int64_t>();

	DetourNavigation::collect_mesh_instances(get_children(), meshes, transforms, aabbs, collision_ids, navmesh);

	DetourNavigationMeshGenerator* dtnavmesh_gen = new DetourNavigationMeshGenerator();
	dtnavmesh_gen->init_mesh_data(meshes, transforms, aabbs, get_global_transform(), collision_ids);
	dtnavmesh_gen->navmesh_parameters = navmesh->navmesh_parameters;
	dtnavmesh_gen->build();

	navmesh->generator = dtnavmesh_gen;
	navmesh->detour_navmesh = dtnavmesh_gen->detour_navmesh;
	navmesh->build_debug_mesh();

}

void DetourNavigation::build_navmesh_cached(DetourNavigationMeshCached* navmesh) {
	std::vector<Ref<Mesh>>* meshes = new std::vector<Ref<Mesh>>();
	std::vector<Transform>* transforms = new std::vector<Transform>();
	std::vector<AABB>* aabbs = new std::vector<AABB>();
	std::vector<int64_t>* collision_ids = new std::vector<int64_t>();

	DetourNavigation::collect_mesh_instances(get_children(), meshes, transforms, aabbs, collision_ids, navmesh);

	DetourNavigationMeshCacheGenerator* dtnavmesh_gen = new DetourNavigationMeshCacheGenerator();
	dtnavmesh_gen->init_mesh_data(meshes, transforms, aabbs, get_global_transform(), collision_ids);
	dtnavmesh_gen->navmesh_parameters = navmesh->navmesh_parameters;
	dtnavmesh_gen->build();

	navmesh->detour_navmesh = dtnavmesh_gen->detour_navmesh;
	navmesh->tile_cache = dtnavmesh_gen->get_tile_cache();
	navmesh->tile_cache_compressor = dtnavmesh_gen->get_tile_cache_compressor();
	navmesh->mesh_process = dtnavmesh_gen->get_mesh_process();

	navmesh->build_debug_mesh();
}

void DetourNavigation::_notification(int p_what) {
	switch (p_what) {
	case NOTIFICATION_PREDELETE: {
		_on_tree_exiting();
	} break;
	}
}


/**
 * Processes a mesh and if it's too large it splits it in
 * smaller chunks
 *
 * @return 1 for mesh found and 0 if mesh is too small to be split
 */
int DetourNavigation::process_large_mesh(
	MeshInstance *mesh_instance, int64_t collision_id, std::vector<Ref<Mesh>>* meshes, std::vector<Transform>* transforms, std::vector<AABB>* aabbs, std::vector<int64_t>* collision_ids
) {
	Ref<ArrayMesh> array_mesh = (Ref<ArrayMesh>) mesh_instance->get_mesh();


	if (!array_mesh.is_valid()) {
		return 0;
	}
	float tile_edge_length = 0;
	if (cached_navmeshes.size() < 0) {
		tile_edge_length = cached_navmeshes[0]->navmesh_parameters->get_tile_edge_length();
	}
	else {
		tile_edge_length = navmeshes[0]->navmesh_parameters->get_tile_edge_length();
	}

	int current_vertex_count, face_count = 0;

	AABB transformed_aabb = mesh_instance->get_transform().xform(mesh_instance->get_aabb());
	Godot::print(mesh_instance->get_transform().xform(mesh_instance->get_aabb()));


	for (int i = 0; i < array_mesh->get_surface_count(); i++) {

		if (array_mesh->surface_get_primitive_type(i) != Mesh::PRIMITIVE_TRIANGLES)
			return 0;

		if ( transformed_aabb.get_size().x < tile_edge_length && transformed_aabb.get_size().z < tile_edge_length) {
			return 0;
		}
		int index_count = 0;

		bool index_format = array_mesh->surface_get_format(i) & Mesh::ARRAY_FORMAT_INDEX;
		PoolVector3Array mesh_vertices;
		
		if (index_format) {
			index_count = array_mesh->surface_get_array_index_len(i);
			return 0; // Temporary continue 
		}
		else {
			mesh_vertices = array_mesh->surface_get_arrays(i)[Mesh::ARRAY_VERTEX];
			index_count = array_mesh->surface_get_array_len(i);
			face_count = mesh_vertices.size() / 3;
		}


		if (face_count < 200) {
			return 0;
		}

		// We want to split the mesh into "tile edge length" pieces
		Vector3 base_position = transformed_aabb.get_position();
		Godot::print(base_position);
		Vector3 make_positive = Vector3(0.f, 0.f, 0.f);
		make_positive.x -= base_position.x;
		make_positive.z -= base_position.z;


		int x_size = (int) std::ceilf((base_position.x + make_positive.x + transformed_aabb.get_size().x) / tile_edge_length);
		int z_size = (int) std::ceilf((base_position.z + make_positive.z + transformed_aabb.get_size().z) / tile_edge_length);


		std::vector<std::vector<std::list<Vector3>>> mesh_instance_array(x_size, std::vector<std::list<Vector3>>(z_size));

		for (int j = 0; j < face_count; j++) {
			Vector3 a = mesh_instance->get_transform().xform(mesh_vertices[j * 3 + 0]) + make_positive;
			Vector3 b = mesh_instance->get_transform().xform(mesh_vertices[j * 3 + 1]) + make_positive;
			Vector3 c = mesh_instance->get_transform().xform(mesh_vertices[j * 3 + 2]) + make_positive;

			float min_x = std::min(std::min(a.x, b.x), c.x);
			float min_z = std::min(std::min(a.z, b.z), c.z);


			float max_x = std::max(std::max(a.x, b.x), c.x);
			float max_z = std::max(std::max(a.z, b.z), c.z);

			int tile_min_x = (int) std::floorf((min_x) / tile_edge_length);
			int tile_min_z = (int) std::floorf((min_z) / tile_edge_length);
			int tile_max_x = (int) std::floorf((max_x) / tile_edge_length);
			int tile_max_z = (int) std::floorf((max_z) / tile_edge_length);

			for (int x = tile_min_x; x <= tile_max_x; x++) {
				for (int z = tile_min_z; z <= tile_max_z; z++) {
					mesh_instance_array[x][z].push_back(mesh_vertices[j * 3 + 0]);
					mesh_instance_array[x][z].push_back(mesh_vertices[j * 3 + 1]);
					mesh_instance_array[x][z].push_back(mesh_vertices[j * 3 + 2]);
				}
			}
		}


		for (int i = 0; i < mesh_instance_array.size(); i++) {
			for (int j = 0; j < mesh_instance_array[i].size(); j++) {
				//Godot::print((std::to_string(i) + "   ::   " + std::to_string(j) + " = " + std::to_string(mesh_instance_array[i][j].size() / 3)).c_str());
				PoolVector3Array faces;
				faces.resize(mesh_instance_array[i][j].size());
				PoolVector3Array::Write w = faces.write();
				int k = 0;
				for (Vector3 a : mesh_instance_array[i][j]) {
					w[k] = a;
					k++;
				}

				Ref<ArrayMesh> am;
				am.instance();
				Array arrays;
				arrays.resize(ArrayMesh::ARRAY_MAX);
				arrays[ArrayMesh::ARRAY_VERTEX] = faces;
				am->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays);

				aabbs->push_back(mesh_instance->get_aabb());
				meshes->push_back(am);
				transforms->push_back(mesh_instance->get_transform());
				collision_ids->push_back(collision_id);

			}
		}

	}


	return 1;
}

void DetourNavigation::convert_collision_shape(
	CollisionShape* collision_shape, std::vector<Ref<Mesh>>* meshes, std::vector<Transform>* transforms, std::vector<AABB>* aabbs, std::vector<int64_t>* collision_ids
) {
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
		mi->set_transform(transform);
		if (!process_large_mesh(mi, collision_shape->get_instance_id(), meshes, transforms, aabbs, collision_ids)) {
			aabbs->push_back(mi->get_aabb());
			meshes->push_back(mesh);
			transforms->push_back(transform);
			collision_ids->push_back(collision_shape->get_instance_id());
		}
		mi->set_mesh(NULL);
		mi->queue_free();
		mi->free();

	}
}

void DetourNavigation::collect_mesh_instances(
	Array& geometries, 
	std::vector<Ref<Mesh>>* meshes, 
	std::vector<Transform>* transforms, 
	std::vector<AABB>* aabbs, 
	std::vector<int64_t>* collision_ids,
	DetourNavigationMesh* navmesh
) 
{
	
	int geom_size = geometries.size();
	for (int i = 0; i < geom_size; i++) {
		// If geometry source is MeshInstances, just collect them 
		if (parsed_geometry_type == PARSED_GEOMETRY_MESH_INSTANCES) {
			MeshInstance* mesh_instance = Object::cast_to<MeshInstance>(geometries[i]);
			if (mesh_instance) {
				meshes->push_back(mesh_instance->get_mesh());
				transforms->push_back(mesh_instance->get_global_transform());
				aabbs->push_back(mesh_instance->get_aabb());
			}
		}
		// If geometry source is Static bodies, convert them to meshes
		else {

			StaticBody* static_body = Object::cast_to<StaticBody>(geometries[i]);
			if (static_body && static_body->get_collision_layer() & navmesh->collision_mask) {
				for (int i = 0; i < static_body->get_child_count(); ++i) {
					CollisionShape* collision_shape = Object::cast_to<CollisionShape>(static_body->get_child(i));
					if (collision_shape) {
						Godot::print(static_body->get_parent()->get_name());
						convert_collision_shape(collision_shape, meshes, transforms, aabbs, collision_ids);
					}
				}
			}
			
		}
		Spatial* spatial = Object::cast_to<Spatial>(geometries[i]);
		if (spatial) {
			collect_mesh_instances(spatial->get_children(), meshes, transforms, aabbs, collision_ids, navmesh);
		}
	}
	
}
