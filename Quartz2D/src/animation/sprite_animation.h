#pragma once

#include <common.h>
#include <renderer/texture_atlas.h>

typedef struct {
    i32 frame_index;     // Index into atlas->uvs[]
    f32 duration;        // How long the current frame should last (in seconds)
} animation_frame;

typedef struct {
    animation_frame* frames;
    i32 frame_count;
    u8 looping;
} sprite_animation;

typedef struct {
    sprite_animation* animation;
    i32 current_frame;
    f32 time_accumulator;
} animation_state;

typedef struct {
    texture_atlas atlas;
    animation_state anim_state;
} animated_sprite;

void animation_state_update(animation_state* state, f32 delta_time);