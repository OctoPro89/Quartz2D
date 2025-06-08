#pragma once
#include "common.h"
#include "vec3.h"
#include "quat.h"

typedef struct
{
	float r[4][4];
} mat4;

mat4 mat4_identity();
mat4 mat4_multiply(const mat4* m1, const mat4* m2);
mat4 mat4_affine_transformation(vec3 scaling, vec3 rotation_origin, quat rotation_quat, vec3 translation);
mat4 mat4_lookat_rh(vec3 eye_position, vec3 focus_position, vec3 up);
mat4 mat4_lookto_rh(vec3 eye_position, vec3 direction, vec3 up);
mat4 mat4_perspective_fov_rh(float fov_y, float aspect_ratio, float near_z, float far_z);
mat4 mat4_orthographic_rh(float view_width, float view_height, float near_z, float far_z);
mat4 mat4_orthographic_rh_offset(float left, float right, float bottom, float top, float near_z, float far_z);
mat4 mat4_inverse(const mat4* m);
mat4 mat4_create_xr_projection(float left, float right, float up, float down, float near_z, float far_z);