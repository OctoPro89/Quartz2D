#pragma once
#include "common.h"

typedef struct
{
	float x, y;
} vec2;

vec2 vec2_add(vec2 v1, vec2 v2);
vec2 vec2_subtract(vec2 v1, vec2 v2);
vec2 vec2_multiply(vec2 v1, vec2 v2);
vec2 vec2_divide(vec2 v1, vec2 v2);
