#ifndef EDITOR_H
#define EDITOR_H

#include "render.h"
#include "track.h"
#include "m.h"

struct editor {
	enum editor_mode {
		EDITOR_POSITION = 1,
		EDITOR_NORMAL,
		EDITOR_WIDTH,
		EDITOR_NODE,
	} mode;

	// camera
	float yaw;
	float pitch;
	struct vec3 position;
};

void editor_init(struct editor* editor);

int editor_run(struct editor* editor, struct render* render, struct track* track);

#endif/*EDITOR_H*/
