#include "../include/octomath/mat4.h"

// Create an identity matrix
mat4 mat4_identity() {
    mat4 result = { 0 };
    for (int i = 0; i < 4; i++) {
        result.r[i][i] = 1.0f;
    }
    return result;
}

// Multiply two matrices
mat4 mat4_multiply(const mat4* m1, const mat4* m2) {
    mat4 result = { 0 };
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            for (int k = 0; k < 4; k++) {
                result.r[i][j] += m1->r[i][k] * m2->r[k][j];
            }
        }
    }
    return result;
}

// Affine transformation matrix
mat4 mat4_affine_transformation(vec3 scaling, vec3 rotation_origin, quat rotation_quat, vec3 translation) {
    mat4 scale = mat4_identity();
    scale.r[0][0] = scaling.x;
    scale.r[1][1] = scaling.y;
    scale.r[2][2] = scaling.z;

    // Rotation matrix from quaternion
    float xx = rotation_quat.x * rotation_quat.x;
    float yy = rotation_quat.y * rotation_quat.y;
    float zz = rotation_quat.z * rotation_quat.z;
    float xy = rotation_quat.x * rotation_quat.y;
    float xz = rotation_quat.x * rotation_quat.z;
    float yz = rotation_quat.y * rotation_quat.z;
    float wx = rotation_quat.w * rotation_quat.x;
    float wy = rotation_quat.w * rotation_quat.y;
    float wz = rotation_quat.w * rotation_quat.z;

    mat4 rotation = mat4_identity();
    rotation.r[0][0] = 1 - 2 * (yy + zz);
    rotation.r[0][1] = 2 * (xy - wz);
    rotation.r[0][2] = 2 * (xz + wy);
    rotation.r[1][0] = 2 * (xy + wz);
    rotation.r[1][1] = 1 - 2 * (xx + zz);
    rotation.r[1][2] = 2 * (yz - wx);
    rotation.r[2][0] = 2 * (xz - wy);
    rotation.r[2][1] = 2 * (yz + wx);
    rotation.r[2][2] = 1 - 2 * (xx + yy);

    mat4 translation_matrix = mat4_identity();
    translation_matrix.r[0][3] = translation.x;
    translation_matrix.r[1][3] = translation.y;
    translation_matrix.r[2][3] = translation.z;

    mat4 multiplied = mat4_multiply(&rotation, &scale);
    return mat4_multiply(&translation_matrix, &multiplied);
}

// LookAt matrix (right-handed)
mat4 mat4_lookat_rh(vec3 eye_position, vec3 focus_position, vec3 up) {
    vec3 z_axis = vec3_normalize(vec3_subtract(eye_position, focus_position)); // Forward vector (negative look direction)
    vec3 x_axis = vec3_normalize(vec3_cross(up, z_axis));                      // Right vector
    vec3 y_axis = vec3_cross(z_axis, x_axis);                                  // Recalculated Up vector

    mat4 result = mat4_identity();
    result.r[0][0] = x_axis.x; result.r[0][1] = y_axis.x; result.r[0][2] = z_axis.x; result.r[0][3] = 0.0f;
    result.r[1][0] = x_axis.y; result.r[1][1] = y_axis.y; result.r[1][2] = z_axis.y; result.r[1][3] = 0.0f;
    result.r[2][0] = x_axis.z; result.r[2][1] = y_axis.z; result.r[2][2] = z_axis.z; result.r[2][3] = 0.0f;

    result.r[3][0] = -vec3_dot(x_axis, eye_position);
    result.r[3][1] = -vec3_dot(y_axis, eye_position);
    result.r[3][2] = -vec3_dot(z_axis, eye_position);
    result.r[3][3] = 1.0f;

    return result;
}

