#pragma once
#include "../include/octomath/vec2.h"

vec2 vec2_add(vec2 v1, vec2 v2)
{
	return (vec2) { v1.x + v2.x, v1.y + v2.y };
}

vec2 vec2_subtract(vec2 v1, vec2 v2)
{
	return (vec2) { v1.x - v2.x, v1.y - v2.y };
}

vec2 vec2_multiply(vec2 v1, vec2 v2)
{
	return (vec2) { v1.x * v2.x, v1.y * v2.y };
}

vec2 vec2_divide(vec2 v1, vec2 v2)
{
	vec2 out = { 0.0f, 0.0f };

	if (!(v1.x == 0.0f || v2.x == 0.0f))
	{
		out.x = v1.x / v2.x;
	}

	if (!(v1.y == 0.0f || v2.y == 0.0f))
	{
		out.y = v1.y / v2.y;
	}

	return out;
}