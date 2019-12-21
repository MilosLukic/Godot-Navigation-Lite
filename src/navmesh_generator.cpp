#include "navmesh_generator.h"

using namespace godot;
DetourNavigationMeshGenerator::DetourNavigationMeshGenerator() {
}

DetourNavigationMeshGenerator::~DetourNavigationMeshGenerator() {
}

void DetourNavigationMeshGenerator::build() {
	for (int i = 0; i < input_meshes->size(); i++) {
			bounding_box.merge_with(
			input_transforms->at(i).xform(input_aabbs->at(i))
		);
	}
	float tile_edge_length = get_tile_edge_length();
	Vector3 bmin = bounding_box.position;
	Vector3 bmax = bounding_box.position + bounding_box.size;

	Godot::print(navmesh_parameters->get_cell_size());
	/* We define width and height of the grid */
	int gridH = 0, gridW = 0;
	rcCalcGridSize(
		&bmin.coord[0],
		&bmax.coord[0],
		navmesh_parameters->get_cell_size(),
		&gridW,
		&gridH
	);

	set_tile_number(gridW, gridH);

	std::string tile_message = "Tile number set to x:" + std::to_string(get_num_tiles_x());
	tile_message += ", z:" + std::to_string(get_num_tiles_z());
	Godot::print(tile_message.c_str());

	/* Calculate how many bits are required to uniquely identify all tiles */
	unsigned int tile_bits = (unsigned int)ilog2(nextPow2(get_num_tiles_x() * get_num_tiles_z()));
	tile_bits = std::min((int)tile_bits, 14);

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
	if (!alloc()) {
		Godot::print("Failed to allocate detour navmesh.");
		return;
	}
	if (!init(&params)) {
		Godot::print("Failed to initialize detour navmesh.");
		return;
	}
	/*DetourNavigationMeshCached* cached_navm = dynamic_cast<DetourNavigationMeshCached*>(navmesh);
	if (cached_navm != nullptr)
	{
		dtTileCacheParams tile_cache_params;
		memset(&tile_cache_params, 0, sizeof(tile_cache_params));
		rcVcopy(tile_cache_params.orig, &bmin.coord[0]);
		tile_cache_params.ch = cached_navm->cell_height;
		tile_cache_params.cs = cached_navm->cell_size;
		tile_cache_params.width = cached_navm->tile_size;
		tile_cache_params.height = cached_navm->tile_size;
		tile_cache_params.maxSimplificationError = cached_navm->edge_max_error;
		tile_cache_params.maxTiles = get_num_tiles_x() * get_num_tiles_z() * max_layers;
		tile_cache_params.maxObstacles = cached_navm->max_obstacles;
		tile_cache_params.walkableClimb = cached_navm->agent_max_climb;
		tile_cache_params.walkableHeight = cached_navm->agent_height;
		tile_cache_params.walkableRadius = cached_navm->agent_radius;
		if (!cached_navm->alloc_tile_cache())
			return;
		if (!cached_navm->init_tile_cache(&tile_cache_params))
			return;

		unsigned int result = cached_navm->build_tiles(
			0, 0, navmesh->get_num_tiles_x() - 1, navmesh->get_num_tiles_z() - 1
		);
		cached_navm->build_debug_mesh();
	}
	else {*/
		unsigned int result = build_tiles(
			0, 0, get_num_tiles_x() - 1, get_num_tiles_z() - 1
		);
	//}

}

unsigned int DetourNavigationMeshGenerator::build_tiles(
	int x1, int z1, int x2, int z2
) {
	unsigned ret = 0;
	for (int z = z1; z <= z2; z++) {
		for (int x = x1; x <= x2; x++) {
			if (build_tile(x, z)) {
				ret++;
			}
		}
	}
	return ret;
}

