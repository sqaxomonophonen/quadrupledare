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
	struct dtype handle_dtype;

	int frame;
};

void render_init(struct render* render, SDL_Window* window);
void render_track(struct render* render, struct track* track);
void render_track_position_handles(struct render* render, struct track* track);

void render_a_wheel(struct render* render, struct mat44* model);

void render_flip(struct render* render);

#endif/*RENDER_H*/
