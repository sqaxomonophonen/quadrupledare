#ifndef RENDER_H
#define RENDER_H

#include <SDL.h>

#include "m.h"
#include "d.h"
#include "track.h"

struct render {
	SDL_Window* window;

	struct mat44 projection;
	struct mat44 view;

	struct dbuf dbuf;
	struct dtype road_dtype;
	struct dtype color_dtype;
	float color_alpha_multiplier;

	struct render_horizon {
		GLuint vertex_buffer;
		float* vertex_data;
		GLuint index_buffer;
		int32_t* index_data;
		struct shader shader;
		GLuint apos;
	} horizon;

	int frame;
};

void render_init(struct render* render, SDL_Window* window);

void render_clear(struct render* render);
void render_horizon(struct render* render);
void render_track(struct render* render, struct track* track);
void render_track_position_handles(struct render* render, struct track* track);

void render_a_wheel(struct render* render, struct mat44* model, float radius, float width);


void render_begin_color(struct render* render, int depth_mode);
void render_end_color(struct render* render);
void render_draw_vector(struct render* render, struct vec3* origin, struct vec3* v, struct vec4* colorp);
void render_box(struct render* render, struct mat44* model, struct vec3* extents);

void render_flip(struct render* render);

#endif/*RENDER_H*/
