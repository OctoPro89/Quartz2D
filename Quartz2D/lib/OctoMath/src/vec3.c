#include "../include/octomath/vec3.h"
#include "../include/octomath/quat.h"
#include <math.h>

// Cross product of two vec3
vec3 vec3_cross(vec3 v1, vec3 v2) {
    vec3 result = { 0 };
    result.x = v1.y * v2.z - v1.z * v2.y;
    result.y = v1.z * v2.x - v1.x * v2.z;
    result.z = v1.x * v2.y - v1.y * v2.x;
    return result;
}

// Normalize a vec3
vec3 vec3_normalize(vec3 v) {
    float magnitude = vec3_magnitude(v);
    if (magnitude == 0.0f) {
        return (vec3) { 0.0f, 0.0f, 0.0f };
    }
    return (vec3) { v.x / magnitude, v.y / magnitude, v.z / magnitude };
}

// Magnitude of a vec3
float vec3_magnitude(vec3 v) {
    return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

// Distance between two vec3 (magnitude of difference)
float vec3_magnitude2(vec3 v1, vec3 v2) {
    return vec3_magnitude(vec3_subtract(v1, v2));
}

float vec3_magnitude_sq(vec3 v) {
    return v.x * v.x + v.y * v.y + v.z * v.z;
}

float vec3_magnitude_sq2(vec3 v1, vec3 v2) {
    return vec3_magnitude_sq(vec3_subtract(v1, v2));
}

// Dot product of two vec3
float vec3_dot(vec3 v1, vec3 v2) {
    return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

// Scalar creation
vec3 vec3_scalar(float scalar) {
    return (vec3) { scalar, scalar, scalar };
}

vec3 vec3_add_scalar(vec3 v1, float scalar)
{
    return (vec3) { v1.x + scalar, v1.y + scalar, v1.z + scalar };
}

vec3 vec3_subtract_scalar(vec3 v1, float scalar)
{
    return (vec3) { v1.x - scalar, v1.y - scalar, v1.z - scalar };
}

vec3 vec3_multiply_scalar(vec3 v1, float scalar)
{
    return (vec3) { v1.x * scalar, v1.y * scalar, v1.z * scalar };
}

vec3 vec3_divide_scalar(vec3 v1, float scalar)
{
    return (vec3) { v1.x / scalar, v1.y / scalar, v1.z / scalar };
}

// Add two vec3
vec3 vec3_add(vec3 v1, vec3 v2) {
    return (vec3) { v1.x + v2.x, v1.y + v2.y, v1.z + v2.z };
}

// Subtract two vec3
vec3 vec3_subtract(vec3 v1, vec3 v2) {
    return (vec3) { v1.x - v2.x, v1.y - v2.y, v1.z - v2.z };
}

// Multiply two vec3 component-wise
vec3 vec3_multiply(vec3 v1, vec3 v2) {
    return (vec3) { v1.x * v2.x, v1.y * v2.y, v1.z * v2.z };
}

// Divide two vec3 component-wise
vec3 vec3_divide(vec3 v1, vec3 v2) {
    return (vec3) {
        v2.x != 0.0f ? v1.x / v2.x : 0.0f,
            v2.y != 0.0f ? v1.y / v2.y : 0.0f,
            v2.z != 0.0f ? v1.z / v2.z : 0.0f
    };
}