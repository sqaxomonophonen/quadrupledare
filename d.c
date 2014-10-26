#include <stdio.h>
#include <stdlib.h>

#include "d.h"
#include "a.h"

static void _init_buffer(GLuint* buffer, size_t sz, void** data, GLuint type)
{
	glGenBuffers(1, buffer); CHKGL;
	glBindBuffer(type, *buffer); CHKGL;
	*data = malloc(sz);
	AN(*data);
	glBufferData(type, sz, *data, GL_STREAM_DRAW); CHKGL;
}

void dbuf_init(struct dbuf* dbuf, size_t vertex_buffer_sz, size_t index_buffer_sz)
{
	_init_buffer(
		&dbuf->vertex_buffer,
		(dbuf->vertex_buffer_sz = vertex_buffer_sz) * sizeof(float),
		(void**) &dbuf->vertex_data,
		GL_ARRAY_BUFFER
	);

	_init_buffer(
		&dbuf->index_buffer,
		(dbuf->index_buffer_sz = index_buffer_sz) * sizeof(int32_t),
		(void**) &dbuf->index_data,
		GL_ELEMENT_ARRAY_BUFFER
	);
}

static void _dbuf_reset(struct dbuf* dbuf)
{
	dbuf->vertex_used = 0;
	dbuf->index_used = 0;
}

void dtype_init(struct dtype* dtype, struct dbuf* dbuf, const char* vertex_shader, const char* fragment_shader, struct dtype_attr_spec attr_specs[])
{
	dtype->dbuf = dbuf;
	shader_init(&dtype->shader, vertex_shader, fragment_shader);

	struct dtype_attr_spec* attr_spec = attr_specs;
	dtype->attr_count = 0;
	dtype->floats_per_vertex = 0;
	while (attr_spec->symbol != NULL) {
		ASSERT(attr_spec->n_floats >= 1);
		ASSERT(dtype->attr_count < DTYPE_ATTR_MAX);

		dtype->attr[dtype->attr_count] = glGetAttribLocation(dtype->shader.program, attr_spec->symbol); CHKGL;
		dtype->attr_n_floats[dtype->attr_count] = attr_spec->n_floats;
		dtype->floats_per_vertex += attr_spec->n_floats;

		attr_spec++;
		dtype->attr_count++;
	}

	dtype->vertex_next_seq = -1;
}

void dtype_begin(struct dtype* dtype)
{
	ASSERT(dtype->vertex_next_seq == -1);
	shader_use(&dtype->shader);
	for (int i = 0; i < dtype->attr_count; i++) {
		glEnableVertexAttribArray(dtype->attr[i]); CHKGL;
	}
	dtype->vertex_next_seq = 0;
	_dbuf_reset(dtype->dbuf);
}

static void _dtype_flush(struct dtype* dtype)
{
	struct dbuf* dbuf = dtype->dbuf;

	glBindBuffer(GL_ARRAY_BUFFER, dbuf->vertex_buffer); CHKGL;
	glBufferSubData(GL_ARRAY_BUFFER, 0, dbuf->vertex_used * sizeof(float), dbuf->vertex_data); CHKGL;

	size_t offset = 0;
	for (int i = 0; i < dtype->attr_count; i++) {
		glVertexAttribPointer(dtype->attr[i], dtype->attr_n_floats[i], GL_FLOAT, GL_FALSE, dtype->floats_per_vertex * sizeof(float), (char*)(sizeof(float)*offset)); CHKGL;
		offset += dtype->attr_n_floats[i];
	}

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dbuf->index_buffer); CHKGL;
	glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, dbuf->index_used * sizeof(int32_t), dbuf->index_data); CHKGL;
	glDrawElements(GL_TRIANGLES, dbuf->index_used, GL_UNSIGNED_INT, NULL); CHKGL;

	_dbuf_reset(dtype->dbuf);
}

void dtype_end(struct dtype* dtype)
{
	ASSERT(dtype->vertex_next_seq >= 0);
	dtype->vertex_next_seq = -1;
	_dtype_flush(dtype);
	for (int i = 0; i < dtype->attr_count; i++) {
		glDisableVertexAttribArray(dtype->attr[i]); CHKGL;
	}
	glUseProgram(0); CHKGL;
}

void dtype_set_matrix(struct dtype* dtype, const char* uniform_name, struct mat44* matrix)
{
	ASSERT(dtype->vertex_next_seq >= 0);

	GLint location = glGetUniformLocation(dtype->shader.program, uniform_name);
	glUniformMatrix4fv(location, 1, GL_FALSE, matrix->s);
}

static void _dtype_requires(struct dtype* dtype, int vertices, int indices)
{
	int vertex_floats = vertices * dtype->floats_per_vertex;
	struct dbuf* dbuf = dtype->dbuf;
	if (dbuf->vertex_used + vertex_floats > dbuf->vertex_buffer_sz || dbuf->index_used + indices > dbuf->index_buffer_sz) {
		_dtype_flush(dtype);
	}
	ASSERT(dbuf->vertex_used + vertex_floats <= dbuf->vertex_buffer_sz && dbuf->index_used + indices <= dbuf->index_buffer_sz);
}

static void _dtype_add_index(struct dtype* dtype, int32_t index)
{
	struct dbuf* dbuf = dtype->dbuf;
	ASSERT(dbuf->index_used < dbuf->index_buffer_sz);
	dbuf->index_data[dbuf->index_used++] = index;
}

void dtype_new_triangle(struct dtype* dtype)
{
	// XXX TODO sanity check vertices used
	_dtype_requires(dtype, 3, 3);
	int o = dtype->dbuf->vertex_used / dtype->floats_per_vertex;
	_dtype_add_index(dtype, o + 0);
	_dtype_add_index(dtype, o + 1);
	_dtype_add_index(dtype, o + 2);
}

void dtype_new_quad(struct dtype* dtype)
{
	// XXX TODO sanity check vertices used
	_dtype_requires(dtype, 4, 6);
	int o = dtype->dbuf->vertex_used / dtype->floats_per_vertex;
	_dtype_add_index(dtype, o + 0);
	_dtype_add_index(dtype, o + 1);
	_dtype_add_index(dtype, o + 2);
	_dtype_add_index(dtype, o + 0);
	_dtype_add_index(dtype, o + 2);
	_dtype_add_index(dtype, o + 3);
}

