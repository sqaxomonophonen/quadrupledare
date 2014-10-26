#include <GL/glew.h>

#include "render.h"
#include "a.h"
#include "m.h"

static const char* road_shader_vertex_src =
	"#version 130\n"
	"uniform mat4 u_projection;\n"
	"uniform mat4 u_view;\n"
	"\n"
	"attribute vec3 a_position;\n"
	"attribute vec3 a_normal;\n"
	"attribute float a_material;\n"
	"\n"
	"varying vec3 v_position;\n"
	"varying vec3 v_normal;\n"
	"varying float v_material;\n"
	"\n"
	"void main()\n"
	"{\n"
	"	v_position = a_position;\n"
	"	v_normal = a_normal;\n"
	"	v_material = a_material;\n"
	"	gl_Position = u_projection * u_view * vec4(a_position, 1);\n"
	"}\n";

static const char* road_shader_fragment_src =
	"#version 130\n"
	"\n"
	"varying vec3 v_position;\n"
	"varying vec3 v_normal;\n"
	"varying float v_material;\n"
	"\n"
	"void main(void)\n"
	"{\n"
	"	float l = abs(dot(v_normal, vec3(1,1,1)));\n"
	"	if (v_material < 1.0) {\n"
	"		gl_FragColor = vec4(l,l,l,0) * 0.5 + vec4(0,0.2,0.4,1);\n"
	"	} else {\n"
	"		gl_FragColor = vec4(l,l,l,0) * 0.1 + vec4(0.2,0.1,0.0,1);\n"
	"	}\n"
	"}\n";


static const char* handle_shader_vertex_src =
	"#version 130\n"
	"uniform mat4 u_projection;\n"
	"uniform mat4 u_view;\n"
	"\n"
	"attribute vec3 a_position;\n"
	"attribute vec4 a_color;\n"
	"\n"
	"varying vec4 v_color;\n"
	"\n"
	"void main()\n"
	"{\n"
	"	v_color = a_color;\n"
	"	gl_Position = u_projection * u_view * vec4(a_position, 1);\n"
	"}\n";

static const char* handle_shader_fragment_src =
	"#version 130\n"
	"\n"
	"varying vec4 v_color;\n"
	"\n"
	"void main(void)\n"
	"{\n"
	"	gl_FragColor = v_color;\n"
	"}\n";



static float render_get_fovy(struct render* render)
{
	AN(render);
	return 65;
}


void render_init(struct render* render, SDL_Window* window)
{
	AN(render); AN(window);

	memset(render, 0, sizeof(struct render));

	render->window = window;

	// set projection matrix
	int width, height;
	SDL_GetWindowSize(render->window, &width, &height);
	float fovy = render_get_fovy(render);
	float aspect = (float)width / (float)height;
	mat44_set_perspective(&render->projection, fovy, aspect, 0.1, 4096);

	// set view matrix
	mat44_set_identity(&render->view);

	dbuf_init(&render->dbuf, 1<<16, 1<<14);

	// road dtype
	static struct dtype_attr_spec road_specs[] = {
		{"a_position", 3},
		{"a_normal", 3},
		{"a_material", 1},
		{NULL, -1}
	};
	dtype_init(
		&render->road_dtype,
		&render->dbuf,
		road_shader_vertex_src,
		road_shader_fragment_src,
		road_specs
	);

	// handle dtype
	static struct dtype_attr_spec handle_specs[] = {
		{"a_position", 3},
		{"a_color", 4},
		{NULL, -1}
	};
	dtype_init(
		&render->handle_dtype,
		&render->dbuf,
		handle_shader_vertex_src,
		handle_shader_fragment_src,
		handle_specs
	);
}

static void gl_viewport_from_sdl_window(SDL_Window* window)
{
	int w, h;
	SDL_GetWindowSize(window, &w, &h);
	glViewport(0, 0, w, h); CHKGL;
}

static void _road_add_vertex(struct render* render, struct vec3* position, struct vec3* normal, float material)
{
	int seq = 0;
	for (int i = 0; i < 3; i++) {
		dtype_add_vertex_float(&render->road_dtype, position->s[i], seq++);
	}
	for (int i = 0; i < 3; i++) {
		dtype_add_vertex_float(&render->road_dtype, normal->s[i], seq++);
	}
	dtype_add_vertex_float(&render->road_dtype, material, seq++);
}