void DetourNavigationMeshGenerator::init_rc_config(rcConfig& config, Vector3& bmin, Vector3& bmax) {
	config.cs = navmesh_parameters->get_cell_size();
	config.ch = navmesh_parameters->get_cell_height();
	config.walkableSlopeAngle = navmesh_parameters->get_agent_max_slope();
	config.walkableHeight = (int)ceil(navmesh_parameters->get_agent_height() / config.ch);
	config.walkableClimb = (int)floor(navmesh_parameters->get_agent_max_climb() / config.ch);
	config.walkableRadius = (int)ceil(navmesh_parameters->get_agent_radius() / config.cs);

	config.maxEdgeLen = (int)(navmesh_parameters->get_edge_max_length()/ config.cs);
	config.maxSimplificationError = navmesh_parameters->get_edge_max_error();
	config.minRegionArea = (int)sqrtf(navmesh_parameters->get_region_min_size());
	config.mergeRegionArea = (int)sqrtf(navmesh_parameters->get_region_merge_size());
	config.maxVertsPerPoly = 6;
	config.tileSize = navmesh_parameters->get_tile_size();
	config.borderSize = config.walkableRadius + 3;
	config.width = config.tileSize + config.borderSize * 2;
	config.height = config.tileSize + config.borderSize * 2;
	config.detailSampleDist =
		navmesh_parameters->get_detail_sample_distance() < 0.9f ? 0.0f : config.cs * navmesh_parameters->get_detail_sample_distance();
	config.detailSampleMaxError = config.ch * navmesh_parameters->get_detail_sample_max_error();

	rcVcopy(config.bmin, &bmin.coord[0]);
	rcVcopy(config.bmax, &bmax.coord[0]);
	config.bmin[0] -= config.borderSize * config.cs;
	config.bmin[2] -= config.borderSize * config.cs;
	config.bmax[0] += config.borderSize * config.cs;
	config.bmax[2] += config.borderSize * config.cs;
}

bool DetourNavigationMeshGenerator::init_tile_data(
	rcConfig& config, Vector3& bmin, Vector3& bmax, std::vector<float>& points,
	std::vector<int>& indices
) {
	/* Set the tile AABB */
	AABB expbox(bmin, bmax - bmin);
	expbox.position.x -= config.borderSize * config.cs;
	expbox.position.z -= config.borderSize * config.cs;
	expbox.size.x += 2.0 * config.borderSize * config.cs;
	expbox.size.z += 2.0 * config.borderSize * config.cs;


	Transform base = global_transform.inverse();

	for (int i = 0; i < input_meshes->size(); i++) {
		if (!input_meshes->at(i).is_valid()) {
			continue;
		}
		Transform mxform = base * input_transforms->at(i);
		AABB mesh_aabb = mxform.xform(input_aabbs->at(i));
		if (!mesh_aabb.intersects_inclusive(expbox) &&
			!expbox.encloses(mesh_aabb)) {
			continue;
		}
		add_meshdata(i, points, indices);
	}

	if (points.size() == 0 || indices.size() == 0) {
		Godot::print("No mesh points and indices found...");
		return true;
	}
	return false;
}

bool DetourNavigationMeshGenerator::init_heightfield_context(
	rcConfig& config, rcCompactHeightfield* compact_heightfield,
	rcContext* ctx, std::vector<float>& points, std::vector<int>& indices
) {
	// returns success value

	rcHeightfield* heightfield = rcAllocHeightfield();

	if (!heightfield) {
		ERR_PRINT("Failed to allocate height field");
		return false;
	}
	if (!rcCreateHeightfield(
		ctx, *heightfield,
		config.width, config.height, config.bmin,
		config.bmax, config.cs, config.ch)
		) {
		ERR_PRINT("Failed to create height field");
		return false;
	}

	int ntris = indices.size() / 3;
	std::vector<unsigned char> tri_areas;
	tri_areas.resize(ntris);
	memset(&tri_areas[0], 0, ntris);

	rcMarkWalkableTriangles(
		ctx, config.walkableSlopeAngle, &points[0],
		points.size() / 3, &indices[0], ntris, &tri_areas[0]
	);

	rcRasterizeTriangles(
		ctx, &points[0], points.size() / 3, &indices[0],
		&tri_areas[0], ntris, *heightfield, config.walkableClimb
	);
	rcFilterLowHangingWalkableObstacles(
		ctx, config.walkableClimb, *heightfield
	);

	rcFilterLedgeSpans(
		ctx, config.walkableHeight, config.walkableClimb, *heightfield
	);
	rcFilterWalkableLowHeightSpans(
		ctx, config.walkableHeight, *heightfield
	);

	if (!compact_heightfield) {
		ERR_PRINT("Failed to allocate compact height field.");
		return false;
	}
	if (!rcBuildCompactHeightfield(ctx, config.walkableHeight, config.walkableClimb,
		*heightfield, *compact_heightfield)) {
		ERR_PRINT("Could not build compact height field.");
		return false;
	}
	if (!rcErodeWalkableArea(ctx, config.walkableRadius, *compact_heightfield)) {
		ERR_PRINT("Could not erode walkable area.");
		return false;
	}
	if (navmesh_parameters->get_partition_type() == 0) { // TODO: Use enum
		if (!rcBuildDistanceField(ctx, *compact_heightfield))
			return false;
		if (!rcBuildRegions(
			ctx, *compact_heightfield, config.borderSize,
			config.minRegionArea, config.mergeRegionArea)
			) {
			return false;
		}
	}
	else if (!rcBuildRegionsMonotone(
		ctx, *compact_heightfield, config.borderSize,
		config.minRegionArea, config.mergeRegionArea)
		) {
		return false;
	}
	return true;
}

