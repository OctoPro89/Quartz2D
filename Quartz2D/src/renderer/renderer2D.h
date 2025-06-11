#pragma once
#include <common.h>
#include <renderer/color.h>
#include <renderer/texture_atlas.h>
#include <renderer/bitmap_font.h>
#include <animation/sprite_animation.h>

u8 renderer2D_init(i32 width, i32 height);
void renderer2D_begin_batch();
void renderer2D_draw_quad(f32 x, f32 y, f32 width, f32 height, i32 texture_slot, color4 color, f32 z);
void renderer2D_draw_quad_atlas(f32 x, f32 y, f32 width, f32 height, i32 texture_slot, const uv_rect* rect, color4 color, f32 z);
void renderer2D_draw_rotated_quad_atlas(f32 x, f32 y, f32 width, f32 height, i32 texture_slot, const uv_rect* rect, color4 color, f32 rotation_rad, f32 z);
void renderer2D_draw_rotated_quad(f32 x, f32 y, f32 width, f32 height, i32 texture_slot, color4 color, f32 rotation_rad, f32 z);
void renderer2D_draw_animated_sprite(f32 x, f32 y, f32 width, f32 height, const animated_sprite* sprite, color4 color, f32 rotation_rad, f32 z);
void renderer2D_draw_bitmap_text(f32 x, f32 y, f32 font_size, const char* text, const bitmap_font* font, color4 color, f32 z);
void renderer2D_end_batch();
void renderer2D_flush();
void renderer2D_shutdown();