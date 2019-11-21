#include "navigation_mesh.h"

static const int DEFAULT_TILE_SIZE = 64;
static const float DEFAULT_CELL_SIZE = 0.3f;
static const float DEFAULT_CELL_HEIGHT = 0.2f;
static const float DEFAULT_AGENT_HEIGHT = 2.0f;
static const float DEFAULT_AGENT_RADIUS = 0.6f;
static const float DEFAULT_AGENT_MAX_CLIMB = 0.9f;
static const float DEFAULT_AGENT_MAX_SLOPE = 45.0f;
static const float DEFAULT_REGION_MIN_SIZE = 8.0f;
static const float DEFAULT_REGION_MERGE_SIZE = 20.0f;
static const float DEFAULT_EDGE_MAX_LENGTH = 12.0f;
static const float DEFAULT_EDGE_MAX_ERROR = 1.3f;
static const float DEFAULT_DETAIL_SAMPLE_DISTANCE = 6.0f;
static const float DEFAULT_DETAIL_SAMPLE_MAX_ERROR = 1.0f;

using namespace godot;


DetourNavigationMesh::DetourNavigationMesh(){
	bounding_box = godot::AABB();
	tile_size = DEFAULT_TILE_SIZE;
	cell_size = DEFAULT_CELL_SIZE;
	cell_height = DEFAULT_CELL_HEIGHT;

	agent_height = DEFAULT_AGENT_HEIGHT;
	agent_radius = DEFAULT_AGENT_RADIUS;
	agent_max_climb = DEFAULT_AGENT_MAX_CLIMB;
	agent_max_slope = DEFAULT_AGENT_MAX_SLOPE;

	region_min_size = DEFAULT_REGION_MIN_SIZE;
	region_merge_size = DEFAULT_REGION_MERGE_SIZE;
	edge_max_length = DEFAULT_EDGE_MAX_LENGTH;
	edge_max_error = DEFAULT_EDGE_MAX_ERROR;
	detail_sample_distance = DEFAULT_DETAIL_SAMPLE_DISTANCE;
	detail_sample_max_error = DEFAULT_DETAIL_SAMPLE_MAX_ERROR;
	padding = Vector3(1.f, 1.f, 1.f);
}

DetourNavigationMesh::~DetourNavigationMesh() {
	for (Ref<ArrayMesh> m : input_meshes) {
		m.unref();
	}
	
	clear_debug_mesh();
	release_navmesh();
}

bool DetourNavigationMesh::alloc() {
	/* Allocates a detour navmesh object, returns bool for succsess status */
	detour_navmesh = dtAllocNavMesh();
	return detour_navmesh ? true : false;
}

bool DetourNavigationMesh::init(dtNavMeshParams* params) {
	if (dtStatusFailed((detour_navmesh)->init(params))) {
		release_navmesh();
		return false;
	}
	set_initialized(true);
	return true;
}

void DetourNavigationMesh::release_navmesh() {
	dtFreeNavMesh((dtNavMesh*) detour_navmesh);
	detour_navmesh = NULL;
	num_tiles_x = 0;
	num_tiles_z = 0;
	bounding_box = AABB();
	Godot::print("Released navmesh");
}

