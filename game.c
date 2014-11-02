#include <string.h>

#include "magic.h"
#include "game.h"

static void add_bezier_node_to_sim(struct game* game, struct track* track, struct track_node_bezier* bezier)
{
	struct track_point tps[4];
	if (!track_node_bezier_derive_4_track_points(track, bezier, tps)) return;

	int N = BEZIER_SUBDIV;
	for (int i = 0; i < N; i++) {
		struct vec3 points[8];
		track_points_construct_block(tps, i, N, points, NULL);

		#if 1
		sim_add_block(game->sim, points, 8);
		#else

		struct vec3 tp[6];

		{
			int tpi = 0;
			vec3_copy(&tp[tpi++], &points[0]);
			vec3_copy(&tp[tpi++], &points[1]);
			vec3_copy(&tp[tpi++], &points[2]);
			vec3_copy(&tp[tpi++], &points[4]);
			vec3_copy(&tp[tpi++], &points[5]);
			vec3_copy(&tp[tpi++], &points[6]);
			sim_add_block(game->sim, tp, 6);
		}
		{
			int tpi = 0;
			vec3_copy(&tp[tpi++], &points[0]);
			vec3_copy(&tp[tpi++], &points[2]);
			vec3_copy(&tp[tpi++], &points[3]);
			vec3_copy(&tp[tpi++], &points[4]);
			vec3_copy(&tp[tpi++], &points[6]);
			vec3_copy(&tp[tpi++], &points[7]);
			sim_add_block(game->sim, tp, 6);
		}
		#endif
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
	struct vec3 fly_position;

	int ctrl_accel = 0;
	int ctrl_brake = 0;
	int ctrl_steer_left = 0;
	int ctrl_steer_right = 0;

	int ctrl_fly_forward = 0;
	int ctrl_fly_backward = 0;
	int ctrl_fly_left = 0;
	int ctrl_fly_right = 0;

	int exiting = 0;
	int fly_mode = 0;

	struct mat44 last_vehicle_view;

	while (!exiting) {
		SDL_Event e;
		int mdx = 0;
		int mdy = 0;
		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT) exiting = 1;

			struct push_key {
				SDL_Keycode sym;
				int* intptr;
			} push_keys[] = {
				{SDLK_UP, &ctrl_accel},
				{SDLK_DOWN, &ctrl_brake},
				{SDLK_LEFT, &ctrl_steer_left},
				{SDLK_RIGHT, &ctrl_steer_right},
				{SDLK_w, &ctrl_fly_forward},
				{SDLK_s, &ctrl_fly_backward},
				{SDLK_a, &ctrl_fly_left},
				{SDLK_d, &ctrl_fly_right},
				{-1, NULL}
			};

			for (struct push_key* tkp = push_keys; tkp->intptr != NULL; tkp++) {
				if ((e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) && e.key.keysym.sym == tkp->sym) {
					*(tkp->intptr) = (e.type == SDL_KEYDOWN);
				}
			}

			if (e.type == SDL_KEYDOWN) {
				if (e.key.keysym.sym == SDLK_ESCAPE) {
					exiting = 1;
				}
				if (e.key.keysym.sym == SDLK_TAB) {
					fly_mode = !fly_mode;
					if (fly_mode) {
						yaw = 0;
						vec3_zero(&fly_position);
					} else {
						yaw = 180;
					}
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

		{
			float speed = 0.5f;
			float forward = (float)(ctrl_fly_forward - ctrl_fly_backward) * speed;
			float right = (float)(ctrl_fly_right - ctrl_fly_left) * speed;
			struct vec3 movement;
			vec3_move(&movement, yaw, pitch, forward, right);
			vec3_add_inplace(&fly_position, &movement);
		}



		sim_vehicle_ctrl(sim_get_vehicle(game->sim, 0), ctrl_accel, ctrl_brake, ctrl_steer_right - ctrl_steer_left);

		sim_step(game->sim, game->dt);

		if (fly_mode) {
			mat44_set_identity(&render->view);
			mat44_rotate_x(&render->view, pitch);
			mat44_rotate_y(&render->view, yaw);

			struct vec3 translate;
			vec3_scale(&translate, &fly_position, -1);
			mat44_translate(&render->view, &translate);
			mat44_multiply_inplace(&render->view, &last_vehicle_view);
		} else {
			mat44_set_identity(&render->view);
			mat44_rotate_x(&render->view, pitch);
			mat44_rotate_y(&render->view, yaw);
			struct vec3 up = {{0,-0.6,0}};
			mat44_translate(&render->view, &up);
			struct mat44 vtx;
			struct sim_vehicle* vehicle = sim_get_vehicle(game->sim, 0);
			sim_vehicle_get_tx(vehicle, &vtx);
			mat44_multiply_inplace(&render->view, &vtx);
			mat44_copy(&last_vehicle_view, &render->view);
		}

		render_clear(render);
		render_horizon(render);
		render_track(render, game->track);

		sim_vehicle_render(render, sim_get_vehicle(game->sim, 0));

		for (int depth_mode = 0; depth_mode < 2; depth_mode++) {
			render_begin_color(render, depth_mode);
			sim_vehicle_visualize(sim_get_vehicle(game->sim, 0), render);
			render_end_color(render);
		}

		render_flip(render);
	}

}
