#pragma once
#include <common.h>
#include <glad/glad.h>

GLuint compile_shader(GLenum type, const char* src);
GLuint create_shader_program(const char* vertex_src, const char* fragment_src);
char* read_shader_source(const char* filepath);
void free_shader_source(char* src);