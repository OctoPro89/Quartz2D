#pragma once

#include <common.h>

typedef struct {
    i32 texture_id;
    i32 glyph_width;
    i32 glyph_height;
    i32 atlas_columns;
    i32 atlas_rows;
    f32 space_width;
    f32 kerning;
} bitmap_font;