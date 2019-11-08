#include "navigation_query.h"

using namespace godot;


DetourNavigationQuery::DetourNavigationQuery(){
	query_data = new QueryData();
}

DetourNavigationQuery::~DetourNavigationQuery() {
}

DetourNavigationQueryFilter::DetourNavigationQueryFilter(){
	dt_query_filter = new dtQueryFilter();
}

DetourNavigationQueryFilter::~DetourNavigationQueryFilter() {
}

void DetourNavigationQuery::init(
	DetourNavigationMesh *mesh,
	const Transform& xform
) {
	navmesh_query = dtAllocNavMeshQuery();
	if (!navmesh_query) {
		ERR_PRINT("Failed to create navigation query.");
		return;
	}
	if (dtStatusFailed(navmesh_query->init(mesh->get_detour_navmesh(), MAX_POLYS))) {
		ERR_PRINT("Failed to initialize navigation query.");
		return;
	}
	transform = xform;
	inverse = xform.inverse();
}

Dictionary DetourNavigationQuery::find_path(
	const Vector3& start, 
	const Vector3& end,
	const Vector3& extents,
	DetourNavigationQueryFilter *filter
) {
	/* Function called by addon's api */
	Vector3 local_start = inverse.xform(start);
	Vector3 local_end = inverse.xform(end);
	Dictionary result = _find_path(local_start, local_end, extents, filter);
	
	PoolVector3Array points = result["points"];
	PoolVector3Array::Write w = points.write();

	for (int i = 0; i < points.size(); i++) {
		w[i] = transform.xform(points[i]);
	}
	result["points"] = points;
	return result;
}


Dictionary DetourNavigationQuery::_find_path(
	const Vector3& start, 
	const Vector3& end,
	const Vector3& extents,
	DetourNavigationQueryFilter *filter
) {
	/* Internal function for finding path */
	Dictionary ret;
	if (!navmesh_query) {
		return ret;
	}
		

	dtPolyRef pstart;
	dtPolyRef pend;

	navmesh_query->findNearestPoly(
		&start.coord[0], &extents.coord[0],
		filter->dt_query_filter, &pstart, NULL
	);
	int stat = navmesh_query->findNearestPoly(
		&end.coord[0], &extents.coord[0],
		filter->dt_query_filter, &pend, NULL
	);

	if (!pstart || !pend) {
		return ret;
	}
		
	int num_polys = 0;
	int num_path_points = 0;
	navmesh_query->findPath(
		pstart, pend, 
		&start.coord[0], &end.coord[0],
		filter->dt_query_filter, query_data->polys,
		&num_polys, MAX_POLYS
	);

	if (!num_polys) {
		return ret;
	}
		
	Vector3 actual_end = end;
	if (query_data->polys[num_polys - 1] != pend) {
		Vector3 tmp;
		navmesh_query->closestPointOnPoly(
			query_data->polys[num_polys - 1],
			&end.coord[0], &tmp.coord[0], NULL
		);
		actual_end = tmp;
	}

	navmesh_query->findStraightPath(
		&start.coord[0], &actual_end.coord[0], 
		query_data->polys, num_polys,
		&query_data->path_points[0].coord[0], 
		&query_data->path_flags[0], query_data->path_polys, 
		&num_path_points, MAX_POLYS
	);

	PoolVector3Array points;
	PoolIntArray flags;
	points.resize(num_path_points);
	flags.resize(num_path_points);

	PoolVector3Array::Write points_write = points.write();
	PoolIntArray::Write flags_write = flags.write();

	for (int i = 0; i < num_path_points; i++) {
		points_write[i] = query_data->path_points[i];
		flags_write[i] = query_data->path_flags[i];
	}

	ret["points"] = points;
	ret["flags"] = flags;
	return ret;
}