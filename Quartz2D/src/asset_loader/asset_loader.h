#pragma once

#include <common.h>

#include <renderer/texture_atlas.h>

#include <glad/glad.h>

i32 asset_loader_load_texture_from_tga(const char* filepath);
void asset_loader_destroy_texture(i32 tex_id);

texture_atlas asset_loader_load_texture_atlas_from_tga(const char* filepath,i32 sprite_width,i32 sprite_height, i32 expected_sprite_count);
void asset_loader_destroy_texture_atlas(texture_atlas* atlas);