#include "pe_graphics.h"
#include "pe_graphics_linux.h"

#include "core/pe_core.h"
#include "core/pe_file_io.h"

#include "glad/glad.h"

#include "HandmadeMath.h"

#include <stdio.h>

peOpenGL pe_opengl = {0};

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
    const char *shader_line_offset = "#line 1\n";
    const char *complete_shader_source[] = { shader_version, shader_define, shader_line_offset, shader_source };
    glShaderSource(shader, PE_COUNT_OF(complete_shader_source), complete_shader_source, NULL);

    glCompileShader(shader);
    // NOTE: We don't check whether compilation succeeded or not because
    // it will be catched by the glDebugMessageCallback anyway.
    return shader;
}

static GLuint pe_shader_create_from_file(peArena *temp_arena, const char *source_file_path) {
    GLuint vertex_shader, fragment_shader;
    {
        peArenaTemp arena_temp = pe_arena_temp_begin(temp_arena);
        peFileContents shader_source_file_contents = pe_file_read_contents(temp_arena, source_file_path, true);
        vertex_shader = pe_shader_compile(GL_VERTEX_SHADER, shader_source_file_contents.data);
        fragment_shader = pe_shader_compile(GL_FRAGMENT_SHADER, shader_source_file_contents.data);
        pe_arena_temp_end(arena_temp);
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

void pe_framebuffer_size_callback_linux(int width, int height) {
    glViewport(0, 0, width, height);
    pe_opengl.framebuffer_width = width;
    pe_opengl.framebuffer_height = height;
}

void pe_graphics_init_linux(peArena *temp_arena, int window_width, int window_height) {
    pe_opengl.framebuffer_width = window_width;
    pe_opengl.framebuffer_height = window_height;

    // TODO: Confirm that glDebugMessageCallbackARB actually works
#if !defined(NDEBUG) && defined(GL_ARB_debug_output)
    if (glDebugMessageCallbackARB) {
        glDebugMessageCallbackARB((GLDEBUGPROCARB)gl_debug_message_proc, NULL);
    }
#endif

    // init shader_program
    {
        pe_opengl.shader_program = pe_shader_create_from_file(temp_arena, "res/shader.glsl");
        glUseProgram(pe_opengl.shader_program);

        GLint diffuse_map_uniform_location = glGetUniformLocation(
            pe_opengl.shader_program, "diffuse_map"
        );
        glUniform1i(diffuse_map_uniform_location, 0);
    }

    {
        // I don't expect the light vector to be relevant
        // anytime soon so lets just set it here
        pe_shader_set_vec3(pe_opengl.shader_program, "light_vector", HMM_V3(0.5f, -1.0f, 0.5f));
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
    glUniformMatrix4fv(uniform_location, 1, GL_FALSE, (void*)value);
}

void pe_shader_set_mat4_array(GLuint shader_program, const GLchar *name, HMM_Mat4 *values, GLsizei count) {
    GLint uniform_location = glGetUniformLocation(shader_program, name);
    glUniformMatrix4fv(uniform_location, count, GL_FALSE, (void*)values);
}

void pe_shader_set_bool(GLuint shader_program, const GLchar *name, bool value) {
    GLint uniform_location = glGetUniformLocation(shader_program, name);
    GLint gl_value = value ? 1 : 0;
    glUniform1i(uniform_location, gl_value);
}

peTexture pe_texture_create_linux(void *data, int width, int height, int channels) {
    GLint texture_object;
    glGenTextures(1, &texture_object);
    glBindTexture(GL_TEXTURE_2D, texture_object);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float border_color[] = { 1.0f, 0.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border_color);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    PE_ASSERT(channels == 3 || channels == 4);
    GLint gl_format = (
        channels == 3 ? GL_RGB  :
        channels == 4 ? GL_RGBA : GL_INVALID_ENUM
    );
    glTexImage2D(GL_TEXTURE_2D, 0, gl_format, width, height, 0, gl_format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    peTexture texture = { .texture_object = texture_object };
    return texture;
}