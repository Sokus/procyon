#include "pe_graphics.h"
#include "pe_graphics_linux.h"

#include "core/pe_core.h"
#include "core/pe_file_io.h"
#include "math/p_math.h"
#include "platform/pe_window.h"
#include "utility/pe_trace.h"

#include "glad/glad.h"

#include <stdio.h>
#include <stddef.h>
#include <string.h>

peOpenGL pe_opengl = {0};
peDynamicDrawStateOpenGL dynamic_draw_opengl = {0};

peTexture default_texture;

//
// GENERAL IMPLEMENTATIONS
//

void pe_graphics_init(peArena *temp_arena, int window_width, int window_height) {
    pe_opengl.framebuffer_width = window_width;
    pe_opengl.framebuffer_height = window_height;

    gladLoadGLLoader((GLADloadproc)&pe_window_get_proc_address);

#if !defined(NDEBUG) && defined(GL_ARB_debug_output)
    if (glDebugMessageCallbackARB) {
        glDebugMessageCallbackARB((GLDEBUGPROCARB)gl_debug_message_proc, NULL);
    }
#elif !defined(NDEBUG) && !defined(GL_ARB_debug_output)
    printf("GL: DebugMessageCallback not available!\n");
#endif

    // init gl
    glViewport(0, 0, window_width, window_height);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // init shader_program
    pe_opengl.shader_program = pe_shader_create_from_file(temp_arena, "res/shader.glsl");
    glUseProgram(pe_opengl.shader_program);

    // init dynamic draw
    {
        glGenVertexArrays(1, &dynamic_draw_opengl.vertex_array_object);
        glBindVertexArray(dynamic_draw_opengl.vertex_array_object);

        glGenBuffers(1, &dynamic_draw_opengl.vertex_buffer_object);
        glBindBuffer(GL_ARRAY_BUFFER, dynamic_draw_opengl.vertex_buffer_object);
        glBufferData(GL_ARRAY_BUFFER, sizeof(dynamic_draw.vertex), NULL, GL_DYNAMIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(peDynamicDrawVertex), (void*)offsetof(peDynamicDrawVertex, position));
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(peDynamicDrawVertex), (void*)offsetof(peDynamicDrawVertex, normal));
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(peDynamicDrawVertex), (void*)offsetof(peDynamicDrawVertex, texcoord));
        glVertexAttribPointer(3, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(peDynamicDrawVertex), (void*)offsetof(peDynamicDrawVertex, color));

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);
        glEnableVertexAttribArray(3);
        glDisableVertexAttribArray(4); // NOTE: we don't enable bone related
        glDisableVertexAttribArray(5); //       attributes for dynamic drawing

        glBindVertexArray(0);
    }

    // init matrices
    {
        pe_graphics.mode = peGraphicsMode_2D;
        pe_graphics.matrix_mode = peMatrixMode_Projection;

        for (int gm = 0; gm < peGraphicsMode_Count; gm += 1) {
            for (int mm = 0; mm < peMatrixMode_Count; mm += 1) {
                pe_graphics.matrix[gm][mm] = p_mat4_i();
                pe_graphics.matrix_dirty[gm][mm] = true;
            }
            pe_graphics.matrix_model_is_identity[gm] = true;
        }

        pe_graphics.matrix[peGraphicsMode_2D][peMatrixMode_Projection] = pe_matrix_orthographic(
            0, (float)window_width,
            (float)window_height, 0,
            0.0f, 1000.0f
        );

        pe_graphics_matrix_update();
    }

    // init shader defaults
    {
        pe_graphics_set_lighting(true);
        pe_graphics_set_light_vector(PE_LIGHT_VECTOR_DEFAULT);
    }

    // init default texture
	{
		uint32_t texture_data[] = { 0xFFFFFFFF };
		default_texture = pe_texture_create(temp_arena, texture_data, 1, 1);
	}

    pe_window_set_framebuffer_size_callback(&pe_framebuffer_size_callback_linux);
}

void pe_graphics_shutdown(void) {
    // do nothing (we only need this for the PSP)
}

int pe_screen_width(void) {
    return pe_opengl.framebuffer_width;
}

int pe_screen_height(void) {
    return pe_opengl.framebuffer_height;
}

