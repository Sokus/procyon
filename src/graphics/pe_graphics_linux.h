#ifndef PE_GRAPHICS_LINUX_H_HEADER_GUARD
#define PE_GRAPHICS_LINUX_H_HEADER_GUARD

#include "glad/glad.h"

#include <stdbool.h>

typedef struct peOpenGL {
    int framebuffer_width;
    int framebuffer_height;

    GLuint shader_program;
} peOpenGL;
extern peOpenGL pe_opengl;

void pe_framebuffer_size_callback_linux(int width, int height);
struct peArena;
void pe_graphics_init_linux(struct peArena *temp_arena, int window_width, int window_height);

union pVec3;
union pMat4;
void pe_shader_set_vec3(GLuint shader_program, const GLchar *name, union pVec3 value);
void pe_shader_get_mat4(GLuint shader_program, const GLchar *name, union pMat4 *value);
void pe_shader_set_mat4(GLuint shader_program, const GLchar *name, union pMat4 *value);
void pe_shader_set_mat4_array(GLuint shader_program, const GLchar *name, union pMat4 *values, GLsizei count);
void pe_shader_get_bool(GLuint shader_program, const GLchar *name, bool *value);
void pe_shader_set_bool(GLuint shader_program, const GLchar *name, bool value);
struct peTexture;
struct peTexture pe_texture_create_linux(void *data, int width, int height, int channels);
void pe_graphics_dynamic_draw_flush(void);
void pe_graphics_dynamic_draw_end_batch(void);

#endif // PE_GRAPHICS_LINUX_H_HEADER_GUARD