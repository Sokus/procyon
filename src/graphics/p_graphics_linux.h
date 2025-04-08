#ifndef P_GRAPHICS_LINUX_H_HEADER_GUARD
#define P_GRAPHICS_LINUX_H_HEADER_GUARD

#include <stdbool.h>

typedef unsigned int GLuint;
typedef unsigned int GLenum;
typedef int GLsizei;
typedef char GLchar;

typedef struct pOpenGL {
    int framebuffer_width;
    int framebuffer_height;

    GLuint shader_program;
} pOpenGL;
extern pOpenGL p_opengl;

typedef struct pDynamicDrawStateOpenGL {
    GLuint vertex_array_object;
    GLuint vertex_buffer_object;
} pDynamicDrawStateOpenGL;
extern pDynamicDrawStateOpenGL dynamic_draw_opengl;

void gl_debug_message_proc(
    GLenum source, GLenum type, GLuint id, GLenum severity,
    GLsizei length, const GLchar *message, const void *userParam
);

typedef struct pArena pArena;
GLuint p_shader_create_from_file(const char *source_file_path);

typedef union pVec3 pVec3;
typedef union pMat4 pMat4;
void p_shader_set_vec3(GLuint shader_program, const GLchar *name, pVec3 value);
void p_shader_get_mat4(GLuint shader_program, const GLchar *name, pMat4 *value);
void p_shader_set_mat4(GLuint shader_program, const GLchar *name, pMat4 *value);
void p_shader_set_mat4_array(GLuint shader_program, const GLchar *name, pMat4 *values, GLsizei count);
void p_shader_get_bool(GLuint shader_program, const GLchar *name, bool *value);
void p_shader_set_bool(GLuint shader_program, const GLchar *name, bool value);

#endif // P_GRAPHICS_LINUX_H_HEADER_GUARD
