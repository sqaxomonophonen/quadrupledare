#ifndef TRACK_H
#define TRACK_H

#include <stdint.h>

#include "m.h"

#define TRACK_POINT_HOVER (1<<0)
#define TRACK_POINT_SELECTED (1<<1)

struct track_point {
	struct vec3 position;
	struct vec3 normal;
	float width;
	int flags; // mode for normal? e.g. "assume up", or "rotate" or "full 3d control". also, tangent linkage? (links ctrl points). mouse over / selected too?
};

struct track_node_stump {
	int32_t prev;
	struct track_point p;
};

struct track_node_bezier {
	int32_t prev;
	int32_t next;
	struct track_point p[3];
};

struct track_node_gap {
	int32_t prev;
	int32_t next;
	struct track_point p;
};

struct track_node {
	enum track_node_type {
		TRACK_NONE = 0,
		TRACK_STUMP,
		TRACK_BEZIER,
		TRACK_GAP
	} type;

	union {
		struct track_node_stump stump;
		struct track_node_bezier bezier;
		struct track_node_gap gap;
	};

	int frame_tag;
};

#define TRACK_NODE_MAX (1<<14)

struct track {
	struct track_node nodes[TRACK_NODE_MAX];
	int node_count;
};

struct track_node* track_get_node(struct track* track, int index);
//void track_get_points(struct track* track, struct track_point* points, int node_index);


void track_init_demo(struct track* track);


struct track_point* track_point_get_first(struct track_node* node);
void track_point_copy_first(struct track* track, struct track_point* p0, int index);
#endif/*TRACK_H*/
