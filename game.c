#include <string.h>

#include "game.h"

static void add_bezier_node_to_sim(struct game* game, struct track* track, struct track_node_bezier* bezier)
{
	struct track_point tps[4];
	if (!track_node_bezier_derive_4_track_points(track, bezier, tps)) return;

	int N = 50;
	for (int i = 0; i < N; i++) {
		struct vec3 points[8];
		track_points_construct_block(tps, i, N, points, NULL);
		sim_add_block(game->sim, points, 8);
	}
}

void game_init(struct game* game, float dt, struct track* track)
{
	memset(game, 0, sizeof(struct game));
	game->dt = dt;
	game->track = track;

	game->sim = sim_new();

	for (int i = 0; i < track->node_count; i++) {
		struct track_node* node = track_get_node(track, i);
		switch (node->type) {
			case TRACK_BEZIER:
				add_bezier_node_to_sim(game, track, &node->bezier);
				break;
			case TRACK_DELETED: arghf("encountered TRACK_DELETED");
				break;
		}
	}
}

void game_run(struct game* game, struct render* render)
{
	int exiting = 0;
	while (!exiting) {
		SDL_Event e;
		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT) exiting = 1;
			if (e.type == SDL_KEYDOWN) {
				if (e.key.keysym.sym == SDLK_ESCAPE) {
					exiting = 1;
				}
			}
		}

		sim_step(game->sim, game->dt);

		struct sim_vehicle* vehicle = sim_get_vehicle(game->sim, 0);
		sim_vehicle_get_tx(vehicle, &render->view);

		render_track(render, game->track);
		render_flip(render);
	}

}
