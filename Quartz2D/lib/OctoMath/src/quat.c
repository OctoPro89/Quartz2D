#include "../include/octomath/quat.h"
#include "../include/octomath/vec3.h"

quat quat_identity() {
    quat q = { 0 };
    q.x = 0.0f;
    q.y = 0.0f;
    q.z = 0.0f;
    q.w = 1.0f;
    return q;
}

quat quat_multiply(quat q1, quat q2) {
    quat result = { 0 };
    result.w = q1.w * q2.w - q1.x * q2.x - q1.y * q2.y - q1.z * q2.z;
    result.x = q1.w * q2.x + q1.x * q2.w + q1.y * q2.z - q1.z * q2.y;
    result.y = q1.w * q2.y - q1.x * q2.z + q1.y * q2.w + q1.z * q2.x;
    result.z = q1.w * q2.z + q1.x * q2.y - q1.y * q2.x + q1.z * q2.w;
    return result;
}

quat quat_conjugate(quat q) {
    quat result = { 0 };
    result.x = -q.x;
    result.y = -q.y;
    result.z = -q.z;
    result.w = q.w;
    return result;
}

quat quat_normalize(quat q)
{
    float len = sqrtf(q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z);
    if (len == 0.0f) { return q; }
    return (quat){ q.w / len, q.x / len, q.y / len, q.z / len };
}

quat quat_from_scalar_and_vec3(float scalar, vec3 v)
{
    quat result = { v.x, v.y, v.z, scalar };
    return result;
}

quat quat_roll_pitch_yaw_from_vec3(vec3 v)
{
    // Extract individual components (z - yaw, y - pitch, x - roll)
    float yaw = v.z;
    float pitch = v.y;
    float roll = v.x;

    // Calculate half-angles
    float halfYaw = yaw * 0.5f;
    float halfPitch = pitch * 0.5f;
    float halfRoll = roll * 0.5f;

    // Compute sine and cosine of half-angles
    float cy = cosf(halfYaw);  // cos(yaw / 2)
    float sy = sinf(halfYaw);  // sin(yaw / 2)
    float cp = cosf(halfPitch); // cos(pitch / 2)
    float sp = sinf(halfPitch); // sin(pitch / 2)
    float cr = cosf(halfRoll);  // cos(roll / 2)
    float sr = sinf(halfRoll);  // sin(roll / 2)

    // Combine to form the quaternion (zyx order)
    quat q;
    q.w = cr * cp * cy + sr * sp * sy;
    q.x = sr * cp * cy - cr * sp * sy;
    q.y = cr * sp * cy + sr * cp * sy;
    q.z = cr * cp * sy - sr * sp * cy;

    return q;
}

quat quat_from_euler_angles(vec3 euler_angles) {
    float cx = cosf(euler_angles.x * 0.5f);
    float sx = sinf(euler_angles.x * 0.5f);
    float cy = cosf(euler_angles.y * 0.5f);
    float sy = sinf(euler_angles.y * 0.5f);
    float cz = cosf(euler_angles.z * 0.5f);
    float sz = sinf(euler_angles.z * 0.5f);

    quat result = { 0 };
    result.w = cx * cy * cz + sx * sy * sz;
    result.x = sx * cy * cz - cx * sy * sz;
    result.y = cx * sy * cz + sx * cy * sz;
    result.z = cx * cy * sz - sx * sy * cz;

    return result;
}

// Extract Euler angles from a quaternion (quat assumed to be a struct with x, y, z, w)
vec3 vec3_euler_angles_from_quat(quat q) {
    vec3 angles = { 0 };

    // Roll (x-axis rotation)
    float sinr_cosp = 2 * (q.w * q.x + q.y * q.z);
    float cosr_cosp = 1 - 2 * (q.x * q.x + q.y * q.y);
    angles.x = atan2f(sinr_cosp, cosr_cosp);

    // Pitch (y-axis rotation)
    float sinp = 2 * (q.w * q.y - q.z * q.x);
    if (fabsf(sinp) >= 1)
        angles.y = copysignf((float)(PI / 2), sinp); // use 90 degrees if out of range
    else
        angles.y = asinf(sinp);

    // Yaw (z-axis rotation)
    float siny_cosp = 2 * (q.w * q.z + q.x * q.y);
    float cosy_cosp = 1 - 2 * (q.y * q.y + q.z * q.z);
    angles.z = atan2f(siny_cosp, cosy_cosp);

    return angles;
}

vec3 vec3_rotate(vec3 v, quat rotation)
{
    quat v_quat = { v.x, v.y, v.z, 0.0f };

    quat q_conjugate = quat_conjugate(rotation);
    quat mult = quat_multiply(rotation, v_quat);
    quat rotated_quat = quat_multiply(mult, q_conjugate);

    return (vec3) { rotated_quat.x, rotated_quat.y, rotated_quat.z };
}