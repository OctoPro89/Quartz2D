#include <renderer/renderer2D.h>
#include <renderer/shaders/shader_utils.h>

#include <platform/platform.h>

// OpenGL
#include <glad/glad.h>

// Standard library
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

// My math library
#include <octomath/vec3.h>
#include <octomath/mat4.h>

typedef struct {
	f32 position[2];
	f32 color[4];
	f32 tex_coord[2];
	i32 tex_index; // -1 for invalid
} vertex;

#define MAX_QUADS 1000
#define MAX_VERTICES (MAX_QUADS * 4)
#define MAX_INDICES (MAX_QUADS * 6)
#define MAX_TEXTURE_SLOTS 8

typedef struct {
	GLuint vao, vbo, ibo;
	vertex* vertex_buffer_base;
	vertex* vertex_buffer_ptr;
	GLuint indices_count;
	GLuint texture_slots[MAX_TEXTURE_SLOTS];
	u32 texture_slot_index;

    GLuint white_texture;
    GLuint shader_program;

    i32 screen_width, screen_height;

    // Matrices
    mat4 projection;
} renderer2D_data;

renderer2D_data renderer;

u8 renderer2D_init(i32 width, i32 height) {
    renderer.vertex_buffer_base = malloc(MAX_VERTICES * sizeof(vertex));

    glGenVertexArrays(1, &renderer.vao);
    glBindVertexArray(renderer.vao);

    glGenBuffers(1, &renderer.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, renderer.vbo);
    glBufferData(GL_ARRAY_BUFFER, MAX_VERTICES * sizeof(vertex), NULL, GL_STATIC_DRAW); // fill later

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (const void*)offsetof(vertex, position));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(vertex), (const void*)offsetof(vertex, color));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (const void*)offsetof(vertex, tex_coord));

    glEnableVertexAttribArray(3);
    glVertexAttribIPointer(3, 1, GL_INT, sizeof(vertex), (const void*)offsetof(vertex, tex_index));

    // Index buffer setup
    GLuint* indices = malloc(MAX_INDICES * sizeof(GLuint));
    for (int i = 0, offset = 0; i < MAX_INDICES; i += 6, offset += 4) {
        indices[i] = offset;
        indices[i + 1] = offset + 1;
        indices[i + 2] = offset + 2;
        indices[i + 3] = offset + 2;
        indices[i + 4] = offset + 3;
        indices[i + 5] = offset + 0;
    }

    glGenBuffers(1, &renderer.ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderer.ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, MAX_INDICES * sizeof(GLuint), indices, GL_STATIC_DRAW);
    free(indices);

    // White texture
    glGenTextures(1, &renderer.white_texture);
    glBindTexture(GL_TEXTURE_2D, renderer.white_texture);
    uint32_t white = 0xFFFFFFFF;
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &white);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    renderer.texture_slots[0] = renderer.white_texture;

    // Shaders & program

    // Load in shaders from their respective files
    char* vertex_shader_source = read_shader_source("D:/Programming/C/Quartz2D/Quartz2D/src/renderer/shaders/vertex_shader.glsl");
    if (vertex_shader_source == NULL) {
        renderer2D_shutdown();
        return false;
    }

    char* fragment_shader_source = read_shader_source("D:/Programming/C/Quartz2D/Quartz2D/src/renderer/shaders/fragment_shader.glsl");
    if (fragment_shader_source == NULL) {
        renderer2D_shutdown();
        return false;
    }

    renderer.shader_program = create_shader_program(vertex_shader_source, fragment_shader_source);
    glUseProgram(renderer.shader_program);

    // Free up memory allocated earlier by read_shader_source because we don't need it anymore
    free_shader_source(vertex_shader_source);
    free_shader_source(fragment_shader_source);

    // Setup u_Textures[0..7]
    GLint samplers[8] = { 0,1,2,3,4,5,6,7 };
    glUniform1iv(glGetUniformLocation(renderer.shader_program, "u_Textures"), 8, samplers);

    renderer.screen_width = width;
    renderer.screen_height = height;

    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glViewport(0, 0, renderer.screen_width, renderer.screen_height);

    renderer.projection = mat4_orthographic_rh((f32)renderer.screen_width, (f32)renderer.screen_height, 0.001f, 100.0f);

    // Upload it to the shader
    GLint loc = glGetUniformLocation(renderer.shader_program, "uProjection");
    glUseProgram(renderer.shader_program);
    glUniformMatrix4fv(loc, 1, GL_FALSE, &renderer.projection.r[0][0]);

    // Turn on blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    return true;
}

