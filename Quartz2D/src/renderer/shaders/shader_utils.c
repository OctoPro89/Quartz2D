#include <renderer/shaders/shader_utils.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

GLuint compile_shader(GLenum type, const char* src) {
    GLuint id = glCreateShader(type);
    glShaderSource(id, 1, &src, NULL);
    glCompileShader(id);

    int success;
    glGetShaderiv(id, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLog(id, 512, NULL, log);
        fprintf(stderr, "Shader compilation error: %s\n", log);
    }

    return id;
}

GLuint create_shader_program(const char* vertex_src, const char* fragment_src) {
    GLuint program = glCreateProgram();
    GLuint vs = compile_shader(GL_VERTEX_SHADER, vertex_src);
    GLuint fs = compile_shader(GL_FRAGMENT_SHADER, fragment_src);

    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    int success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char log[512];
        glGetProgramInfoLog(program, 512, NULL, log);
        fprintf(stderr, "Shader linking error: %s\n", log);
    }

    glDeleteShader(vs);
    glDeleteShader(fs);

    return program;
}

char* read_shader_source(const char* filepath) {
    char* source = NULL;
    FILE* fp = fopen(filepath, "r");
    if (fp != NULL) {
        // Seek to the send of the file
        if (fseek(fp, 0L, SEEK_END) == 0) {
            /* Get the size of the file. */
            long buffer_size = ftell(fp);
            if (buffer_size == -1) { return NULL; } // Failed to get file size

            // Allocate memory for the size of the file + 1 for null termination
            source = malloc(sizeof(char) * (buffer_size + 1));

            // Go back to the start of the file
            if (fseek(fp, 0L, SEEK_SET) != 0) {
                free_shader_source(source);
                return NULL; // Failed to get file size
            }

            // Read the content of the file into the allocated memory
            size_t new_len = fread(source, sizeof(char), buffer_size, fp);
            if (ferror(fp) != 0) {
                free_shader_source(source);
                return NULL; // Failed to read file
            }
            else {
                source[new_len++] = '\0'; // Null terminate to be safe
            }
        }

        fclose(fp);
        return source;
    }
    else {
        return NULL; // Failed to open file
    }
}

void free_shader_source(char* src) {
    if (src) {
        free(src);
    }
}