bool DetourNavigationMeshGenerator::build_tile(int x, int z) {
	Vector3 bmin, bmax;
	get_tile_bounding_box(x, z, bmin, bmax);
	dtNavMesh* nav = get_detour_navmesh();

	nav->removeTile(nav->getTileRefAt(x, z, 0), NULL, NULL);

	rcConfig config;
	init_rc_config(config, bmin, bmax);

	std::vector<float> points;
	std::vector<int> indices;
	if (init_tile_data(config, bmin, bmax, points, indices)) {
		return true;
	}

	rcCompactHeightfield* compact_heightfield = rcAllocCompactHeightfield();
	rcContext* ctx = new rcContext(true);

	if (!init_heightfield_context(config, compact_heightfield, ctx, points, indices)) {
		return false;
	}

	// Create ContourSet 
	rcContourSet* contour_set = rcAllocContourSet();
	if (!contour_set) {
		ERR_PRINT("Could not allocate contour set.");
		return false;
	}

	if (!rcBuildContours(
		ctx, *compact_heightfield, config.maxSimplificationError,
		config.maxEdgeLen, *contour_set)
		) {
		ERR_PRINT("Could not build contour set.");
		return false;
	}

	rcPolyMesh* poly_mesh = rcAllocPolyMesh();
	if (!poly_mesh) {
		ERR_PRINT("Could not allocate poli mesh.");
		return false;
	}

	if (!rcBuildPolyMesh(ctx, *contour_set, config.maxVertsPerPoly, *poly_mesh)) {
		ERR_PRINT("Could not build poli mesh.");
		return false;
	}

	rcPolyMeshDetail* poly_mesh_detail = rcAllocPolyMeshDetail();
	if (!poly_mesh_detail) {
		ERR_PRINT("Could not alloc polimesh detail.");
		return false;
	}

	if (!rcBuildPolyMeshDetail(
		ctx, *poly_mesh, *compact_heightfield,
		config.detailSampleDist, config.detailSampleMaxError,
		*poly_mesh_detail)
		) {
		ERR_PRINT("Could not build polimesh detail.");
		return false;
	}

	for (int i = 0; i < poly_mesh->npolys; i++) {
		if (poly_mesh->areas[i] != RC_NULL_AREA) {
			poly_mesh->flags[i] = 0x1;
		}

	}

	unsigned char* nav_data = NULL;
	int nav_data_size = 0;
	dtNavMeshCreateParams params;
	memset(&params, 0, sizeof params);
	params.verts = poly_mesh->verts;
	params.vertCount = poly_mesh->nverts;
	params.polys = poly_mesh->polys;
	params.polyAreas = poly_mesh->areas;
	params.polyFlags = poly_mesh->flags;
	params.polyCount = poly_mesh->npolys;
	params.nvp = poly_mesh->nvp;
	params.detailMeshes = poly_mesh_detail->meshes;
	params.detailVerts = poly_mesh_detail->verts;
	params.detailVertsCount = poly_mesh_detail->nverts;
	params.detailTris = poly_mesh_detail->tris;
	params.detailTriCount = poly_mesh_detail->ntris;
	params.walkableHeight = navmesh_parameters->get_agent_height();
	params.walkableRadius = navmesh_parameters->get_agent_radius();
	params.walkableClimb = navmesh_parameters->get_agent_max_climb();
	params.tileX = x;
	params.tileY = z;
	rcVcopy(params.bmin, poly_mesh->bmin);
	rcVcopy(params.bmax, poly_mesh->bmax);
	params.cs = config.cs;
	params.ch = config.ch;
	params.buildBvTree = true;


	if (!dtCreateNavMeshData(&params, &nav_data, &nav_data_size)) {
		ERR_PRINT("Could not create navmesh data.");
		return false;
	}

	if (dtStatusFailed(get_detour_navmesh()->addTile(
		nav_data, nav_data_size, DT_TILE_FREE_DATA, 0, NULL))) {
		dtFree(nav_data);
		ERR_PRINT("Failed to instantiate navdata");
		return false;
	}

	return true;
}

