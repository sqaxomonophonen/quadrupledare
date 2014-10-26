#include <string.h>
#include <stdio.h>

#include "track.h"
#include "a.h"
#include "m.h"

struct track_node* track_get_node(struct track* track, int index)
{
	ASSERT(index >= 0 && index < track->node_count);
	return &track->nodes[index];
}

void track_init_demo(struct track* track)
{
	track->node_count = 4;

	struct vec3 normal = {{0,1,0}};

	struct vec3 ps[4] = {
		{{-10,0,-10}},
		{{10,0,-10}},
		{{10,5,10}},
		{{-10,0,10}},
	};

	for (int i = 0; i < 4; i++) {
		struct track_node* node = &track->nodes[i];
		node->type = TRACK_BEZIER;
		struct track_node_bezier* bezier = &node->bezier;
		bezier->prev = (i-1)&3;
		bezier->next = (i+1)&3;
		vec3_copy(&bezier->p[0].position, &ps[i]);
		for (int j = 0; j < 3; j++) {
			struct track_point* p = &bezier->p[j];
			p->width = 2;
			vec3_copy(&p->normal, &normal);
		}
	}

	for (int i = 0; i < 4; i++) {
		float clen = 0.6f;
		{
			struct vec3 a;
			vec3_sub(&a, &track->nodes[(i+1)&3].bezier.p[0].position, &track->nodes[i].bezier.p[0].position);
			struct vec3 b;
			vec3_sub(&b, &track->nodes[i].bezier.p[0].position, &track->nodes[(i-1)&3].bezier.p[0].position);
			struct vec3 c;
			vec3_lerp(&c, &a, &b, 0.5f);
			vec3_scale_inplace(&c, clen);
			vec3_add_inplace(&c, &track->nodes[i].bezier.p[0].position);
			vec3_copy(&track->nodes[i].bezier.p[1].position, &c);
		}
		{
			struct vec3 a;
			vec3_sub(&a, &track->nodes[(i+2)&3].bezier.p[0].position, &track->nodes[(i+1)&3].bezier.p[0].position);
			struct vec3 b;
			vec3_sub(&b, &track->nodes[(i+1)&3].bezier.p[0].position, &track->nodes[i].bezier.p[0].position);
			struct vec3 c;
			vec3_lerp(&c, &a, &b, 0.5f);
			vec3_scale_inplace(&c, -clen);
			vec3_add_inplace(&c, &track->nodes[(i+1)&3].bezier.p[0].position);
			vec3_copy(&track->nodes[i].bezier.p[2].position, &c);
		}
	}
}

struct track_point* track_point_get_first(struct track_node* node)
{
	switch (node->type) {
		case TRACK_BEZIER: return &node->bezier.p[0];
		case TRACK_STUMP: return &node->stump.p;
		case TRACK_GAP: return &node->gap.p;
		default: arghf("cannot get p0 from node type %d\n", node->type);
	}
}

void track_point_copy_first(struct track* track, struct track_point* p0, int index)
{
	memcpy(p0, track_point_get_first(track_get_node(track, index)), sizeof(struct track_point));
}

