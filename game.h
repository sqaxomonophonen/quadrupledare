#ifndef GAME_H
#define GAME_H

#include "render.h"
#include "track.h"
#include "sim.h"

struct game {
	float dt;
	struct track* track;
	struct sim* sim;
};

void game_init(struct game* game, float dt, struct track* track);
void game_run(struct game* game, struct render* render);

#endif/*GAME_H*/
