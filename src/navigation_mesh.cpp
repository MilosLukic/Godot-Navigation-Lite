#include "navigation_mesh.h"
#include "navigation_query.h"
#include "navigation.h"



using namespace godot;

void DetourNavigationMesh::_register_methods() {
	register_method("_ready", &DetourNavigationMesh::_ready);
	//register_property<DetourNavigationMesh, Variant::OBJECT>("navmesh parameters", &DetourNavigationMesh::navmesh_parameters, NULL);
	register_property<DetourNavigationMesh, Ref<NavmeshParameters>>("parameters", &DetourNavigationMesh::navmesh_parameters, Ref<NavmeshParameters>(), GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_DEFAULT,
		GODOT_PROPERTY_HINT_RESOURCE_TYPE,
		"Resource");
	//register_property<DetourNavigationMesh, real_t>("cell_size", DetourNavigationMesh::navmesh_parameters->set_cell_size(), DetourNavigationMesh::navmesh_parameters->get_cell_size(), 32.0);
}
void DetourNavigationMesh::_init() {
	Godot::print("Navmehs inited");
	Godot::print(navmesh_parameters.is_valid());
	Godot::print("Navmehs inited ended");
	// initialize any variables here
}

void DetourNavigationMesh::_ready() {

	Godot::print("Navmehs ready");
	Godot::print(std::to_string(get_instance_id()).c_str());
	//build_navmesh();
	/*
	Godot::print("Adding obstacle");
	DetourNavigationMeshCached* cached_navm = (DetourNavigationMeshCached *)navmesh;
	unsigned int id = cached_navm->add_obstacle(Vector3(-5.f, 0.f, -5.f), 4.f, 3.f);
	dtTileCache* tile_cache = cached_navm->get_tile_cache();
	tile_cache->update(0.1f, cached_navm->get_detour_navmesh());

	build_debug_mesh();*/
}

DetourNavigationMesh::DetourNavigationMesh(){
	Godot::print("Navmehs constructor called");
}

DetourNavigationMesh::~DetourNavigationMesh() {
	Godot::print("Navigation mesh destructor called.");
	Godot::print(std::to_string(get_instance_id()).c_str());
	clear_debug_mesh();
	release_navmesh();
	if (navmesh_parameters.is_valid()) {
		navmesh_parameters.unref();
	}
}


void DetourNavigationMesh::release_navmesh() {
	if (detour_navmesh != nullptr) {
		dtFreeNavMesh(detour_navmesh);
	}
	detour_navmesh = NULL;
}

void DetourNavigationMesh::build_navmesh() {
	DetourNavigationMeshInstance* dtmi = Object::cast_to<DetourNavigationMeshInstance>(get_parent());
	if (dtmi == NULL) {
		return;
	}
	dtmi->build_navmesh(this);
}


Ref<ArrayMesh> DetourNavigationMesh::get_debug_mesh() {
	if (debug_mesh.is_valid()) {
		return debug_mesh;
	}
	std::list<Vector3> lines;
	const dtNavMesh* navm = detour_navmesh;
	for (int i = 0; i < navm->getMaxTiles(); i++) {
		const dtMeshTile* tile = navm->getTile(i);
		if (!tile || !tile->header) {
			continue;
		}
		for (int j = 0; j < tile->header->polyCount; j++) {
			dtPoly* poly = tile->polys + j;
			if (poly->getType() != DT_POLYTYPE_OFFMESH_CONNECTION) {
				for (int k = 0; k < poly->vertCount; k++) {
					lines.push_back(
						*reinterpret_cast<const Vector3*>(
							&tile->verts[poly->verts[k] * 3]
						)
					);
					lines.push_back(
						*reinterpret_cast<const Vector3*>(
							&tile->verts[poly->verts[(k + 1) % poly->vertCount] * 3]
						)
					);
				}
			}
			else if (poly->getType() == DT_POLYTYPE_OFFMESH_CONNECTION) {
				const dtOffMeshConnection* con =
					&tile->offMeshCons[j - tile->header->offMeshBase];
				const float* va = &tile->verts[poly->verts[0] * 3];
				const float* vb = &tile->verts[poly->verts[1] * 3];

				Vector3 p0 = *reinterpret_cast<const Vector3*>(va);
				Vector3 p1 = *reinterpret_cast<const Vector3*>(&con[0]);
				Vector3 p2 = *reinterpret_cast<const Vector3*>(&con[3]);
				Vector3 p3 = *reinterpret_cast<const Vector3*>(vb);
				lines.push_back(p0);
				lines.push_back(p1);
				lines.push_back(p1);
				lines.push_back(p2);
				lines.push_back(p2);
				lines.push_back(p3);
			}
		}
	}

	debug_mesh = Ref<ArrayMesh>(ArrayMesh::_new());
	PoolVector3Array varr;
	varr.resize(lines.size());
	PoolVector3Array::Write w = varr.write();
	int idx = 0;
	for (Vector3 vec : lines) {
		w[idx++] = vec;
	}


	Array arr;
	arr.resize(Mesh::ARRAY_MAX);
	arr[Mesh::ARRAY_VERTEX] = varr;

	debug_mesh->add_surface_from_arrays(Mesh::PRIMITIVE_LINES, arr);
	return debug_mesh;
}


dtNavMesh* DetourNavigationMesh::load_mesh() {
	dtNavMesh* dt_navmesh = FileManager::loadNavigationMesh("abc.bin");
	if (dt_navmesh == 0) {
		return 0;
	}
	else {
		detour_navmesh = dt_navmesh;
		return dt_navmesh;
	}
}

void DetourNavigationMesh::save_mesh() {
	Godot::print("Saving navmesh...");
	FileManager::saveNavigationMesh("abc.bin", detour_navmesh);
	Godot::print("Navmesh successfully saved.");

}


void DetourNavigationMesh::build_debug_mesh() {
	clear_debug_mesh();
	debug_mesh_instance = MeshInstance::_new();
	debug_mesh_instance->set_mesh(get_debug_mesh());
	debug_mesh_instance->set_material_override(get_debug_navigation_material());
	add_child(debug_mesh_instance);
}


Ref<Material> DetourNavigationMesh::get_debug_navigation_material() {
	/*	Create a navigation mesh material -
		it is not exposed in godot, so we have to create it here again
	*/
	Ref<SpatialMaterial> line_material = Ref<SpatialMaterial>(SpatialMaterial::_new());
	line_material->set_flag(SpatialMaterial::FLAG_UNSHADED, true);
	line_material->set_feature(SpatialMaterial::FEATURE_TRANSPARENT, true);
	line_material->set_flag(SpatialMaterial::FLAG_SRGB_VERTEX_COLOR, true);
	line_material->set_flag(SpatialMaterial::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);
	line_material->set_albedo(Color(0.1f, 1.0f, 0.7f, 0.4f));

	return line_material;
}

void DetourNavigationMesh::find_path() {
	DetourNavigationQuery* nav_query = new DetourNavigationQuery();
	nav_query->init(this, get_global_transform());
	//Dictionary result = nav_query->find_path(Vector3(0.f, 0.f, 0.f), Vector3(11.f, 0.3f, 11.f), Vector3(50.0f, 3.f, 50.f), new DetourNavigationQueryFilter());
	Dictionary result = nav_query->find_path(Vector3(3.f, 0.f, -15.f), Vector3(-5.6f, 0.3f, 2.8f), Vector3(50.0f, 3.f, 50.f), new DetourNavigationQueryFilter());
	Godot::print(result["points"]);
	result.clear();
}