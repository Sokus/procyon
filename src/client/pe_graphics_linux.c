#include "pe_graphics_linux.h"

#include "pe_core.h"
#include "pe_window_glfw.h"
#include "pe_file_io.h"

#include "glad/glad.h"

#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"

#include "HandmadeMath.h"

#include <stdio.h>

peOpenGL pe_opengl = {0};

static void glfw_framebuffer_size_proc(GLFWwindow *window, int width, int height) {
    glViewport(0, 0, width, height);
    pe_glfw.window_width = width;
    pe_glfw.window_height = height;
}

static const char *gl_debug_message_source_to_string(GLenum source) {
    switch (source) {
        case GL_DEBUG_SOURCE_API_ARB:               return "GL_DEBUG_SOURCE_API";
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB:     return "GL_DEBUG_SOURCE_WINDOW_SYSTEM";
        case GL_DEBUG_SOURCE_SHADER_COMPILER_ARB:   return "GL_DEBUG_SOURCE_SHADER_COMPILE";
        case GL_DEBUG_SOURCE_THIRD_PARTY_ARB:       return "GL_DEBUG_SOURCE_THIRD_PARTY";
        case GL_DEBUG_SOURCE_APPLICATION_ARB:       return "GL_DEBUG_SOURCE_APPLICATION";
        case GL_DEBUG_SOURCE_OTHER_ARB:             return "GL_DEBUG_SOURCE_OTHER";
        default:                                    break;
    }
    return "GL_DEBUG_SOURCE_???";
}

static const char *gl_debug_message_type_to_string(GLenum type) {
    switch (type) {
        case GL_DEBUG_TYPE_ERROR_ARB:               return "GL_DEBUG_TYPE_ERROR";
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB: return "GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR";
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB:  return "GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR";
        case GL_DEBUG_TYPE_PORTABILITY_ARB:         return "GL_DEBUG_TYPE_PORTABILITY";
        case GL_DEBUG_TYPE_PERFORMANCE_ARB:         return "GL_DEBUG_TYPE_PERFORMANCE";
        case GL_DEBUG_TYPE_OTHER_ARB:               return "GL_DEBUG_TYPE_OTHER";
        default:                                    break;
    }
    return "GL_DEBUG_TYPE_???";
}

static const char *gl_debug_message_severity_to_string(GLenum severity) {
    switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH_ARB:    return "GL_DEBUG_SEVERITY_HIGH";
        case GL_DEBUG_SEVERITY_MEDIUM_ARB:  return "GL_DEBUG_SEVERITY_MEDIUM";
        case GL_DEBUG_SEVERITY_LOW_ARB:     return "GL_DEBUG_SEVERITY_LOW";
        default:                            break;
    }
    return "GL_DEBUG_SEVERITY_???";
}

static void gl_debug_message_proc(
    GLenum source, GLenum type, GLuint id, GLenum severity,
    GLsizei length, const GLchar *message, const void *userParam
) {
    // ignore non-significant error/warning codes
    if (id == 131169 || id == 131185 || id == 131218 || id == 131204) {
        return;
    }

    printf(
        "GL_DEBUG [%d], %s [0x%x], %s [0x%x], %s [0x%x]:\n%s\n",
        id,
        gl_debug_message_source_to_string(source), source,
        gl_debug_message_type_to_string(type), type,
        gl_debug_message_severity_to_string(severity), severity,
        message
    );
}


static const char *gl_shader_type_string(GLenum type) {
    switch (type) {
        case GL_VERTEX_SHADER: return "GL_VERTEX_SHADER";
        case GL_FRAGMENT_SHADER: return "GL_FRAGMENT_SHADER";
        default: return "???";
    }
}

static GLuint pe_shader_compile(GLenum type, const GLchar *shader_source) {
    GLuint shader = glCreateShader(type);

    const char *shader_version = "#version 420 core\n";
    const char *shader_define = (
        type == GL_VERTEX_SHADER   ? "#define PE_VERTEX_SHADER 1\n"   :
        type == GL_FRAGMENT_SHADER ? "#define PE_FRAGMENT_SHADER 1\n" : ""
    );
    const char *complete_shader_source[] = { shader_version, shader_define, shader_source };
    glShaderSource(shader, PE_COUNT_OF(complete_shader_source), complete_shader_source, NULL);

    glCompileShader(shader);
    // NOTE: We don't check whether compilation succeeded or not because
    // it will be catched by the glDebugMessageCallback anyway.
    return shader;
}

static GLuint pe_shader_create_from_file(const char *source_file_path) {
    GLuint vertex_shader, fragment_shader;
    {
        peFileContents shader_source_file_contents = pe_file_read_contents(pe_heap_allocator(), source_file_path, true);
        vertex_shader = pe_shader_compile(GL_VERTEX_SHADER, shader_source_file_contents.data);
        fragment_shader = pe_shader_compile(GL_FRAGMENT_SHADER, shader_source_file_contents.data);
        pe_file_free_contents(shader_source_file_contents);
    }

    GLuint shader_program = glCreateProgram();
    {
        glAttachShader(shader_program, vertex_shader);
        glAttachShader(shader_program, fragment_shader);
        glLinkProgram(shader_program);
        GLint success;
        char info_log[512];
        glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader_program, 512, NULL,  info_log);
            fprintf(stderr, "Linking error:\n%s\n", info_log);
        }
    }

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    return shader_program;
}

void pe_graphics_init_linux(void) {
    int window_width, window_height;

    // init GLFW
    {
        PE_ASSERT(pe_glfw.initialized);
        gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
        glfwGetWindowSize(pe_glfw.window, &window_width, &window_height);
    }

    // TODO: Confirm that glDebugMessageCallbackARB actually works
#if !defined(NDEBUG) && defined(GL_ARB_debug_output)
    if (glDebugMessageCallbackARB) {
        glDebugMessageCallbackARB((GLDEBUGPROCARB)gl_debug_message_proc, NULL);
    }
#endif

    // init shader_program
    {
        pe_opengl.shader_program = pe_shader_create_from_file("res/shader.glsl");
        glUseProgram(pe_opengl.shader_program);
    }

    glViewport(0, 0, window_width, window_height);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
}

void pe_shader_set_vec3(GLuint shader_program, const GLchar *name, HMM_Vec3 value) {
    GLint uniform_location = glGetUniformLocation(shader_program, name);
    glUniform3fv(uniform_location, 1, value.Elements);
}

void pe_shader_set_mat4(GLuint shader_program, const GLchar *name, HMM_Mat4 *value) {
    GLint uniform_location = glGetUniformLocation(shader_program, name);
    glUniformMatrix4fv(uniform_location, 1, false, value->Elements[0]);
}