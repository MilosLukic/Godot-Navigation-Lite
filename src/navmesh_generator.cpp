#include "navmesh_generator.h"

using namespace godot;
DetourNavigationMeshGenerator::DetourNavigationMeshGenerator() {
}

DetourNavigationMeshGenerator::~DetourNavigationMeshGenerator() {
	if (navmesh_parameters.is_valid()){
		navmesh_parameters.unref();
	}
	if (input_meshes != nullptr){
		for (Ref<ArrayMesh> m : *input_meshes) {
			if (m.is_valid()) {
				m.unref();
			}
		}
	}
	delete input_meshes;
	delete input_transforms;
	delete input_aabbs;
}

void DetourNavigationMeshGenerator::build() {
	joint_build();

	unsigned int result = build_tiles(
		0, 0, get_num_tiles_x() - 1, get_num_tiles_z() - 1
	);
}

/**
 * Function that does mutual detour/recast logic for both
 * regular navmesh and cached navmesh
 */
void DetourNavigationMeshGenerator::joint_build() {
	for (int i = 0; i < input_meshes->size(); i++) {
		bounding_box.merge_with(
			input_transforms->at(i).xform(input_aabbs->at(i))
		);
	}
	float tile_edge_length = navmesh_parameters->get_tile_edge_length();
	Vector3 bmin = bounding_box.position;
	Vector3 bmax = bounding_box.position + bounding_box.size;

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

	
	if (OS::get_singleton()->is_stdout_verbose()) {
		std::string tile_message = "Tile number set to x:" + std::to_string(get_num_tiles_x());
		tile_message += ", z:" + std::to_string(get_num_tiles_z());
		Godot::print(tile_message.c_str());
	}

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

/**
 * Inits recast config - internal logic
 */
void DetourNavigationMeshGenerator::init_rc_config(rcConfig& config, Vector3& bmin, Vector3& bmax) {
	config.cs = navmesh_parameters->get_cell_size();
	config.ch = navmesh_parameters->get_cell_height();
	config.walkableSlopeAngle = navmesh_parameters->get_agent_max_slope();
	config.walkableHeight = (int)ceil(navmesh_parameters->get_agent_height() / config.ch);
	config.walkableClimb = (int)floor(navmesh_parameters->get_agent_max_climb() / config.ch);
	config.walkableRadius = (int)ceil(navmesh_parameters->get_agent_radius() / config.cs);

	config.maxEdgeLen = (int)(navmesh_parameters->get_edge_max_length() / config.cs);
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

/**
 * Checks which meshes intersect the tile and creates
 * triangle arrays (vertice + indices) from it
 */
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
		// Godot::print("No mesh points and indices found...");
		return true;
	}
	return false;
}

/**
 * Calls all the necessery functions on detour library to init
 * heightfield context - internal detour logic.
 */
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

/**
 * Calls all the necessery functions on detour library to build a tile
 * on specified indices
 *
 * @param x and z are the 2d coordinates of the tile to rebuild
 * @return true if it was success, false if either it was a failure
 * or there were no triangles in the area to generate mesh from.
 */
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

	rcContext* ctx = new rcContext(true);
	rcCompactHeightfield* compact_heightfield = rcAllocCompactHeightfield();

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
		if (OS::get_singleton()->is_stdout_verbose()) {
			ERR_PRINT("Could not create navmesh data.");
		}
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

/**
 * Finds the collision shape by id, and it marks the tiles it streches
 * over as dirrty. And in the end it removes it from all the data arrays
 */
void DetourNavigationMeshGenerator::remove_collision_shape(int64_t collision_id) {
	Godot::print("Removing CS...");
	int start = -1;
	int end = -1;

	for (int i = 0; i < input_meshes->size(); i++) {
		if (collision_ids->at(i) == collision_id) {
			if (start == -1) {
				start = i;
				end = i;
				mark_dirty(i, i + 1);
			}
			else {
				end = i;
				mark_dirty(i, i + 1);
			}
		}
	}
	end++;
	if (start > -1 && end > -1) {
		input_meshes->erase(input_meshes->begin() + start, input_meshes->begin() + end);
		input_transforms->erase(input_transforms->begin() + start, input_transforms->begin() + end);
		input_aabbs->erase(input_aabbs->begin() + start, input_aabbs->begin() + end);
		collision_ids->erase(collision_ids->begin() + start, collision_ids->begin() + end);
	}
}

/**
 * Inits the dirty tiles as not dirty
 */
void DetourNavigationMeshGenerator::init_dirty_tiles() {
	dirty_tiles = new int* [get_num_tiles_x()];
	for (int i = 0; i < get_num_tiles_x(); ++i) {
		dirty_tiles[i] = new int[get_num_tiles_z()];
		for (int j = 0; j < get_num_tiles_z(); ++j) {
			dirty_tiles[i][j] = 0;
		}
	}
}

/**
 * Marks tiles as dirty if they intersect with AABBs provided by index args
 *
 * @param start_index and end_index tell us from (including) which to (excluding) which index
 * the aabbs are considered
 */
void DetourNavigationMeshGenerator::mark_dirty(int start_index, int end_index) {

	Godot::print("Marking dirty");
	if (end_index == -1) {
		end_index = input_aabbs->size();
	}
	
	for (int aabb_index = start_index; aabb_index < end_index; aabb_index++) {
		AABB changes_bounding_box = input_aabbs->at(aabb_index);
		const float tile_edge_length = (float)navmesh_parameters->get_tile_size() * navmesh_parameters->get_cell_size();

		Vector3 min = (changes_bounding_box.position + input_transforms->at(aabb_index).origin - bounding_box.position) / tile_edge_length;
		Vector3 max = (
			changes_bounding_box.position + changes_bounding_box.size + input_transforms->at(aabb_index).origin  -
			(bounding_box.position)) / tile_edge_length;

		if (dirty_tiles == nullptr) {
			init_dirty_tiles();
		}
		for (int i = std::max(0, int((min.x))); i < std::min(int(std::ceil(max.x)), get_num_tiles_x()); i++) {
			for (int j = std::max(0, int(std::floor(min.z))); j < std::min( get_num_tiles_z(), int(std::ceil(max.z))); j++) {
				dirty_tiles[i][j] = 1;
			}
		}
	}
}

/**
 * Rebuilds all the tiles that were marked as dirty
 */
void DetourNavigationMeshGenerator::recalculate_tiles() {
	Godot::print("Rebuilding tiles");
	if (dirty_tiles == nullptr) {
		return;
	}
	for (int i = 0; i < get_num_tiles_x(); i++) {
		for (int j = 0; j < get_num_tiles_z(); j++) {
			if (dirty_tiles[i][j] == 1) {
				build_tile(i, j);
				dirty_tiles[i][j] = 0;
			}
		}
	}
	Godot::print("Done rebuilding.");
}

/**
 * Create triangle set from mesh retrieved by provided mesh index
 *
 * @param p_vertices and p_indices are outputs and get filled in with data
 */
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
		PoolVector3Array mesh_vertices = p_mesh->surface_get_arrays(i)[Mesh::ARRAY_VERTEX];
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
	dtFreeNavMesh((dtNavMesh*)detour_navmesh);
	detour_navmesh = NULL;
	num_tiles_x = 0;
	num_tiles_z = 0;
	bounding_box = AABB();
	Godot::print("Released navmesh");
}

/**
 * Allocates a detour navmesh object, returns bool for succsess status
 */
bool DetourNavigationMeshGenerator::alloc() {
	detour_navmesh = dtAllocNavMesh();
	return detour_navmesh ? true : false;
}