#ifndef PE_GRAPHICS_LINUX_H_HEADER_GUARD
#define PE_GRAPHICS_LINUX_H_HEADER_GUARD

#include "glad/glad.h"

typedef struct peOpenGL {
    GLuint shader_program;
} peOpenGL;
extern peOpenGL pe_opengl;

void pe_graphics_init_linux(void);

union HMM_Vec3;
union HMM_Mat4;
void pe_shader_set_vec3(GLuint shader_program, const GLchar *name, union HMM_Vec3 value);
void pe_shader_set_mat4(GLuint shader_program, const GLchar *name, union HMM_Mat4 *value);

struct peTexture;
struct peTexture pe_texture_create_linux(void *data, int width, int height, int channels);

#endif // PE_GRAPHICS_LINUX_H_HEADER_GUARD