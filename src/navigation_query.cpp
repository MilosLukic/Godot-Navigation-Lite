#include "navigation_query.h"

using namespace godot;

DetourNavigationQuery::DetourNavigationQuery()
{
	query_data = new QueryData();
}

DetourNavigationQuery::~DetourNavigationQuery()
{
	dtFreeNavMeshQuery(navmesh_query);
}

DetourNavigationQueryFilter::DetourNavigationQueryFilter()
{
	dt_query_filter = new dtQueryFilter();
}

DetourNavigationQueryFilter::~DetourNavigationQueryFilter()
{
	delete dt_query_filter;
}

void DetourNavigationQuery::init(
	dtNavMesh *dtMesh,
	const Transform &xform)
{
	navmesh_query = dtAllocNavMeshQuery();
	if (!navmesh_query)
	{
		ERR_PRINT("Failed to create navigation query.");
		return;
	}
	detour_navmesh = dtMesh;
	if (dtStatusFailed(navmesh_query->init(dtMesh, MAX_POLYS)))
	{
		ERR_PRINT("Failed to initialize navigation query.");
		return;
	}
	transform = xform;
	inverse = xform.inverse();
}

Dictionary DetourNavigationQuery::find_path(
	const Vector3 &start,
	const Vector3 &end,
	const Vector3 &extents,
	DetourNavigationQueryFilter *filter)
{
	/* Function called by addon's api */
	Vector3 local_start = inverse.xform(start);
	Vector3 local_end = inverse.xform(end);
	Dictionary result = _find_path(local_start, local_end, extents, filter);

	PoolVector3Array points = result["points"];
	PoolVector3Array::Write w = points.write();

	for (int i = 0; i < points.size(); i++)
	{
		w[i] = transform.xform(points[i]);
	}
	return result;
}

Dictionary DetourNavigationQuery::_find_path(
	const Vector3 &start,
	const Vector3 &end,
	const Vector3 &extents,
	DetourNavigationQueryFilter *filter)
{
	/* Internal function for finding path */
	Dictionary ret;
	ret["points"] = Array();
	ret["flags"] = Array();

	if (!navmesh_query || detour_navmesh == nullptr)
	{
		return ret;
	}

	dtStatus status;
	dtPolyRef StartPoly;
	float StartNearest[3];
	dtPolyRef EndPoly;
	float EndNearest[3];
	dtPolyRef PolyPath[MAX_POLYS];
	int nPathCount = 0;
	float StraightPath[MAX_POLYS * 2 * 3];
	int nVertCount = 0;

	// find the start polygon
	status = navmesh_query->findNearestPoly(&start.coord[0], &extents.coord[0], filter->dt_query_filter, &StartPoly, StartNearest);
	if ((status & DT_FAILURE) || (status & DT_STATUS_DETAIL_MASK))
		return ret; // couldn't find a polygon

	// find the end polygon
	status = navmesh_query->findNearestPoly(&end.coord[0], &extents.coord[0], filter->dt_query_filter, &EndPoly, EndNearest);
	if ((status & DT_FAILURE) || (status & DT_STATUS_DETAIL_MASK))
		return ret; // couldn't find a polygon

	status = navmesh_query->findPath(StartPoly, EndPoly, StartNearest, EndNearest, filter->dt_query_filter, PolyPath, &nPathCount, MAX_POLYS);
	if ((status & DT_FAILURE) || (status & DT_STATUS_DETAIL_MASK))
		return ret; // couldn't create a path
	if (nPathCount == 0)
		return ret; // couldn't find a path

	status = navmesh_query->findStraightPath(StartNearest, EndNearest, PolyPath, nPathCount, StraightPath, NULL, NULL, &nVertCount, MAX_POLYS * 2 * 3, DT_STRAIGHTPATH_ALL_CROSSINGS);
	if ((status & DT_FAILURE) || (status & DT_STATUS_DETAIL_MASK))
		return ret; // couldn't create a path
	if (nVertCount == 0)
		return ret; // couldn't find a path

	PoolVector3Array points;
	PoolIntArray flags;
	points.resize(nVertCount);
	flags.resize(nVertCount);

	PoolVector3Array::Write points_write = points.write();
	PoolIntArray::Write flags_write = flags.write();

	int nIndex = 0;
	for (int nVert = 0; nVert < nVertCount; nVert++)
	{
		points_write[nVert] = Vector3(StraightPath[nIndex], StraightPath[nIndex + 1], StraightPath[nIndex + 2]);
		nIndex += 3;
	}

	ret["points"] = points;
	ret["flags"] = flags;
	return ret;
}
