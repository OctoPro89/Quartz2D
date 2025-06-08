#pragma once
#include "common.h"

typedef struct
{
	float x, y, z;
} vec3;

vec3 vec3_cross(vec3 v1, vec3 v2);
vec3 vec3_normalize(vec3 v);
float vec3_magnitude(vec3 v1);
float vec3_magnitude2(vec3 v1, vec3 v2);
float vec3_magnitude_sq(vec3 v1);
float vec3_magnitude_sq2(vec3 v1, vec3 v2);
float vec3_dot(vec3 v1, vec3 v2);
vec3 vec3_scalar(float scalar);
vec3 vec3_add_scalar(vec3 v1, float scalar);
vec3 vec3_subtract_scalar(vec3 v1, float scalar);
vec3 vec3_multiply_scalar(vec3 v1, float scalar);
vec3 vec3_divide_scalar(vec3 v1, float scalar);
vec3 vec3_add(vec3 v1, vec3 v2);
vec3 vec3_subtract(vec3 v1, vec3 v2);
vec3 vec3_multiply(vec3 v1, vec3 v2);
vec3 vec3_divide(vec3 v1, vec3 v2);