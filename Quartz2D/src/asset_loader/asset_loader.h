#pragma once

#include <common.h>
#include <glad/glad.h>

i32 asset_loader_load_texture_from_tga(const char* filepath);
void asset_loader_destroy_texture(i32 tex_id);