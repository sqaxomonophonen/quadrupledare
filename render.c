#include <GL/glew.h>

#include "render.h"
#include "magic.h"
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


static const char* color_shader_vertex_src =
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

static const char* color_shader_fragment_src =
	"#version 130\n"
	"\n"
	"varying vec4 v_color;\n"
	"\n"
	"void main(void)\n"
	"{\n"
	"	gl_FragColor = v_color;\n"
	"}\n";


static const char* horizon_vertex_shader_src =
	"#version 130\n"
	"uniform mat4 u_projection;\n"
	"uniform mat4 u_view;\n"
	"\n"
	"attribute vec3 a_position;\n"
	"\n"
	"varying vec3 v_position;\n"
	"\n"
	"void main()\n"
	"{\n"
	"	v_position = a_position;\n"
	"	gl_Position = u_projection * u_view * vec4(a_position,1);\n"
	"}\n";

static const char* horizon_fragment_shader_src =
	"#version 130\n"
	"\n"
	"varying vec3 v_position;\n"
	"\n"
	"void main(void)\n"
	"{\n"
	"	int x = int(v_position[0]/8+1000);\n"
	"	int y = int(v_position[2]/8+1000);\n"
	"	if (((x+y)&1)==0) {\n"
	"		gl_FragColor = vec4(0,0.2,0,1);\n"
	"	} else {\n"
	"		gl_FragColor = vec4(0,0.1,0,1);\n"
	"	}\n"
	"}\n";


static float render_get_fovy(struct render* render)
{
	AN(render);
	return 65;
}


