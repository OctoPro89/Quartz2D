#include <asset_loader/asset_loader.h>
#include <asset_loader/tga_loader.h>

#include <stdlib.h>
#include <string.h>

i32 asset_loader_load_texture_from_tga(const char* filepath) {
    tga_image image = tga_import(filepath);
    if (image.data == NULL) {
        return -1;
    }

    GLuint tex_id = 0;
    glGenTextures(1, &tex_id);
    glBindTexture(GL_TEXTURE_2D, tex_id);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, image.width, image.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.data);

    // Basic filtering and wrapping
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Generate mipmaps
    glGenerateMipmap(GL_TEXTURE_2D);

    tga_free(&image);
    return (i32)tex_id;
}

void asset_loader_destroy_texture(i32 tex_id) {
    if (tex_id != -1) {
        glDeleteTextures(1, (GLuint*)&tex_id);
    }
}

texture_atlas asset_loader_load_texture_atlas_from_tga(const char* filepath, i32 sprite_width, i32 sprite_height, i32 expected_sprite_count) {
    tga_image image = tga_import(filepath);
    if (image.data == NULL) {
        return (texture_atlas) { .texture_id = -1 };
    }

    GLuint tex_id = 0;
    glGenTextures(1, &tex_id);
    glBindTexture(GL_TEXTURE_2D, tex_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, image.width, image.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenerateMipmap(GL_TEXTURE_2D);

    // Compute UVs
    i32 cols = image.width / sprite_width;
    i32 rows = image.height / sprite_height;
    i32 max_sprites = cols * rows;

    i32 count = (expected_sprite_count > 0 && expected_sprite_count < max_sprites) ? expected_sprite_count : max_sprites;

    uv_rect* rects = malloc(sizeof(uv_rect) * count);

    for (i32 i = 0; i < count; ++i) {
        i32 col = i % cols;
        i32 row = i / cols;

        f32 u0 = (f32)(col * sprite_width) / image.width;
        f32 v0 = (f32)(row * sprite_height) / image.height;
        f32 u1 = (f32)((col + 1) * sprite_width) / image.width;
        f32 v1 = (f32)((row + 1) * sprite_height) / image.height;

        rects[i] = (uv_rect){ u0, v0, u1, v1 };
    }

    tga_free(&image);

    return (texture_atlas) {
        .texture_id = (i32)tex_id,
            .sprite_width = sprite_width,
            .sprite_height = sprite_height,
            .atlas_width = image.width,
            .atlas_height = image.height,
            .sprite_count = count,
            .uvs = rects
    };
}

void asset_loader_destroy_texture_atlas(texture_atlas* atlas) {
    if (atlas && atlas->texture_id != -1) {
        glDeleteTextures(1, (GLuint*)&atlas->texture_id);
        free(atlas->uvs);
        atlas->texture_id = -1;
    }
}