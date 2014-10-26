#ifndef D_H
#define D_H

#include "shader.h"
#include "m.h"
#include "a.h"

struct dbuf {
	GLuint vertex_buffer;
	size_t vertex_buffer_sz;
	float* vertex_data;
	size_t vertex_used;

	GLuint index_buffer;
	size_t index_buffer_sz;
	int32_t* index_data;
	size_t index_used;
};

void dbuf_init(struct dbuf* dbuf, size_t vertex_buffer_sz, size_t index_buffer_sz);

#define DTYPE_ATTR_MAX (12)

struct dtype_attr_spec {
	const char* symbol;
	int n_floats;
};

struct dtype {
	struct dbuf* dbuf;
	struct shader shader;

	GLuint attr[DTYPE_ATTR_MAX];
	int attr_n_floats[DTYPE_ATTR_MAX];
	int attr_count;
	int floats_per_vertex;

	int vertex_next_seq;
};

void dtype_init(struct dtype* dtype, struct dbuf* dbuf, const char* vertex_shader, const char* fragment_shader, struct dtype_attr_spec attr_specs[]);

void dtype_begin(struct dtype* dtype);
void dtype_end(struct dtype* dtype);


void dtype_set_matrix(struct dtype* dtype, const char* uniform_name, struct mat44* matrix);

void dtype_new_triangle(struct dtype* dtype);
void dtype_new_quad(struct dtype* dtype);
static inline void dtype_add_vertex_float(struct dtype* dtype, float value, int seq)
{
	ASSERT(seq == dtype->vertex_next_seq);
	struct dbuf* dbuf = dtype->dbuf;
	ASSERT(dbuf->vertex_used < dbuf->vertex_buffer_sz);

	dbuf->vertex_data[dbuf->vertex_used++] = value;

	dtype->vertex_next_seq++;
	if (dtype->vertex_next_seq == dtype->floats_per_vertex) {
		dtype->vertex_next_seq = 0;
	}
	ASSERT(dtype->vertex_next_seq >= 0 && dtype->vertex_next_seq < dtype->floats_per_vertex);
}

#endif/*D_H*/