static void render_init_horizon(struct render_horizon* h)
{
	{
		glGenBuffers(1, &h->vertex_buffer); CHKGL;
		GLuint type = GL_ARRAY_BUFFER;
		glBindBuffer(type, h->vertex_buffer); CHKGL;
		size_t sz = sizeof(float)*12;
		h->vertex_data = malloc(sz);
		AN(h->vertex_data);
		float big = 1000;
		for (int i = 0; i < 4; i++) {
			h->vertex_data[i*3+0] = (i&1)?big:-big;
			h->vertex_data[i*3+1] = 0;
			h->vertex_data[i*3+2] = (i&2)?big:-big;
		}
		glBufferData(type, sz, h->vertex_data, GL_STATIC_DRAW); CHKGL;
	}

	{
		glGenBuffers(1, &h->index_buffer); CHKGL;
		GLuint type = GL_ELEMENT_ARRAY_BUFFER;
		glBindBuffer(type, h->index_buffer); CHKGL;
		size_t sz = sizeof(int32_t)*4;
		h->index_data = malloc(sz);
		AN(h->index_data);
		h->index_data[0] = 1;
		h->index_data[1] = 0;
		h->index_data[2] = 2;
		h->index_data[3] = 3;
		glBufferData(type, sz, h->index_data, GL_STATIC_DRAW); CHKGL;
	}

	{
		shader_init(&h->shader, horizon_vertex_shader_src, horizon_fragment_shader_src);
		h->apos = glGetAttribLocation(h->shader.program, "a_position"); CHKGL;
	}
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
	static struct dtype_attr_spec color_specs[] = {
		{"a_position", 3},
		{"a_color", 4},
		{NULL, -1}
	};
	dtype_init(
		&render->color_dtype,
		&render->dbuf,
		color_shader_vertex_src,
		color_shader_fragment_src,
		color_specs
	);

	render_init_horizon(&render->horizon);
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

static void _color_add_vertex(struct render* render, struct vec3* position, struct vec4* color)
{
	int seq = 0;
	for (int i = 0; i < 3; i++) {
		dtype_add_vertex_float(&render->color_dtype, position->s[i], seq++);
	}
	for (int i = 0; i < 4; i++) {
		float value = color->s[i];
		if (i == 3) value *= render->color_alpha_multiplier;
		dtype_add_vertex_float(&render->color_dtype, value, seq++);
	}
}

static void render_road_node_bezier(struct render* render, struct track* track, struct track_node_bezier* bz)
{
	struct track_point tps[4];
	if (!track_node_bezier_derive_4_track_points(track, bz, tps)) return;

	int N = BEZIER_SUBDIV;

	for (int i = 0; i < N; i++) {

		struct vec3 points[8];
		struct vec3 normals[6];
		track_points_construct_block(tps, i, N, points, normals);

		dtype_new_quad(&render->road_dtype);
		float mflat = 0.5f;
		for (int i = 0; i < 4; i++) {
			_road_add_vertex(render, &points[i], &normals[i < 2 ? 0 : 1], mflat);
		}

		float mside = 1.5f;
		dtype_new_quad(&render->road_dtype);
		_road_add_vertex(render, &points[0], &normals[2], mside);
		_road_add_vertex(render, &points[3], &normals[3], mside);
		_road_add_vertex(render, &points[7], &normals[3], mside);
		_road_add_vertex(render, &points[4], &normals[2], mside);

		dtype_new_quad(&render->road_dtype);
		_road_add_vertex(render, &points[2], &normals[5], mside);
		_road_add_vertex(render, &points[1], &normals[4], mside);
		_road_add_vertex(render, &points[5], &normals[4], mside);
		_road_add_vertex(render, &points[6], &normals[5], mside);
	}
}

static void render_road_nodes(struct render* render, struct track* track)
{
	for (int i = 0; i < track->node_count; i++) {
		struct track_node* node = track_get_node(track, i);
		switch (node->type) {
			case TRACK_BEZIER:
				render_road_node_bezier(render, track, &node->bezier);
				break;
			case TRACK_DELETED: arghf("encountered TRACK_DELETED");
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

void render_clear(struct render* render)
{
	gl_viewport_from_sdl_window(render->window);

	glClearColor(0,0,0,0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); CHKGL;
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
}

void render_horizon(struct render* render)
{
	struct render_horizon* h = &render->horizon;
	shader_use(&h->shader);
	glEnableVertexAttribArray(h->apos); CHKGL;

	{
		GLint location = glGetUniformLocation(h->shader.program, "u_projection");
		glUniformMatrix4fv(location, 1, GL_FALSE, render->projection.s);
	}
	{
		GLint location = glGetUniformLocation(h->shader.program, "u_view");
		glUniformMatrix4fv(location, 1, GL_FALSE, render->view.s);
	}

	glBindBuffer(GL_ARRAY_BUFFER, h->vertex_buffer); CHKGL;
	glVertexAttribPointer(h->apos, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0); CHKGL;
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, h->index_buffer); CHKGL;
	glDrawElements(GL_QUADS, 4, GL_UNSIGNED_INT, NULL); CHKGL;
	glDisableVertexAttribArray(h->apos); CHKGL;
	glUseProgram(0); CHKGL;
}

void render_track(struct render* render, struct track* track)
{
	AN(render);

	render->frame++;

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
	float phi = I2RAD((float)(i%N) / (float)N);
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
		dtype_new_quad(&render->color_dtype);
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
			_color_add_vertex(render, &p, color);
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

	dtype_new_quad(&render->color_dtype);

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
		_color_add_vertex(render, &qp, color);
	}
}

void render_track_position_handles(struct render* render, struct track* track)
{
	glClear(GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); CHKGL;

	dtype_begin(&render->color_dtype);

	dtype_set_matrix(&render->color_dtype, "u_projection", &render->projection);
	dtype_set_matrix(&render->color_dtype, "u_view", &render->view);

	struct vec4 primary_color = {{1, 1, 0, 1}};
	struct vec4 secondary_color = {{0.5, 0.6, 1, 1}};
	struct vec4 line_color = {{0.3, 0.8, 0.3, 1}};
	float primary_radius = 0.04;
	float secondary_radius = 0.03;
	float line_width = 0.003;

	struct track_point tps[4];

	for (int i = 0; i < track->node_count; i++) {
		struct track_node* node = track_get_node(track, i);
		int j;
		switch (node->type) {
			case TRACK_BEZIER:
				if (!track_node_bezier_derive_4_track_points(track, &node->bezier, tps)) continue;
				for (j = 0; j < 3; j++) {
					render_circle(
						render,
						&tps[j].position,
						j == 0 ? primary_radius : secondary_radius,
						j == 0 ? &primary_color : &secondary_color
					);
				}
				render_line(
					render,
					&tps[0].position,
					&tps[1].position,
					line_width,
					&line_color
				);
				render_line(
					render,
					&tps[3].position,
					&tps[2].position,
					line_width,
					&line_color
				);
				break;
			case TRACK_DELETED:
				arghf("unexpected TRACK_NONE");
		}
	}

	dtype_end(&render->color_dtype);
}

void render_begin_color(struct render* render, int depth_mode)
{
	//glClear(GL_DEPTH_BUFFER_BIT);
	if (!depth_mode) {
		glDepthFunc(GL_LESS);
		render->color_alpha_multiplier = 1.0;
	} else {
		glDepthFunc(GL_GEQUAL);
		render->color_alpha_multiplier = 0.2;
	}
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); CHKGL;

	dtype_begin(&render->color_dtype);

	dtype_set_matrix(&render->color_dtype, "u_projection", &render->projection);
	dtype_set_matrix(&render->color_dtype, "u_view", &render->view);
}

void render_end_color(struct render* render)
{
	dtype_end(&render->color_dtype);
}

void render_draw_vector(struct render* render, struct vec3* o, struct vec3* v, struct vec4* colorp)
{
	struct vec3 a, b;
	vec3_complete_basis(v, &a, &b);

	float radius = 0.01f;
	int N = 8;

	struct vec4 color = {{1,1,0,1}};
	if (colorp != NULL) vec4_copy(&color, colorp);

	for (int i = 0; i < N; i++) {

		float t1 = 0.9;

		dtype_new_quad(&render->color_dtype);
		for (int j = 0; j < 4; j++) {
			struct vec3 p;
			vec3_copy(&p, o);
			float x, y;
			_circle_point(i + (j == 1 || j == 2 ? 1 : 0), N, radius, &x, &y);
			vec3_add_scaled_inplace(&p, &a, x);
			vec3_add_scaled_inplace(&p, &b, y);
			if (j >= 2) {
				vec3_add_scaled_inplace(&p, v, t1);
			}
			_color_add_vertex(render, &p, &color);
		}

		{
			dtype_new_triangle(&render->color_dtype);
			struct vec3 p;
			float x, y;

			{
				vec3_copy(&p, o);
				_circle_point(i, N, radius * 2, &x, &y);
				vec3_add_scaled_inplace(&p, &a, x);
				vec3_add_scaled_inplace(&p, &b, y);
				vec3_add_scaled_inplace(&p, v, 0.9);
				_color_add_vertex(render, &p, &color);
			}

			{
				vec3_copy(&p, o);
				_circle_point(i+1, N, radius * 2, &x, &y);
				vec3_add_scaled_inplace(&p, &a, x);
				vec3_add_scaled_inplace(&p, &b, y);
				vec3_add_scaled_inplace(&p, v, t1);
				_color_add_vertex(render, &p, &color);
			}

			{
				vec3_copy(&p, o);
				vec3_add_inplace(&p, v);
				_color_add_vertex(render, &p, &color);
			}
		}
	}
}

void render_box(struct render* render, struct mat44* model, struct vec3* extents)
{
	glDisable(GL_CULL_FACE);

	dtype_begin(&render->road_dtype);

	dtype_set_matrix(&render->road_dtype, "u_projection", &render->projection);
	dtype_set_matrix(&render->road_dtype, "u_view", &render->view);

	for (int i = 0; i < 3; i++) {
		int ap = 1<<i;
		for (int j = 0; j < 2; j++) {
			struct vec3 ps[4];
			int psi = 0;
			for (int k = 0; k < 8; k++) {
				if ((k&ap) == (ap*j)) {
					struct vec3 p = {{
						extents->s[0] * (k&1 ? 1 : -1),
						extents->s[1] * (k&2 ? 1 : -1),
						extents->s[2] * (k&4 ? 1 : -1),
					}};
					struct vec3 ptx;
					vec3_apply_mat44(&ptx, &p, model);
					vec3_copy(&ps[psi++], &ptx);
				}
			}
			ASSERT(psi == 4);
			dtype_new_quad(&render->road_dtype);
			float mat = 1.5;
			struct vec3 n = {{
				i == 0 ? (j == 0 ? 1 : -1) : 0,
				i == 1 ? (j == 0 ? 1 : -1) : 0,
				i == 2 ? (j == 0 ? 1 : -1) : 0
			}};
			struct vec3 ntx;
			vec3_apply_rotation_mat44(&ntx, &n, model);
			_road_add_vertex(render, &ps[0], &ntx, mat);
			_road_add_vertex(render, &ps[1], &ntx, mat);
			_road_add_vertex(render, &ps[3], &ntx, mat);
			_road_add_vertex(render, &ps[2], &ntx, mat);
		}
	}
	dtype_end(&render->road_dtype);
}

void render_a_wheel(struct render* render, struct mat44* model, float radius, float width)
{
	glDisable(GL_CULL_FACE);

	dtype_begin(&render->road_dtype);

	dtype_set_matrix(&render->road_dtype, "u_projection", &render->projection);
	dtype_set_matrix(&render->road_dtype, "u_view", &render->view);

	int N = 32;

	for (int i = 0; i < N; i++) {

		struct vec3 ps[4];

		for (int j = 0; j < 4; j++) {
			float x, y;
			_circle_point(i + (j == 1 || j == 2 ? 1 : 0), N, radius, &x, &y);
			struct vec3 p;
			vec3_zero(&p);
			struct vec3 a = {{0,1,0}};
			struct vec3 b = {{0,0,1}};
			struct vec3 c = {{width/2,0,0}};
			vec3_add_scaled_inplace(&p, &a, x);
			vec3_add_scaled_inplace(&p, &b, y);
			vec3_add_scaled_inplace(&p, &c, j < 2 ? -1 : 1);
			vec3_apply_mat44(&ps[j], &p, model);
		}

		float mat = (i&3) ? 2.5 : 0.5;
		struct vec3 n;
		vec3_calculate_normal_from_3_points(&n, ps);

		dtype_new_quad(&render->road_dtype);
		for (int j = 0; j < 4; j++) {
			_road_add_vertex(render, &ps[j], &n, mat);
		}
	}

	dtype_end(&render->road_dtype);
}

void render_flip(struct render* render)
{
	SDL_GL_SwapWindow(render->window);
}