static void _handle_add_vertex(struct render* render, struct vec3* position, struct vec4* color)
{
	int seq = 0;
	for (int i = 0; i < 3; i++) {
		dtype_add_vertex_float(&render->handle_dtype, position->s[i], seq++);
	}
	for (int i = 0; i < 4; i++) {
		dtype_add_vertex_float(&render->handle_dtype, color->s[i], seq++);
	}
}

static int render_road_node_bezier(struct render* render, struct track* track, int node_index)
{
	//printf("HARR %d\n", node_index);
	struct track_node_bezier* bz = &track_get_node(track, node_index)->bezier;

	struct track_point tp[4];
	for (int i = 0; i < 3; i++) {
		memcpy(&tp[i], &bz->p[i], sizeof(struct track_point));
	}
	track_point_copy_first(track, &tp[3], bz->next);
	//for (int i = 0; i < 4; i++) {
		//printf("tppp %f %f %f\n", tp[i].position.s[0], tp[i].position.s[1], tp[i].position.s[2]);
	//}

	int N = 50;
	for (int i = 0; i < N; i++) {
		//printf("%f %f %f\n", p0.s[0], p0.s[1], p0.s[2]);
		float t0 = (float)i / (float)N;
		struct vec3 p0;
		vec3_bezier(&p0, t0, &tp[0].position, &tp[1].position, &tp[2].position, &tp[3].position);
		#if 0
		for (int x = 0; x < 4; x++) {
			printf(" tp[%d] ", x);
			for (int y = 0; y < 3; y++) {
				printf("%f ", tp[x].position.s[y]);
			}
			printf("\n");
		}
		#endif
		struct vec3 d0;
		vec3_bezier_deriv(&d0, t0, &tp[0].position, &tp[1].position, &tp[2].position, &tp[3].position);
		struct vec3 n0;
		vec3_bezier(&n0, t0, &tp[0].normal, &tp[1].normal, &tp[2].normal, &tp[3].normal);
		struct vec3 r0;
		vec3_cross(&r0, &d0, &n0);
		vec3_normalize_inplace(&r0);
		vec3_cross(&n0, &r0, &d0);
		vec3_normalize_inplace(&n0);
		struct vec3 p0a, p0b;
		float w0 = bezier(t0, tp[0].width, tp[1].width, tp[2].width, tp[3].width);
		vec3_copy(&p0a, &p0);
		vec3_add_scaled_inplace(&p0a, &r0, -w0);
		vec3_copy(&p0b, &p0);
		vec3_add_scaled_inplace(&p0b, &r0, w0);


		float t1 = (float)(i+1) / (float)N;
		struct vec3 p1;
		vec3_bezier(&p1, t1, &tp[0].position, &tp[1].position, &tp[2].position, &tp[3].position);
		struct vec3 d1;
		vec3_bezier_deriv(&d1, t1, &tp[0].position, &tp[1].position, &tp[2].position, &tp[3].position);
		struct vec3 n1;
		vec3_bezier(&n1, t1, &tp[0].normal, &tp[1].normal, &tp[2].normal, &tp[3].normal);
		struct vec3 r1;
		vec3_cross(&r1, &d1, &n1);
		vec3_normalize_inplace(&r1);
		vec3_cross(&n1, &r1, &d1);
		vec3_normalize_inplace(&n1);
		struct vec3 p1a, p1b;
		float w1 = bezier(t1, tp[0].width, tp[1].width, tp[2].width, tp[3].width);
		vec3_copy(&p1a, &p1);
		vec3_add_scaled_inplace(&p1a, &r1, -w1);
		vec3_copy(&p1b, &p1);
		vec3_add_scaled_inplace(&p1b, &r1, w1);

		dtype_new_quad(&render->road_dtype);
		float m = 0.5f;
		//printf("hurr\n");
		_road_add_vertex(render, &p1a, &n1, m);
		_road_add_vertex(render, &p0a, &n0, m);
		_road_add_vertex(render, &p0b, &n0, m);
		_road_add_vertex(render, &p1b, &n1, m);

		m = 1.5f;
		dtype_new_quad(&render->road_dtype);

		struct vec3 rn0, rn1;
		vec3_copy(&rn0, &r0);
		rn0.s[1] = 0;
		vec3_normalize_inplace(&rn0);

		vec3_copy(&rn1, &r1);
		rn1.s[1] = 0;
		vec3_normalize_inplace(&rn1);

		struct vec3 p0az, p1az, p0bz, p1bz;

		vec3_copy(&p0az, &p0a);
		p0az.s[1] = 0;
		vec3_copy(&p0bz, &p0b);
		p0bz.s[1] = 0;
		vec3_copy(&p1az, &p1a);
		p1az.s[1] = 0;
		vec3_copy(&p1bz, &p1b);
		p1bz.s[1] = 0;

		_road_add_vertex(render, &p0a, &rn0, m);
		_road_add_vertex(render, &p1a, &rn1, m);
		_road_add_vertex(render, &p1az, &rn1, m);
		_road_add_vertex(render, &p0az, &rn0, m);

		dtype_new_quad(&render->road_dtype);

		vec3_scale_inplace(&rn0, -1);
		vec3_scale_inplace(&rn1, -1);

		_road_add_vertex(render, &p1b, &rn1, m);
		_road_add_vertex(render, &p0b, &rn0, m);
		_road_add_vertex(render, &p0bz, &rn0, m);
		_road_add_vertex(render, &p1bz, &rn1, m);
	}

	#if 0
	{
	int n = 10;
	for (int y = -n; y < n; y++) {
		for (int x = -n; x < n; x++) {
			if ((x^y)&1) continue;
			dtype_new_quad(&render->road_dtype);

			float s = 10;
			float dx = -s * (float)x*2;
			float dy = -s * (float)y*2;
			float z = -5;
			struct vec3 ps[4] = {
				{{ s+dx,z,-s+dy}},
				{{-s+dx,z,-s+dy}},
				{{-s+dx,z, s+dy}},
				{{ s+dx,z, s+dy}},
			};
			struct vec3 normal = {{0,0,0.5}};
			for (int i = 0; i < 4; i++) {
				_road_add_vertex(render, &ps[i], &normal, 1.0f);
			}
		}
	}
	}
	#endif

	return bz->next;
}