void renderer2D_draw_quad(f32 x, f32 y, f32 width, f32 height, i32 texture_slot, color4 color) {
    if (renderer.indices_count >= MAX_INDICES) {
        renderer2D_flush(); // If we add more indices now, we will have more than the max allowed, so flush, then continue
    }

    i32 tex_index = -1; // default to white texture

    if (texture_slot != -1) {
        for (uint32_t i = 1; i < renderer.texture_slot_index; i++) {
            if (renderer.texture_slots[i] == texture_slot) {
                tex_index = i;
                break;
            }
        }

        if (tex_index == -1) {
            if (renderer.texture_slot_index >= MAX_TEXTURE_SLOTS) {
                renderer2D_flush();
                renderer2D_begin_batch();
            }

            tex_index = (i32)renderer.texture_slot_index;
            renderer.texture_slots[renderer.texture_slot_index++] = (GLuint)texture_slot;
        }
    }

    f32 x_offset = -renderer.screen_width / 2.0f;
    f32 y_offset = -renderer.screen_height / 2.0f;

    f32 positions[4][2] = {
        {x + x_offset, y + y_offset},
        {x + width + x_offset, y + y_offset},
        {x + width + x_offset, y + height + y_offset},
        {x + x_offset, y + height + y_offset}
    };

    f32 tex_coords[4][2] = {
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
        {0.0f, 1.0f}
    };

    for (int i = 0; i < 4; i++) {
        renderer.vertex_buffer_ptr->position[0] = positions[i][0];
        renderer.vertex_buffer_ptr->position[1] = positions[i][1];
        memcpy(renderer.vertex_buffer_ptr->color, &color.r, 4 * sizeof(f32));
        memcpy(renderer.vertex_buffer_ptr->tex_coord, tex_coords[i], 2 * sizeof(f32));
        renderer.vertex_buffer_ptr->tex_index = tex_index;
        renderer.vertex_buffer_ptr++;
    }

    renderer.indices_count += 6;
}

void renderer2D_begin_batch() {
    renderer.vertex_buffer_ptr = renderer.vertex_buffer_base;
    renderer.indices_count = 0;
    renderer.texture_slot_index = 1; // Slot 0 is white texture
}

void renderer2D_end_batch() {
    size_t size = (uint8_t*)renderer.vertex_buffer_ptr - (uint8_t*)renderer.vertex_buffer_base;
    glBindBuffer(GL_ARRAY_BUFFER, renderer.vbo);
    glBufferData(GL_ARRAY_BUFFER, MAX_VERTICES * sizeof(vertex), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, size, renderer.vertex_buffer_base);
}

void renderer2D_flush() {
    glClear(GL_COLOR_BUFFER_BIT);
    if (renderer.indices_count == 0) {
        return; // Return from function because there is nothing to draw
    }

    glUseProgram(renderer.shader_program); // Use the shaders from earlier in drawing

    glBindVertexArray(renderer.vao); // Use the quad vertices in the buffer

    // Bind white texture to slot 0 always
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, renderer.white_texture);

    for (uint32_t i = 1; i < renderer.texture_slot_index; i++) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, renderer.texture_slots[i]);
    }

    glDrawElements(GL_TRIANGLES, renderer.indices_count, GL_UNSIGNED_INT, NULL); // Draw the quads using the shaders
    platform_swap_buffers(); // Swap buffers to display new stuff
}

void renderer2D_shutdown() {
    // Free up all allocated resources
    glDeleteBuffers(1, &renderer.vbo);
    glDeleteBuffers(1, &renderer.ibo);
    glDeleteVertexArrays(1, &renderer.vao);
    glDeleteTextures(1, &renderer.white_texture);
    glDeleteProgram(renderer.shader_program);

    if (renderer.vertex_buffer_base) {
        free(renderer.vertex_buffer_base);
    }
}