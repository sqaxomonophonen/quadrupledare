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
	SDL_SetRelativeMouseMode(SDL_TRUE);

	float yaw = 180;
	float pitch = 0;

	int ctrl_accel = 0;
	int ctrl_brake = 0;
	int ctrl_left = 0;
	int ctrl_right = 0;

	int exiting = 0;
	while (!exiting) {
		SDL_Event e;
		int mdx = 0;
		int mdy = 0;
		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT) exiting = 1;
			if (e.type == SDL_KEYDOWN) {
				if (e.key.keysym.sym == SDLK_ESCAPE) {
					exiting = 1;
				}
				if (e.key.keysym.sym == SDLK_w) {
					ctrl_accel = 1;
				}
				if (e.key.keysym.sym == SDLK_s) {
					ctrl_brake = 1;
				}
				if (e.key.keysym.sym == SDLK_a) {
					ctrl_left = 1;
				}
				if (e.key.keysym.sym == SDLK_d) {
					ctrl_right = 1;
				}
			}
			if (e.type == SDL_KEYUP) {
				if (e.key.keysym.sym == SDLK_w) {
					ctrl_accel = 0;
				}
				if (e.key.keysym.sym == SDLK_s) {
					ctrl_brake = 0;
				}
				if (e.key.keysym.sym == SDLK_a) {
					ctrl_left = 0;
				}
				if (e.key.keysym.sym == SDLK_d) {
					ctrl_right = 0;
				}
			}
			if (e.type == SDL_MOUSEMOTION) {
				mdx += e.motion.xrel;
				mdy += e.motion.yrel;
			}
		}

		{
			float sensitivity = 0.1f;
			yaw += (float)mdx * sensitivity;
			pitch += (float)mdy * sensitivity;
			float pitch_limit = 90;
			if (pitch > pitch_limit) pitch = pitch_limit;
			if (pitch < -pitch_limit) pitch = -pitch_limit;
		}


		sim_vehicle_ctrl(sim_get_vehicle(game->sim, 0), ctrl_accel, ctrl_brake, ctrl_right - ctrl_left);

		sim_step(game->sim, game->dt);

		struct sim_vehicle* vehicle = sim_get_vehicle(game->sim, 0);
		mat44_set_identity(&render->view);
		mat44_rotate_x(&render->view, pitch);
		mat44_rotate_y(&render->view, yaw);
		struct mat44 vtx;
		sim_vehicle_get_tx(vehicle, &vtx);
		mat44_multiply_inplace(&render->view, &vtx);

		render_track(render, game->track);

		for (int i = 0; i < 4; i++) {
			struct mat44 wtx;
			sim_vehicle_get_wheel_tx(sim_get_vehicle(game->sim, 0), &wtx, i);
			render_a_wheel(render, &wtx);
		}


		render_flip(render);
	}

}