static void render_road_nodes(struct render* render, struct track* track)
{
	int node_index = 0;
	for (;;) {
		struct track_node* node = track_get_node(track, node_index);

		if (node->frame_tag == render->frame) return;
		node->frame_tag = render->frame;

		switch (node->type) {
			case TRACK_NONE: arghf("encountered TRACK_NONE");
			case TRACK_BEZIER:
				node_index = render_road_node_bezier(render, track, node_index);
				break;
			case TRACK_STUMP:
			case TRACK_GAP:
				arghf("TODO implement ME");
				break;
		}
	}
}

static void render_road(struct render* render, struct track* track)
{
	dtype_begin(&render->road_dtype);

	dtype_set_matrix(&render->road_dtype, "u_projection", &render->projection);
	dtype_set_matrix(&render->road_dtype, "u_view", &render->view);

	render_road_nodes(render, track);

	dtype_end(&render->road_dtype);
}

void render_track(struct render* render, struct track* track)
{
	AN(render);

	render->frame++;

	gl_viewport_from_sdl_window(render->window);

	glClearColor(0,0,0,0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); CHKGL;
	glEnable(GL_DEPTH_TEST);

	// transform
	/*
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(fovy, aspect, 0.1, 4096);
	*/

	//glMatrixMode(GL_MODELVIEW);
	//glLoadIdentity();
	//glRotatef(xxx, 1, 0, 0);
	//glRotatef(xxx, 0, 1, 0);
	//glTranslatef(-render->entity_cam->position.s[0], -render->entity_cam->z, -render->entity_cam->position.s[1]);

	render_road(render, track);
}

static int _screen_bases(struct render* render, struct vec3* bx, struct vec3* by, struct vec3* bz, struct vec3* point)
{
	struct vec3 ps;
	vec3_apply_mat44(&ps, point, &render->view);

	mat44_get_bases(&render->view, bx, by, bz);
	float scale = -ps.s[2] / render->projection.s[5];
	if (bx != NULL) vec3_scale_inplace(bx, scale);
	if (by != NULL) vec3_scale_inplace(by, scale);
	if (bz != NULL) vec3_scale_inplace(bz, scale);

	return ps.s[2] < 0;
}

static void _circle_point(int i, int N, float radius, float* x, float* y)
{
	float phi = (float)(i%N) / (float)N * M_PI * 2.0f;
	*x = cosf(phi) * radius;
	*y = sinf(phi) * radius;
}