void pe_clear_background(peColor color) {
    pVec4 v = pe_color_to_vec4(color);
	glClearColor(v.r, v.g, v.b, v.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void pe_graphics_frame_begin(void) {
    pe_graphics.matrix[peGraphicsMode_2D][peMatrixMode_Projection] = pe_matrix_orthographic(
        0, (float)pe_screen_width(),
        (float)pe_screen_height(), 0,
        0.0f, 1000.0f
    );
    pe_graphics_matrix_update();
}

void pe_graphics_frame_end(bool vsync) {
	PE_TRACE_FUNCTION_BEGIN();
	pe_graphics_dynamic_draw_draw_batches();
	pe_window_swap_buffers(vsync);
	pe_graphics_dynamic_draw_clear();
	PE_TRACE_FUNCTION_END();
}

void pe_graphics_set_depth_test(bool enable) {
    if (enable) {
        glEnable(GL_DEPTH_TEST);
    } else {
        glDisable(GL_DEPTH_TEST);
    }
}

void pe_graphics_set_lighting(bool enable) {
    pe_shader_set_bool(pe_opengl.shader_program, "do_lighting", enable);
}

void pe_graphics_set_light_vector(pVec3 light_vector) {
    pe_shader_set_vec3(pe_opengl.shader_program, "light_vector", light_vector);
}

void pe_graphics_set_diffuse_color(peColor color) {
    pe_shader_set_vec3(pe_opengl.shader_program, "diffuse_color", pe_color_to_vec4(color).rgb);
}

void pe_graphics_matrix_update(void) {
    char *uniform_names[peMatrixMode_Count] = {
        "matrix_projection",
        "matrix_view",
        "matrix_model"
    };
    for (int matrix_mode = 0; matrix_mode < peMatrixMode_Count; matrix_mode += 1) {
        if (pe_graphics.matrix_dirty[pe_graphics.mode][matrix_mode]) {
            pMat4 *matrix = &pe_graphics.matrix[pe_graphics.mode][matrix_mode];
            pe_shader_set_mat4(pe_opengl.shader_program, uniform_names[matrix_mode], matrix);
            pe_graphics.matrix_dirty[pe_graphics.mode][matrix_mode] = false;
        }
    }
}

pMat4 pe_matrix_perspective(float fovy, float aspect_ratio, float near_z, float far_z) {
	return p_perspective_rh_no(fovy * P_DEG2RAD, aspect_ratio, near_z, far_z);
}

pMat4 pe_matrix_orthographic(float left, float right, float bottom, float top, float near_z, float far_z) {
    return p_orthographic_rh_no(left, right, bottom, top, near_z, far_z);
}

void pe_graphics_dynamic_draw_draw_batches(void) {
    if (dynamic_draw.vertex_used > 0) {
        peMatrixMode old_matrix_mode = pe_graphics.matrix_mode;
        pMat4 old_matrix_model = pe_graphics.matrix[pe_graphics.mode][peMatrixMode_Model];
        bool old_matrix_model_is_identity = pe_graphics.matrix_model_is_identity[pe_graphics.mode];

        pe_graphics_matrix_mode(peMatrixMode_Model);
        pe_graphics_matrix_identity();
        pe_graphics_matrix_update();
        pe_shader_set_bool(pe_opengl.shader_program, "has_skeleton", false);

        glBindVertexArray(dynamic_draw_opengl.vertex_array_object);
        glBindBuffer(GL_ARRAY_BUFFER, dynamic_draw_opengl.vertex_buffer_object);
        glBufferSubData(GL_ARRAY_BUFFER, 0, (size_t)dynamic_draw.vertex_used*sizeof(peDynamicDrawVertex), dynamic_draw.vertex);

        bool previous_do_lighting = false;
        for (int b = dynamic_draw.batch_drawn_count; b <= dynamic_draw.batch_current; b += 1) {
            PE_ASSERT(dynamic_draw.batch[b].vertex_count > 0);

            peTexture *texture = dynamic_draw.batch[b].texture;
            if (!texture) texture = &default_texture;
            pe_texture_bind(*texture);

            bool do_lighting = dynamic_draw.batch[b].do_lighting;
            if (b == 0 || previous_do_lighting != do_lighting) {
                pe_graphics_set_lighting(do_lighting);
                pe_graphics_set_light_vector(PE_LIGHT_VECTOR_DEFAULT);
                previous_do_lighting = do_lighting;
            }

            GLenum primitive_gl;
            switch (dynamic_draw.batch[b].primitive) {
                case pePrimitive_Points: primitive_gl = GL_POINTS; break;
                case pePrimitive_Lines: primitive_gl = GL_LINES; break;
                case pePrimitive_Triangles: primitive_gl = GL_TRIANGLES; break;
                default: PE_PANIC(); break;
            }

            glDrawArrays(
                primitive_gl,
                dynamic_draw.batch[b].vertex_offset,
                dynamic_draw.batch[b].vertex_count
            );
        }

        glBindVertexArray(0);

        pe_graphics_matrix_set(&old_matrix_model);
        pe_graphics_matrix_update();
        pe_graphics_matrix_mode(old_matrix_mode);
        pe_graphics.matrix_model_is_identity[pe_graphics.mode] = old_matrix_model_is_identity;

        dynamic_draw.batch_drawn_count = dynamic_draw.batch_current + 1;
    }
}

void pe_graphics_dynamic_draw_push_vec3(pVec3 position) {
    if (!pe_graphics.matrix_model_is_identity[pe_graphics.mode]) {
        pMat4 *matrix = &pe_graphics.matrix[pe_graphics.mode][peMatrixMode_Model];
        pVec4 position_vec4 = p_vec4v(position, 1.0f);
        pVec4 transformed_position = p_mat4_mul_vec4(*matrix, position_vec4);
        position = transformed_position.xyz;
    }
    pePrimitive batch_primitive = dynamic_draw.batch[dynamic_draw.batch_current].primitive;
    int batch_vertex_count = dynamic_draw.batch[dynamic_draw.batch_current].vertex_count;
    int primitive_vertex_count = pe_graphics_primitive_vertex_count(batch_primitive);
    int primitive_vertex_left = primitive_vertex_count - (batch_vertex_count % primitive_vertex_count);
    pe_graphics_dynamic_draw_vertex_reserve(primitive_vertex_left);
    peDynamicDrawVertex *vertex = &dynamic_draw.vertex[dynamic_draw.vertex_used];
    vertex->position = position;
    vertex->normal = dynamic_draw.normal;
    vertex->texcoord = dynamic_draw.texcoord;
    vertex->color = dynamic_draw.color.rgba;
    dynamic_draw.batch[dynamic_draw.batch_current].vertex_count += 1;
    dynamic_draw.vertex_used += 1;
}

//
// INTERNAL IMPLEMENTATIONS
//

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

void gl_debug_message_proc(
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

GLuint pe_shader_create_from_file(peArena *temp_arena, const char *source_file_path) {
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

    PE_ASSERT(pe_graphics.mode == peGraphicsMode_2D);
    pe_graphics.matrix_mode = peMatrixMode_Projection;
    pMat4 matrix_orthographic = pe_matrix_orthographic(
        0, (float)width,
        (float)height, 0,
        0.0f, 1000.0f
    );
    pe_graphics_matrix_set(&matrix_orthographic);
    pe_graphics_matrix_update();
}

void pe_shader_set_vec3(GLuint shader_program, const GLchar *name, pVec3 value) {
    GLint uniform_location = glGetUniformLocation(shader_program, name);
    glUniform3fv(uniform_location, 1, value.elements);
}

void pe_shader_get_mat4(GLuint shader_program, const GLchar *name, pMat4 *value) {
    GLint uniform_location = glGetUniformLocation(shader_program, name);
    glGetUniformfv(shader_program, uniform_location, (void*)value);
}

void pe_shader_set_mat4(GLuint shader_program, const GLchar *name, pMat4 *value) {
    GLint uniform_location = glGetUniformLocation(shader_program, name);
    glUniformMatrix4fv(uniform_location, 1, GL_FALSE, (void*)value);
}

void pe_shader_set_mat4_array(GLuint shader_program, const GLchar *name, pMat4 *values, GLsizei count) {
    GLint uniform_location = glGetUniformLocation(shader_program, name);
    glUniformMatrix4fv(uniform_location, count, GL_FALSE, (void*)values);
}

void pe_shader_get_bool(GLuint shader_program, const GLchar *name, bool *value) {
    GLint uniform_location = glGetUniformLocation(shader_program, name);
    glGetUniformiv(shader_program, uniform_location, (GLint*)value);
}

void pe_shader_set_bool(GLuint shader_program, const GLchar *name, bool value) {
    GLint uniform_location = glGetUniformLocation(shader_program, name);
    GLint gl_value = value ? 1 : 0;
    glUniform1i(uniform_location, gl_value);
}

peTexture pe_texture_create(peArena *temp_arena, void *data, int width, int height) {
    GLint texture_object;
    glGenTextures(1, &texture_object);
    glBindTexture(GL_TEXTURE_2D, texture_object);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float border_color[] = { 1.0f, 0.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border_color);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    const int channels = 4;
    PE_ASSERT(channels == 3 || channels == 4);
    GLint gl_format = (
        channels == 3 ? GL_RGB  :
        channels == 4 ? GL_RGBA : GL_INVALID_ENUM
    );
    glTexImage2D(GL_TEXTURE_2D, 0, gl_format, width, height, 0, gl_format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    peTexture texture = {
        .texture_object = texture_object,
        .width = width,
        .height = height
    };
    return texture;
}

void pe_texture_bind(peTexture texture) {
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture.texture_object);
}

void pe_texture_bind_default(void) {
	pe_texture_bind(default_texture);
}
