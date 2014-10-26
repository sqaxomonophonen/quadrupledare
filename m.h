#ifndef M_H
#define M_H

#ifndef M_PI
#define M_PI (3.141592653589793)
#endif

#define DEG2RAD(x) (x/180.0f*M_PI)


float bezier(float t, float a, float b, float c, float d);
float bezier_deriv(float t, float a, float b, float c, float d);

struct vec3 {
	float s[3];
};

void vec3_dump(struct vec3* x);
void vec3_copy(struct vec3* dst, struct vec3* src);
void vec3_scale(struct vec3* dst, struct vec3* src, float scalar);
void vec3_add_inplace(struct vec3* dst, struct vec3* src);
void vec3_add_scaled_inplace(struct vec3* dst, struct vec3* src, float scalar);
void vec3_sub(struct vec3* dst, struct vec3* a, struct vec3* b);
void vec3_scale_inplace(struct vec3* dst, float scalar);
void vec3_lerp(struct vec3* dst, struct vec3* a, struct vec3* b, float t);
float vec3_dot(struct vec3* a, struct vec3* b);
void vec3_cross(struct vec3* dst, struct vec3* a, struct vec3* b);
void vec3_normalize_inplace(struct vec3* dst);

void vec3_bezier(struct vec3* dst, float t, struct vec3* a, struct vec3* b, struct vec3* c, struct vec3* d);
void vec3_bezier_deriv(struct vec3* dst, float t, struct vec3* a, struct vec3* b, struct vec3* c, struct vec3* d);

struct vec4 {
	float s[4];
};
void vec4_dump(struct vec4* x);
float vec4_dot(struct vec4* a, struct vec4* b);


// column-major order
struct mat44 {
	float s[16];
};

void mat44_dump(struct mat44* x);
void mat44_copy(struct mat44* dst, struct mat44* src);
void mat44_multiply(struct mat44* dst, struct mat44* a, struct mat44* b);
void mat44_multiply_inplace(struct mat44* dst, struct mat44* b);
void mat44_set_identity(struct mat44* m);
void mat44_set_perspective(struct mat44* m, float fovy, float aspect, float znear, float zfar);
float mat44_get_znear(struct mat44* m);
void mat44_set_rotation(struct mat44* m, float angle, struct vec3* axis);
void mat44_rotate(struct mat44* m, float angle, struct vec3* axis);
void mat44_rotate_x(struct mat44* m, float angle);
void mat44_rotate_y(struct mat44* m, float angle);
void mat44_set_translation(struct mat44* m, struct vec3* delta);
void mat44_translate(struct mat44* m, struct vec3* delta);
void mat44_inverse(struct mat44* dst, struct mat44* src);
void mat44_get_bases(struct mat44* m, struct vec3* x, struct vec3* y, struct vec3* z);

// combined
void vec3_from_vec4(struct vec3* dst, struct vec4* src);
void vec4_from_vec3(struct vec4* dst, struct vec3* src);
void vec3_apply_mat44(struct vec3* dst, struct vec3* src, struct mat44* m);
void vec4_apply_mat44_to_vec3(struct vec4* dst, struct vec3* src, struct mat44* m);

#endif/*M_H*/
