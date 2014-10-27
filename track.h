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

struct track_node_bezier {
	int32_t prev;
	int32_t next;
	struct track_point p[2];
	uint32_t flags;
};

struct track_node {
	enum track_node_type {
		TRACK_DELETED = 0,
		TRACK_BEZIER
	} type;

	union {
		struct track_node_bezier bezier;
	};
};

#define TRACK_NODE_MAX (1<<14)

struct track {
	struct track_node nodes[TRACK_NODE_MAX];
	int node_count;
};

struct track_node* track_get_node(struct track* track, int index);

void track_init_demo(struct track* track);

int track_node_bezier_derive_4_track_points(struct track* track, struct track_node_bezier* bezier, struct track_point* points);

void track_points_construct_block(struct track_point* tps, int i, int N, struct vec3* points, struct vec3* normals);

#endif/*TRACK_H*/