mat4 mat4_lookto_rh(vec3 eye_position, vec3 direction, vec3 up)
{
    mat4 result = { 0 };

    vec3 z = vec3_multiply_scalar(vec3_normalize(direction), -1.0f); // RH -> reverse forward
    vec3 x = vec3_normalize(vec3_cross(up, z)); // Right
    vec3 y = vec3_cross(z, x); // Up

    result.r[0][0] = x.x; result.r[1][0] = x.y; result.r[2][0] = x.z; result.r[3][0] = -vec3_dot(x, eye_position);
    result.r[0][1] = y.x; result.r[1][1] = y.y; result.r[2][1] = y.z; result.r[3][1] = -vec3_dot(y, eye_position);
    result.r[0][2] = z.x; result.r[1][2] = z.y; result.r[2][2] = z.z; result.r[3][2] = -vec3_dot(z, eye_position);
    result.r[0][3] = 0;   result.r[1][3] = 0;   result.r[2][3] = 0;   result.r[3][3] = 1;

    return result;
}

// Perspective projection matrix (right-handed)
mat4 mat4_perspective_fov_rh(float fov_y, float aspect_ratio, float near_z, float far_z) {
    float f = 1.0f / tanf(fov_y * 0.5f);
    mat4 result = { 0 };

    result.r[0][0] = f / aspect_ratio;
    result.r[1][1] = f;
    result.r[2][2] = far_z / (near_z - far_z);
    result.r[2][3] = -1.0f;
    result.r[3][2] = (near_z * far_z) / (near_z - far_z);

    return result;
}

// Orthographic projection matrix (right-handed)
mat4 mat4_orthographic_rh(float view_width, float view_height, float near_z, float far_z) {
    mat4 result = mat4_identity();

    result.r[0][0] = 2.0f / view_width;
    result.r[1][1] = 2.0f / view_height;
    result.r[2][2] = 1.0f / (near_z - far_z);
    result.r[2][3] = near_z / (near_z - far_z);

    return result;
}

// Inverse of a matrix
mat4 mat4_inverse(const mat4* m) {
    mat4 result = { 0 };
    float temp[4][8] = { 0 };

    // Initialize augmented matrix with input matrix and identity matrix
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            temp[i][j] = m->r[i][j];
            temp[i][j + 4] = (i == j) ? 1.0f : 0.0f;
        }
    }

    // Perform Gaussian elimination
    for (int i = 0; i < 4; i++) {
        float pivot = temp[i][i];
        if (fabsf(pivot) < 1e-6f) {
            // Singular matrix
            return result;
        }

        // Normalize the pivot row
        for (int j = 0; j < 8; j++) {
            temp[i][j] /= pivot;
        }

        // Eliminate other rows
        for (int k = 0; k < 4; k++) {
            if (k != i) {
                float factor = temp[k][i];
                for (int j = 0; j < 8; j++) {
                    temp[k][j] -= factor * temp[i][j];
                }
            }
        }
    }

    // Extract the inverse matrix
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            result.r[i][j] = temp[i][j + 4];
        }
    }

    return result;
}

mat4 mat4_create_xr_projection(float left, float right, float up, float down, float near_z, float far_z)
{
    float tan_l = tanf(left);
    float tan_r = tanf(right);
    float tan_up = tanf(up);
    float tan_down = tanf(down);

    float tan_width = tan_r - tan_l;
    float tan_height = tan_up - tan_down;

    mat4 result = { 0 };

    result.r[0][0] = 2.0f / tan_width;
    result.r[0][1] = 0.0f;
    result.r[0][2] = (tan_r + tan_l) / tan_width;
    result.r[0][3] = 0.0f;

    result.r[1][0] = 0.0f;
    result.r[1][1] = 2.0f / tan_height;
    result.r[1][2] = (tan_up + tan_down) / tan_height;
    result.r[1][3] = 0.0f;

    result.r[2][0] = 0.0f;
    result.r[2][1] = 0.0f;
    result.r[2][2] = -(far_z) / (far_z - near_z);
    result.r[2][3] = -(far_z * (near_z)) / (far_z - near_z);

    result.r[3][0] = 0.0f;
    result.r[3][1] = 0.0f;
    result.r[3][2] = -1.0f;
    result.r[3][3] = 0.0f;

    return result;
}