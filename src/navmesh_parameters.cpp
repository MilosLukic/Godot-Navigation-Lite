#include"navmesh_parameters.h"

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

static const int DEFAULT_MAX_OBSTACLES = 1000;
static const int DEFAULT_MAX_LAYERS = 8;

using namespace godot;


void NavmeshParameters::_register_methods() {
	register_property<NavmeshParameters, real_t>("cell_size", &(NavmeshParameters::set_cell_size), &(NavmeshParameters::get_cell_size), DEFAULT_CELL_SIZE);
	register_property<NavmeshParameters, int>("tile_size", &(NavmeshParameters::set_tile_size), &(NavmeshParameters::get_tile_size), DEFAULT_TILE_SIZE);
	register_property<NavmeshParameters, real_t>("cell_height", &(NavmeshParameters::set_cell_height), &(NavmeshParameters::get_cell_height), DEFAULT_CELL_HEIGHT);

	register_property<NavmeshParameters, real_t>("agent_height", &(NavmeshParameters::set_agent_height), &(NavmeshParameters::get_agent_height), DEFAULT_AGENT_HEIGHT);
	register_property<NavmeshParameters, real_t>("agent_radius", &(NavmeshParameters::set_agent_radius), &(NavmeshParameters::get_agent_radius), DEFAULT_AGENT_RADIUS);
	register_property<NavmeshParameters, real_t>("agent_max_climb", &(NavmeshParameters::set_agent_max_climb), &(NavmeshParameters::get_agent_max_climb), DEFAULT_AGENT_MAX_CLIMB);
	register_property<NavmeshParameters, real_t>("agent_max_slope", &(NavmeshParameters::set_agent_max_slope), &(NavmeshParameters::get_agent_max_slope), DEFAULT_AGENT_MAX_SLOPE);

	register_property<NavmeshParameters, real_t>("region_min_size", &(NavmeshParameters::set_region_min_size), &(NavmeshParameters::get_region_min_size), DEFAULT_REGION_MIN_SIZE);
	register_property<NavmeshParameters, real_t>("region_merge_size", &(NavmeshParameters::set_region_merge_size), &(NavmeshParameters::get_region_merge_size), DEFAULT_REGION_MERGE_SIZE);
	register_property<NavmeshParameters, real_t>("edge_max_length", &(NavmeshParameters::set_edge_max_length), &(NavmeshParameters::get_edge_max_length), DEFAULT_EDGE_MAX_LENGTH);
	register_property<NavmeshParameters, real_t>("edge_max_error", &(NavmeshParameters::set_edge_max_error), &(NavmeshParameters::get_edge_max_error), DEFAULT_EDGE_MAX_ERROR);

	register_property<NavmeshParameters, real_t>("detail_sample_distance", &(NavmeshParameters::set_detail_sample_distance), &(NavmeshParameters::get_detail_sample_distance), DEFAULT_DETAIL_SAMPLE_DISTANCE);
	register_property<NavmeshParameters, real_t>("detail_sample_max_error", &(NavmeshParameters::set_detail_sample_max_error), &(NavmeshParameters::get_detail_sample_max_error), DEFAULT_DETAIL_SAMPLE_MAX_ERROR);
	register_property<NavmeshParameters, Vector3>("padding", &(NavmeshParameters::set_padding), &(NavmeshParameters::get_padding), Vector3(1.0f, 1.0f, 1.0f));
}
void NavmeshParameters::_init() {
	Godot::print("navmehs parameters inited");
}

void NavmeshParameters::_ready() {
	Godot::print("navmehs parameters ready");
}

