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
	struct vec3 normal2 = {{-0.3,1,-0.3}};

	struct vec3 ps[4] = {
		{{-10,5,-10}},
		{{10,4,-10}},
		{{10,15,10}},
		{{-10,7,10}},
	};

	for (int i = 0; i < 4; i++) {
		struct track_node* node = &track->nodes[i];
		node->type = TRACK_BEZIER;
		struct track_node_bezier* bezier = &node->bezier;
		bezier->prev = (i-1)&3;
		bezier->next = (i+1)&3;

		vec3_copy(&bezier->p[0].position, &ps[i]);

		bezier->p[0].width = 3;
		vec3_copy(&bezier->p[0].normal, i == 3 ? &normal2 : &normal);

		bezier->p[1].width = 3;
		vec3_copy(&bezier->p[1].normal, &normal);
	}

	for (int i = 0; i < 4; i++) {
		float clen = 0.4f;
		struct vec3 a;
		int i_next = (i+1)&3;
		int i_prev = (i-1)&3;
		vec3_sub(&a, &track->nodes[i_next].bezier.p[0].position, &track->nodes[i].bezier.p[0].position);
		struct vec3 b;
		vec3_sub(&b, &track->nodes[i].bezier.p[0].position, &track->nodes[i_prev].bezier.p[0].position);
		struct vec3 c;
		vec3_lerp(&c, &a, &b, 0.5f);
		vec3_scale_inplace(&c, clen);
		vec3_add_inplace(&c, &track->nodes[i].bezier.p[0].position);
		vec3_copy(&track->nodes[i].bezier.p[1].position, &c);
	}
}

static void track_point_calc_mirrored(struct track_point* dst, struct track_point* a, struct track_point* b)
{
	struct vec3 dposition;
	vec3_sub(&dposition, &b->position, &a->position);
	vec3_sub(&dst->position, &a->position, &dposition);

	struct vec3 dnormal;
	vec3_sub(&dnormal, &b->normal, &a->normal);
	vec3_sub(&dst->normal, &a->normal, &dnormal);

	dst->width = 2 * a->width - b->width;
}

int track_node_bezier_derive_4_track_points(struct track* track, struct track_node_bezier* bz, struct track_point* points)
{
	if (bz->next < 0) return 0;
	struct track_node* next = track_get_node(track, bz->next);
	ASSERT(next->type == TRACK_BEZIER);

	for (int i = 0; i < 2; i++) memcpy(&points[i], &bz->p[i], sizeof(struct track_point));

	struct track_node_bezier* bz_next = &next->bezier;

	struct track_point mirrored;
	track_point_calc_mirrored(&mirrored, &bz_next->p[0], &bz_next->p[1]);
	memcpy(&points[2], &mirrored, sizeof(struct track_point));

	memcpy(&points[3], &bz_next->p[0], sizeof(struct track_point));

	return 1;
}

static void _bezier_stuff(struct track_point* tps, int i, int N, struct vec3* pa, struct vec3* pb, struct vec3* n, struct vec3* r)
{
	float t = (float)i / (float)N;
	struct vec3 p;
	vec3_bezier(&p, t, &tps[0].position, &tps[1].position, &tps[2].position, &tps[3].position);
	struct vec3 d;
	vec3_bezier_deriv(&d, t, &tps[0].position, &tps[1].position, &tps[2].position, &tps[3].position);
	vec3_bezier(n, t, &tps[0].normal, &tps[1].normal, &tps[2].normal, &tps[3].normal);
	vec3_cross(r, &d, n);
	vec3_normalize_inplace(r);
	vec3_cross(n, r, &d);
	vec3_normalize_inplace(n);
	float w = calc_bezier(t, tps[0].width, tps[1].width, tps[2].width, tps[3].width);
	vec3_copy(pa, &p);
	vec3_add_scaled_inplace(pa, r, -w);
	vec3_copy(pb, &p);
	vec3_add_scaled_inplace(pb, r, w);
}

void track_points_construct_block(struct track_point* tps, int i, int N, struct vec3* points, struct vec3* normals)
{
	AN(points);

	struct vec3 n0, r0;
	_bezier_stuff(tps, i, N, &points[0], &points[1], &n0, &r0);

	struct vec3 n1, r1;
	_bezier_stuff(tps, i+1, N, &points[3], &points[2], &n1, &r1);

	for (int i = 0; i < 4; i++) {
		vec3_copy(&points[i+4], &points[i]);
		points[i+4].s[1] = 0;
	}

	if (normals != NULL) {
		vec3_copy(&normals[0], &n0);
		vec3_copy(&normals[1], &n1);

		vec3_copy(&normals[2], &r0);
		vec3_copy(&normals[3], &r1);
		for (int i = 0; i < 2; i++) {
			normals[2+i].s[1] = 0;
			vec3_normalize_inplace(&normals[2+i]);
			vec3_scale(&normals[4+i], &normals[2+i], -1);
		}
	}
}

