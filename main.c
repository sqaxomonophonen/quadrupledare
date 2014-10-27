#include <SDL.h>
#include <GL/glew.h>

#include <stdio.h>

#include "sim.h"
#include "a.h"
#include "render.h"
#include "track.h"
#include "editor.h"
#include "game.h"

void glew_init()
{
	GLenum err = glewInit();
	if (err != GLEW_OK) {
		arghf("glewInit() failed: %s", glewGetErrorString(err));
	}

#define CHECK_GL_EXT(x) { if(!GLEW_ ## x) arghf("OpenGL extension not found: " #x); }
	CHECK_GL_EXT(ARB_shader_objects);
	CHECK_GL_EXT(ARB_vertex_shader);
	CHECK_GL_EXT(ARB_fragment_shader);
	CHECK_GL_EXT(ARB_framebuffer_object);
	CHECK_GL_EXT(ARB_vertex_buffer_object);
#undef CHECK_GL_EXT

	/* to figure out what extension something belongs to, see:
	 * http://www.opengl.org/registry/#specfiles */

	// XXX check that version is at least 1.30?
	// printf("GLSL version %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
}


int main(int argc, char** argv)
{
	SAZ(SDL_Init(SDL_INIT_VIDEO));
	atexit(SDL_Quit);

	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24); // XXX use framebuffer instead? this be some fragile sheeit

	int bitmask = SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_OPENGL;
	SDL_Window* window = SDL_CreateWindow(
			"quadruple dare",
			SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
			0, 0,
			bitmask);

	SAN(window);

	SDL_GLContext glctx = SDL_GL_CreateContext(window);
	SAN(glctx);

	SAZ(SDL_GL_SetSwapInterval(1)); // or -1, "late swap tearing"?

	glew_init();


	struct render render;
	render_init(&render, window);

	struct track track;
	track_init_demo(&track);

	#if 0
	struct editor editor;
	editor_init(&editor);
	editor_run(&editor, &render, &track);
	#else
	SDL_DisplayMode mode;
	SDL_GetCurrentDisplayMode(SDL_GetWindowDisplayIndex(window), &mode);
	float dt = 1.0f / (float)mode.refresh_rate;
	struct game game;
	game_init(&game, dt, &track);

	game_run(&game, &render);
	#endif

	SDL_DestroyWindow(window);
	SDL_GL_DeleteContext(glctx);

	return 0;
}
