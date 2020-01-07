#ifndef NAVIGATION_QUERY_H
#define NAVIGATION_QUERY_H
#include <DetourNavMeshQuery.h>
#include <Godot.hpp>
#include <Dictionary.hpp>


namespace godot {

	class DetourNavigationQueryFilter {
	public:
		dtQueryFilter* dt_query_filter;
		DetourNavigationQueryFilter();
		~DetourNavigationQueryFilter();
	};

	class DetourNavigationQuery {
		dtNavMeshQuery* navmesh_query;
		godot::Transform transform;
		godot::Transform inverse;
	protected:
		static const int MAX_POLYS = 2048;

		Dictionary _find_path(const Vector3& start, const Vector3& end, const Vector3& extents, DetourNavigationQueryFilter *filter);
	public:
		class QueryData {
		public:
			Vector3 path_points[MAX_POLYS];
			unsigned char path_flags[MAX_POLYS];
			dtPolyRef polys[MAX_POLYS];
			dtPolyRef path_polys[MAX_POLYS];
		};
		QueryData* query_data = nullptr;

		DetourNavigationQuery();
		~DetourNavigationQuery();

		void init(dtNavMesh *dtMesh, const Transform& xform);

		int get_max_polys() const { return MAX_POLYS; }
		Dictionary find_path(const Vector3& start, const Vector3& end, const Vector3& extents, DetourNavigationQueryFilter *filter);
	};
}
#endif