NavmeshParameters::NavmeshParameters() {
	Godot::print("navmehs parameters constructor called");
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

NavmeshParameters::~NavmeshParameters() {
	Godot::print("deconstruct navmesh parameters");

}

CachedNavmeshParameters::CachedNavmeshParameters() {
	max_obstacles = DEFAULT_MAX_OBSTACLES;
	max_layers = DEFAULT_MAX_LAYERS;
}

CachedNavmeshParameters::~CachedNavmeshParameters() {
}

void CachedNavmeshParameters::_init() {
	Godot::print("cached navmesh parameters inited");
}

void CachedNavmeshParameters::_ready() {
	Godot::print("cached navmesh parameters ready");
}


void CachedNavmeshParameters::_register_methods() {
	register_property<CachedNavmeshParameters, real_t>("cell_size", &(NavmeshParameters::set_cell_size), &(NavmeshParameters::get_cell_size), DEFAULT_CELL_SIZE);
	register_property<CachedNavmeshParameters, int>("tile_size", &(NavmeshParameters::set_tile_size), &(NavmeshParameters::get_tile_size), DEFAULT_TILE_SIZE);
	register_property<CachedNavmeshParameters, real_t>("cell_height", &(NavmeshParameters::set_cell_height), &(NavmeshParameters::get_cell_height), DEFAULT_CELL_HEIGHT);

	register_property<CachedNavmeshParameters, real_t>("agent_height", &(NavmeshParameters::set_agent_height), &(NavmeshParameters::get_agent_height), DEFAULT_AGENT_HEIGHT);
	register_property<CachedNavmeshParameters, real_t>("agent_radius", &(NavmeshParameters::set_agent_radius), &(NavmeshParameters::get_agent_radius), DEFAULT_AGENT_RADIUS);
	register_property<CachedNavmeshParameters, real_t>("agent_max_climb", &(NavmeshParameters::set_agent_max_climb), &(NavmeshParameters::get_agent_max_climb), DEFAULT_AGENT_MAX_CLIMB);
	register_property<CachedNavmeshParameters, real_t>("agent_max_slope", &(NavmeshParameters::set_agent_max_slope), &(NavmeshParameters::get_agent_max_slope), DEFAULT_AGENT_MAX_SLOPE);

	register_property<CachedNavmeshParameters, real_t>("region_min_size", &(NavmeshParameters::set_region_min_size), &(NavmeshParameters::get_region_min_size), DEFAULT_REGION_MIN_SIZE);
	register_property<CachedNavmeshParameters, real_t>("region_merge_size", &(NavmeshParameters::set_region_merge_size), &(NavmeshParameters::get_region_merge_size), DEFAULT_REGION_MERGE_SIZE);
	register_property<CachedNavmeshParameters, real_t>("edge_max_length", &(NavmeshParameters::set_edge_max_length), &(NavmeshParameters::get_edge_max_length), DEFAULT_EDGE_MAX_LENGTH);
	register_property<CachedNavmeshParameters, real_t>("edge_max_error", &(NavmeshParameters::set_edge_max_error), &(NavmeshParameters::get_edge_max_error), DEFAULT_EDGE_MAX_ERROR);

	register_property<CachedNavmeshParameters, real_t>("detail_sample_distance", &(NavmeshParameters::set_detail_sample_distance), &(NavmeshParameters::get_detail_sample_distance), DEFAULT_DETAIL_SAMPLE_DISTANCE);
	register_property<CachedNavmeshParameters, real_t>("detail_sample_max_error", &(NavmeshParameters::set_detail_sample_max_error), &(NavmeshParameters::get_detail_sample_max_error), DEFAULT_DETAIL_SAMPLE_MAX_ERROR);
	register_property<CachedNavmeshParameters, Vector3>("padding", &(NavmeshParameters::set_padding), &(NavmeshParameters::get_padding), Vector3(1.0f, 1.0f, 1.0f));

	register_property<CachedNavmeshParameters, int>("max_obstacles", &(CachedNavmeshParameters::set_max_obstacles), &(CachedNavmeshParameters::get_max_obstacles), DEFAULT_MAX_OBSTACLES);
	register_property<CachedNavmeshParameters, int>("max_layers", &(CachedNavmeshParameters::set_max_layers), &(CachedNavmeshParameters::get_max_layers), DEFAULT_MAX_LAYERS);

}