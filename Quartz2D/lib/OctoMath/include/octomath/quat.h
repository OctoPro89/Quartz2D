#pragma once
#include "common.h"
#include "vec3.h"

typedef struct
{
	float x, y, z, w;
} quat;

quat quat_identity();
quat quat_multiply(quat q1, quat q2);
quat quat_conjugate(quat q);
quat quat_normalize(quat q);
quat quat_from_scalar_and_vec3(float scalar, vec3 v);
quat quat_roll_pitch_yaw_from_vec3(vec3 v);
quat quat_from_euler_angles(vec3 euler_angles);
vec3 vec3_euler_angles_from_quat(quat q);
vec3 vec3_rotate(vec3 v, quat q);