void DetourNavigationMeshGenerator::get_tile_bounding_box(
	int x, int z, Vector3& bmin, Vector3& bmax
) {
	const float tile_edge_length = (float)navmesh_parameters->get_tile_size() * navmesh_parameters->get_cell_size();
	bmin = bounding_box.position + Vector3(tile_edge_length * (float)x, 0, tile_edge_length * (float)z);
	bmax = bmin + Vector3(tile_edge_length, bounding_box.size.y, tile_edge_length);
}

void DetourNavigationMeshGenerator::add_meshdata(
	int mesh_index, std::vector<float>& p_verticies, std::vector<int>& p_indices
) {
	Transform* p_xform = &(input_transforms->at(mesh_index));
	Ref<ArrayMesh> p_mesh = input_meshes->at(mesh_index);
	int current_vertex_count = 0;

	for (int i = 0; i < p_mesh->get_surface_count(); i++) {
		current_vertex_count = p_verticies.size() / 3;


		if (p_mesh->surface_get_primitive_type(i) != Mesh::PRIMITIVE_TRIANGLES)
			continue;

		int index_count = 0;
		if (p_mesh->surface_get_format(i) & Mesh::ARRAY_FORMAT_INDEX) {
			index_count = p_mesh->surface_get_array_index_len(i);
		}
		else {
			index_count = p_mesh->surface_get_array_len(i);
		}

		ERR_CONTINUE((index_count == 0 || (index_count % 3) != 0));

		int face_count = index_count / 3;

		PoolVector3Array mesh_vertices(p_mesh->surface_get_arrays(i)[Mesh::ARRAY_VERTEX]);
		if (p_mesh->surface_get_format(i) & Mesh::ARRAY_FORMAT_INDEX) {
			PoolIntArray mesh_indices = p_mesh->surface_get_arrays(i)[Mesh::ARRAY_INDEX];
			PoolIntArray::Read ir = mesh_indices.read();

			for (int j = 0; j < mesh_vertices.size(); j++) {
				Vector3 p_vec3 = p_xform->xform(mesh_vertices[j]);
				p_verticies.push_back(p_vec3.x);
				p_verticies.push_back(p_vec3.y);
				p_verticies.push_back(p_vec3.z);
			}

			for (int j = 0; j < face_count; j++) {
				p_indices.push_back(current_vertex_count + (ir[j * 3 + 0]));
				p_indices.push_back(current_vertex_count + (ir[j * 3 + 2]));
				p_indices.push_back(current_vertex_count + (ir[j * 3 + 1]));
			}
		}
		else {
			face_count = mesh_vertices.size() / 3;
			for (int j = 0; j < face_count; j++) {
				Vector3 p_vec3 = p_xform->xform(mesh_vertices[j * 3 + 0]);
				p_verticies.push_back(p_vec3.x);
				p_verticies.push_back(p_vec3.y);
				p_verticies.push_back(p_vec3.z);
				p_vec3 = p_xform->xform(mesh_vertices[j * 3 + 2]);
				p_verticies.push_back(p_vec3.x);
				p_verticies.push_back(p_vec3.y);
				p_verticies.push_back(p_vec3.z);
				p_vec3 = p_xform->xform(mesh_vertices[j * 3 + 1]);
				p_verticies.push_back(p_vec3.x);
				p_verticies.push_back(p_vec3.y);
				p_verticies.push_back(p_vec3.z);

				p_indices.push_back(current_vertex_count + (j * 3 + 0));
				p_indices.push_back(current_vertex_count + (j * 3 + 1));
				p_indices.push_back(current_vertex_count + (j * 3 + 2));
			}
		}
	}
}

bool DetourNavigationMeshGenerator::init(dtNavMeshParams* params) {
	if (dtStatusFailed((detour_navmesh)->init(params))) {
		release_navmesh();
		return false;
	}
	set_initialized(true);
	return true;
}

void DetourNavigationMeshGenerator::release_navmesh() {
	dtFreeNavMesh((dtNavMesh*) detour_navmesh);
	detour_navmesh = NULL;
	num_tiles_x = 0;
	num_tiles_z = 0;
	bounding_box = AABB();
	Godot::print("Released navmesh");
}


bool DetourNavigationMeshGenerator::alloc() {
	/* Allocates a detour navmesh object, returns bool for succsess status */
	detour_navmesh = dtAllocNavMesh();
	return detour_navmesh ? true : false;
}