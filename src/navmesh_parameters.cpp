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

using namespace godot;


void NavmeshParameters::_register_methods() {
	register_property<NavmeshParameters, real_t>("cell_size", &(NavmeshParameters::set_cell_size), &(NavmeshParameters::get_cell_size), 32.0);
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