static void render_circle(struct render* render, struct vec3* pos, float radius, struct vec4* color)
{
	struct vec3 bx, by;
	if (!_screen_bases(render, &bx, &by, NULL, pos)) {
		return;
	}
	int N = 8;
	for (int i = 0; i < N; i++) {
		dtype_new_quad(&render->handle_dtype);
		float x, y;

		for (int j = 0; j < 4; j++) {
			int a = 0;
			switch (j) { case 1: case 2: a = 1; }
			float s = 1.0f;
			switch (j) { case 2: case 3: s = 0.82f; }
			_circle_point(i+a, N, radius * s, &x, &y);
			struct vec3 p;
			vec3_copy(&p, pos);
			vec3_add_scaled_inplace(&p, &bx, x);
			vec3_add_scaled_inplace(&p, &by, y);
			_handle_add_vertex(render, &p, color);
		}
	}
}

static void render_line(struct render* render, struct vec3* p0, struct vec3* p1, float width, struct vec4* color)
{
	struct vec3 b0x, b0y;
	if (!_screen_bases(render, &b0x, &b0y, NULL, p0)) {
		return;
	}

	struct vec3 b1x, b1y;
	if (!_screen_bases(render, &b1x, &b1y, NULL, p1)) {
		return;
	}

	struct mat44 tx;
	mat44_multiply(&tx, &render->projection, &render->view);

	struct vec3 p0s, p1s;
	vec3_apply_mat44(&p0s, p0, &tx);
	vec3_apply_mat44(&p1s, p1, &tx);

	float nx = p1s.s[1] - p0s.s[1];
	float ny = p0s.s[0] - p1s.s[0];
	float ns = sqrtf(nx*nx + ny*ny);
	nx /= ns;
	ny /= ns;

	dtype_new_quad(&render->handle_dtype);

	for (int i = 0; i < 4; i++) {
		float w = -width;
		switch (i) { case 1: case 2: w = width; }
		struct vec3* bx = &b0x;
		struct vec3* by = &b0y;
		switch (i) { case 2: case 3: bx = &b1x; by = &b1y; }
		struct vec3 qp;
		vec3_copy(&qp, i < 2 ? p0 : p1);
		vec3_add_scaled_inplace(&qp, bx, nx * w);
		vec3_add_scaled_inplace(&qp, by, ny * w);
		_handle_add_vertex(render, &qp, color);
	}
}

void render_track_position_handles(struct render* render, struct track* track)
{
	glClear(GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); CHKGL;

	dtype_begin(&render->handle_dtype);

	dtype_set_matrix(&render->handle_dtype, "u_projection", &render->projection);
	dtype_set_matrix(&render->handle_dtype, "u_view", &render->view);

	struct vec4 primary_color = {{1, 1, 0, 1}};
	struct vec4 secondary_color = {{0.5, 0.6, 1, 1}};
	struct vec4 line_color = {{0.3, 0.8, 0.3, 1}};
	float primary_radius = 0.04;
	float secondary_radius = 0.03;
	float line_width = 0.003;

	for (int i = 0; i < track->node_count; i++) {
		struct track_node* node = track_get_node(track, i);
		int j;
		switch (track_get_node(track, i)->type) {
			case TRACK_BEZIER:
				for (j = 0; j < 3; j++) {
					render_circle(
						render,
						&node->bezier.p[j].position,
						j == 0 ? primary_radius : secondary_radius,
						j == 0 ? &primary_color : &secondary_color
					);
				}
				render_line(
					render,
					&node->bezier.p[0].position,
					&node->bezier.p[1].position,
					line_width,
					&line_color
				);
				render_line(
					render,
					&track_point_get_first(track_get_node(track, node->bezier.next))->position,
					&node->bezier.p[2].position,
					line_width,
					&line_color
				);
				break;
			case TRACK_STUMP:
				render_circle(
					render,
					&node->stump.p.position,
					primary_radius,
					&primary_color
				);
				break;
			case TRACK_GAP:
				render_circle(
					render,
					&node->gap.p.position,
					primary_radius,
					&primary_color
				);
				break;
			case TRACK_NONE:
				arghf("unexpected TRACK_NONE");
		}

		/*
		dtype_new_triangle(&render->handle_dtype);

		struct vec3 p;
		vec3_copy(&p, &node->bezier.p[0].position);
		_handle_add_vertex(render, &p, &color);

		struct vec3 a = {{0,1,0}};
		vec3_add_inplace(&p, &a);
		_handle_add_vertex(render, &p, &color);

		struct vec3 b = {{1,0,0}};
		vec3_add_inplace(&p, &b);
		_handle_add_vertex(render, &p, &color);
		*/
	}

	dtype_end(&render->handle_dtype);

}

void render_flip(struct render* render)
{
	SDL_GL_SwapWindow(render->window);
}
