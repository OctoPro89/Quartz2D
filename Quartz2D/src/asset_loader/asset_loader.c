#include <asset_loader/asset_loader.h>
#include <asset_loader/tga_loader.h>

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
    if (tex_id != 0) {
        glDeleteTextures(1, (GLuint*)&tex_id);
    }
}