#pragma once

#include <common.h>

typedef struct {
	u32 width;
	u32 height;
	u32 bpp;
	u8* data;
} tga_image;

tga_image tga_import(const char* filename);
void tga_free(tga_image* image);