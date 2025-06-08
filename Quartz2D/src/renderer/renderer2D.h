#pragma once
#include <common.h>
#include <renderer/color.h>

u8 renderer2D_init(i32 width, i32 height);
void renderer2D_begin_batch();
void renderer2D_draw_quad(f32 x, f32 y, f32 width, f32 height, i32 texture_slot, color4 color);
void renderer2D_end_batch();
void renderer2D_flush();
void renderer2D_shutdown();