unsigned int DetourNavigationMesh::build_tiles(
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

bool DetourNavigationMesh::build_tile(int x, int z) {
	Vector3 bmin, bmax;
	get_tile_bounding_box(x, z, bmin, bmax);
	dtNavMesh* nav = get_detour_navmesh();

	nav->removeTile(nav->getTileRefAt(x, z, 0), NULL, NULL);
	rcConfig config;
	config.cs = cell_size;
	config.ch = cell_height;
	config.walkableSlopeAngle = agent_max_slope;
	config.walkableHeight = (int) ceil(agent_height / config.ch);
	config.walkableClimb = (int) floor(agent_max_climb / config.ch);
	config.walkableRadius = (int) ceil(agent_radius / config.cs);
	config.maxEdgeLen = (int) (edge_max_length / config.cs);
	config.maxSimplificationError = edge_max_error;
	config.minRegionArea = (int) sqrtf(region_min_size);
	config.mergeRegionArea = (int) sqrtf(region_merge_size);
	config.maxVertsPerPoly = 6;
	config.tileSize = tile_size;
	config.borderSize = config.walkableRadius + 3;
	config.width = config.tileSize + config.borderSize * 2;
	config.height = config.tileSize + config.borderSize * 2;
	config.detailSampleDist =
		detail_sample_distance < 0.9f ? 0.0f : cell_size * detail_sample_distance;
	config.detailSampleMaxError = cell_height * detail_sample_max_error;
	rcVcopy(config.bmin, &bmin.coord[0]);
	rcVcopy(config.bmax, &bmax.coord[0]);

	config.bmin[0] -= config.borderSize * config.cs;
	config.bmin[2] -= config.borderSize * config.cs;
	config.bmax[0] += config.borderSize * config.cs;
	config.bmax[2] += config.borderSize * config.cs;

	/* Set the tile AABB */
	AABB expbox(bmin, bmax - bmin);
	expbox.position.x -= config.borderSize * config.cs;
	expbox.position.z -= config.borderSize * config.cs;
	expbox.size.x += 2.0 * config.borderSize * config.cs;
	expbox.size.z += 2.0 * config.borderSize * config.cs;

	std::vector<float> points;
	std::vector<int> indices;

	Transform base = global_transform.inverse();


	for (int i = 0; i < input_meshes.size(); i++) {
		if (!input_meshes[i].is_valid()) {
			continue;
		}
		Transform mxform = base * input_transforms[i];
		AABB mesh_aabb = mxform.xform(input_aabbs[i]);
		if (!mesh_aabb.intersects_inclusive(expbox) &&
			!expbox.encloses(mesh_aabb)) {
			continue;
		}
		add_meshdata(i, points, indices);
	}

	if (points.size() == 0 || indices.size() == 0) {
		Godot::print("No mesh points and indices found.");
		return true;
	}

	// Create heightfield
	rcHeightfield* heightfield = rcAllocHeightfield();
	if (!heightfield) {
		ERR_PRINT("Failed to allocate height field");
		return false;
	}
	rcContext* ctx = new rcContext(true);
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

	rcCompactHeightfield* compact_heightfield = rcAllocCompactHeightfield();
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
	if (partition_type == 0) { // TODO: Use enum
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

	/* Create ContourSet */
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
		
	/* Assign area flags TODO: use nav area assignment here */
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
	params.walkableHeight = get_agent_height();
	params.walkableRadius = get_agent_radius();
	params.walkableClimb = get_agent_max_climb();
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

void DetourNavigationMesh::get_tile_bounding_box(
	int x, int z, Vector3& bmin, Vector3& bmax
) {
	const float tile_edge_length = (float) tile_size * cell_size;
	bmin = bounding_box.position + Vector3(tile_edge_length * (float) x, 0, tile_edge_length * (float) z);
	bmax = bmin + Vector3(tile_edge_length, bounding_box.size.y, tile_edge_length);
}

void DetourNavigationMesh::add_meshdata(
	int mesh_index, std::vector<float>& p_verticies, std::vector<int>& p_indices
) {
	Transform* p_xform = &(input_transforms[mesh_index]);
	Ref<ArrayMesh> p_mesh = input_meshes[mesh_index];
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

		Array a = p_mesh->surface_get_arrays(i);
		PoolVector3Array mesh_vertices(a[Mesh::ARRAY_VERTEX]);

		if (p_mesh->surface_get_format(i) & Mesh::ARRAY_FORMAT_INDEX) {

			PoolIntArray mesh_indices = a[Mesh::ARRAY_INDEX];
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
		} else {
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

	PoolVector3Array varr;
	varr.resize(lines.size());
	PoolVector3Array::Write w = varr.write();
	int idx = 0;
	for (Vector3 vec : lines) {
		w[idx++] = vec;
	}

	debug_mesh = Ref<ArrayMesh>(ArrayMesh::_new());

	Array arr;
	arr.resize(Mesh::ARRAY_MAX);
	arr[Mesh::ARRAY_VERTEX] = varr;

	debug_mesh->add_surface_from_arrays(Mesh::PRIMITIVE_LINES, arr);
	return debug_mesh;
}