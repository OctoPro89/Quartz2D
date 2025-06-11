#pragma once

#include <common.h>

typedef struct {
    f32 u0, v0;
    f32 u1, v1;
} uv_rect;

typedef struct {
    i32 texture_id;
    i32 sprite_width;
    i32 sprite_height;
    i32 atlas_width;
    i32 atlas_height;
    i32 sprite_count;
    uv_rect* uvs; // Array of sprite_count rects
} texture_atlas;