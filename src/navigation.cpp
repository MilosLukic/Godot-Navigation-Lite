#include "navigation.h"

using namespace godot;

void DetourNavigationMeshInstance::_register_methods() {
	register_method("_ready", &DetourNavigationMeshInstance::_ready);
	//register_method("_process", &DetourNavigationMeshInstance::_process);
	register_method("build_mesh", &DetourNavigationMeshInstance::build_mesh);
}

DetourNavigationMeshInstance::DetourNavigationMeshInstance(){
}

DetourNavigationMeshInstance::~DetourNavigationMeshInstance(){
	if (navmesh) {
		navmesh->clear_debug_mesh();
	}
}

void DetourNavigationMeshInstance::_init() {
	// initialize any variables here
	time_passed = 0.0;
}

void DetourNavigationMeshInstance::_ready() {
	DetourNavigationMeshInstance::build_mesh();
	DetourNavigationMeshInstance::find_path();
}

void DetourNavigationMeshInstance::find_path() {
	DetourNavigationQuery *nav_query = new DetourNavigationQuery();
	nav_query->init(navmesh, get_global_transform());
	Dictionary result = nav_query->find_path(Vector3(0.f, 0.f, 0.f), Vector3(11.f, 0.3f, 11.f), Vector3(50.0f, 3.f, 50.f), new DetourNavigationQueryFilter());
	Godot::print(result["points"]);
}

void DetourNavigationMeshInstance::build_mesh() {
	Array geometries = get_tree()->get_nodes_in_group("navigation_group");
	navmesh = new DetourNavigationMesh();

	int geom_size = geometries.size();
	std::list<MeshInstance*> mesh_instances;

	for (int i = 0; i < geom_size; i++) {
		MeshInstance* mesh_instance = Object::cast_to<MeshInstance>(geometries[i]);
		if (mesh_instance) {
			mesh_instances.push_back(mesh_instance);
		}
	}

	for (auto const& mi : mesh_instances) {
		navmesh->bounding_box.merge_with(mi->get_transformed_aabb());
	}

	float tile_edge_length = navmesh->get_tile_edge_length();
	Vector3 bmin = navmesh->bounding_box.position;
	Vector3 bmax = navmesh->bounding_box.position + navmesh->bounding_box.size;

	/* We define width and height of the grid */
	int gridH = 0, gridW = 0;
	rcCalcGridSize(
		&bmin.coord[0],
		&bmax.coord[0],
		navmesh->cell_size,
		&gridW,
		&gridH
	);
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

	/* We start building tiles */
	Transform xform = get_global_transform();
	unsigned int result = navmesh->build_tiles(
		xform, mesh_instances, 0, 0, navmesh->get_num_tiles_x() - 1, navmesh->get_num_tiles_z() - 1
	);
	//Godot::print(std::to_string(result).c_str()); 
	/* TODO: Remove this mesh instance on deinit */
	if (navmesh) {
		navmesh->clear_debug_mesh();
		MeshInstance *dm = MeshInstance::_new();
		if (navmesh) {
			dm->set_mesh(navmesh->get_debug_mesh());
		}
		dm->set_material_override(get_debug_navigation_material());
		add_child(dm);
	}

	Godot::print("Successfully inited navmesh");
	
}


Ref<Material> DetourNavigationMeshInstance::get_debug_navigation_material() {
	Ref<SpatialMaterial> line_material = Ref<SpatialMaterial>(SpatialMaterial::_new());
	line_material->set_flag(SpatialMaterial::FLAG_UNSHADED, true);
	line_material->set_feature(SpatialMaterial::FEATURE_TRANSPARENT, true);
	line_material->set_flag(SpatialMaterial::FLAG_SRGB_VERTEX_COLOR, true);
	line_material->set_flag(SpatialMaterial::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);
	line_material->set_albedo(Color(0.1f, 1.0f, 0.7f, 0.4f));

	return line_material;
}

