#include <SDL.h>

#include "a.h"
#include "editor.h"

void editor_init(struct editor* editor)
{
	memset(editor, 0, sizeof(struct editor));
	editor->mode = EDITOR_POSITION;
}

static void editor_mouselook(struct editor* editor, int dx, int dy)
{
	float sensitivity = 0.1f;
	editor->yaw += (float)dx * sensitivity;
	editor->pitch += (float)dy * sensitivity;
	float pitch_limit = 90;
	if (editor->pitch > pitch_limit) editor->pitch = pitch_limit;
	if (editor->pitch < -pitch_limit) editor->pitch = -pitch_limit;
}

int editor_run(struct editor* editor, struct render* render, struct track* track)
{
	SDL_SetRelativeMouseMode(SDL_FALSE);

	int ctrl_forward = 0;
	int ctrl_backward = 0;
	int ctrl_left = 0;
	int ctrl_right = 0;
	int ctrl_click = 0;

	int mx = 0;
	int my = 0;

	enum {
		DRAG_STATE_NONE = 0,
		DRAG_STATE_MOUSELOOK
	} drag_state = DRAG_STATE_NONE;

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
				if (e.key.keysym.sym == SDLK_w) ctrl_forward = 1;
				if (e.key.keysym.sym == SDLK_s) ctrl_backward = 1;
				if (e.key.keysym.sym == SDLK_a) ctrl_left = 1;
				if (e.key.keysym.sym == SDLK_d) ctrl_right = 1;
			}

			if (e.type == SDL_KEYUP) {
				if (e.key.keysym.sym == SDLK_w) ctrl_forward = 0;
				if (e.key.keysym.sym == SDLK_s) ctrl_backward = 0;
				if (e.key.keysym.sym == SDLK_a) ctrl_left = 0;
				if (e.key.keysym.sym == SDLK_d) ctrl_right = 0;
			}

			if (e.type == SDL_MOUSEMOTION) {
				mx = e.motion.x;
				my = e.motion.y;
				mdx += e.motion.xrel;
				mdy += e.motion.yrel;
			}

			if (e.type == SDL_MOUSEBUTTONDOWN) {
				ctrl_click++;
				mx = e.button.x;
				my = e.button.y;
			}

			if (e.type == SDL_MOUSEBUTTONUP) {
				ctrl_click--;
				mx = e.button.x;
				my = e.button.y;
			}
		}

		switch (drag_state) {
			case DRAG_STATE_MOUSELOOK:
				editor_mouselook(editor, mdx, mdy);
				break;
			case DRAG_STATE_NONE:
				break;
		}


		if (!ctrl_click && drag_state != DRAG_STATE_NONE) {
			drag_state = DRAG_STATE_NONE;
			SDL_SetRelativeMouseMode(SDL_FALSE);
		}

		if (ctrl_click && drag_state == DRAG_STATE_NONE) {
			drag_state = DRAG_STATE_MOUSELOOK;
			SDL_SetRelativeMouseMode(SDL_TRUE);
		}

		float speed = 0.5f;
		if (ctrl_forward || ctrl_backward) {
			float sgn = 0;
			if (ctrl_forward) sgn += 1.0f;
			if (ctrl_backward) sgn -= 1.0f;
			float yaw_s = sinf(DEG2RAD(editor->yaw));
			float yaw_c = cosf(DEG2RAD(editor->yaw));
			float pitch_s = sinf(DEG2RAD(editor->pitch));
			float pitch_c = cosf(DEG2RAD(editor->pitch));
			struct vec3 forward = {{
				yaw_s * pitch_c,
				-pitch_s,
				-yaw_c * pitch_c
			}};
			vec3_add_scaled_inplace(&editor->position, &forward, sgn * speed);
		}

		if (ctrl_left || ctrl_right) {
			float sgn = 0;
			if (ctrl_right) sgn += 1.0f;
			if (ctrl_left) sgn -= 1.0f;
			float yaw_s = sinf(DEG2RAD(editor->yaw));
			float yaw_c = cosf(DEG2RAD(editor->yaw));
			struct vec3 right = {{
				yaw_c,
				0,
				yaw_s,
			}};
			vec3_add_scaled_inplace(&editor->position, &right, sgn * speed);
		}

		mat44_set_identity(&render->view);
		mat44_rotate_x(&render->view, editor->pitch);
		mat44_rotate_y(&render->view, editor->yaw);
		struct vec3 translation;
		vec3_scale(&translation, &editor->position, -1);
		mat44_translate(&render->view, &translation);

		render_track(render, track);
		render_track_position_handles(render, track);

		render_flip(render);
	}

	return 0; // TODO pass control to... who?
}
