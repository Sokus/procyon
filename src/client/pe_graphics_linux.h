#ifndef PE_GRAPHICS_LINUX_H_HEADER_GUARD
#define PE_GRAPHICS_LINUX_H_HEADER_GUARD

#include "HandmadeMath.h"

#include "glad/glad.h"

typedef struct peOpenGL {
    GLuint shader_program;
} peOpenGL;
extern peOpenGL pe_opengl;

void pe_graphics_init_linux(void);

void pe_shader_set_vec3(GLuint shader_program, const GLchar *name, HMM_Vec3 value);
void pe_shader_set_mat4(GLuint shader_program, const GLchar *name, HMM_Mat4 *value);

#endif // PE_GRAPHICS_LINUX_H_HEADER_GUARD