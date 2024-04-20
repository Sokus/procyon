#include "pe_graphics.h"
#include "pe_graphics_linux.h"

#include "core/pe_core.h"
#include "core/pe_file_io.h"
#include "platform/pe_window.h"

#include "glad/glad.h"

#include "HandmadeMath.h"

#include <stdio.h>
#include <stddef.h>
#include <string.h>

peOpenGL pe_opengl = {0};

extern peTexture default_texture;

typedef enum peMatrixMode {
    peMatrixMode_Projection,
    peMatrixMode_View,
    peMatrixMode_Model,
    peMatrixMode_Count
} peMatrixMode;

#define MAX_MATRIX_STACK_DEPTH 32

struct peGraphics {
    peMatrixMode matrix_mode;
    HMM_Mat4 matrix_stack[peMatrixMode_Count][MAX_MATRIX_STACK_DEPTH];
    int matrix_stack_depth[peMatrixMode_Count];
    bool matrix_update[peMatrixMode_Count];
} graphics = {0};

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
    
    PE_ASSERT(graphics.matrix_stack_depth[peMatrixMode_Projection] == 0);
    graphics.matrix_mode = peMatrixMode_Projection;
    graphics.matrix_stack[peMatrixMode_Projection][0] = pe_matrix_orthographic(
        0, (float)width,
        (float)height, 0,
        0.0f, 1000.0f
    );
    graphics.matrix_update[peMatrixMode_Projection] = true;
}

void pe_graphics_init_linux(peArena *temp_arena, int window_width, int window_height) {
    pe_opengl.framebuffer_width = window_width;
    pe_opengl.framebuffer_height = window_height;

    gladLoadGLLoader((GLADloadproc)&pe_window_get_proc_address);

    // TODO: Confirm that glDebugMessageCallbackARB actually works
#if !defined(NDEBUG) && defined(GL_ARB_debug_output)
    if (glDebugMessageCallbackARB) {
        glDebugMessageCallbackARB((GLDEBUGPROCARB)gl_debug_message_proc, NULL);
    }
#endif

    glViewport(0, 0, window_width, window_height);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);    

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
        pe_shader_set_bool(pe_opengl.shader_program, "do_lighting", true);
        // I don't expect the light vector to be relevant
        // anytime soon so lets just set it here
        pe_shader_set_vec3(pe_opengl.shader_program, "light_vector", HMM_V3(0.5f, -1.0f, 0.5f));
    }

    {
        graphics.matrix_mode = peMatrixMode_Projection;
        graphics.matrix_stack[peMatrixMode_Projection][0] = pe_matrix_orthographic(
            0, (float)pe_screen_width(),
            (float)pe_screen_height(), 0,
            0.0f, 1000.0f
        );
        graphics.matrix_stack[peMatrixMode_View][0] = HMM_M4D(1.0f);
        graphics.matrix_stack[peMatrixMode_Model][0] = HMM_M4D(1.0f);
        graphics.matrix_update[peMatrixMode_Projection] = true;
        graphics.matrix_update[peMatrixMode_View] = true;
        graphics.matrix_update[peMatrixMode_Model] = true;
    }    

    pe_window_set_framebuffer_size_callback(&pe_framebuffer_size_callback_linux);

    pe_dynamic_draw_init();
}

void pe_graphics_push_matrix(void) {
    int mode = graphics.matrix_mode;
    int stack_depth = graphics.matrix_stack_depth[mode];
    PE_ASSERT(stack_depth + 1 < MAX_MATRIX_STACK_DEPTH);
    memcpy(
        &graphics.matrix_stack[mode][stack_depth+1],
        &graphics.matrix_stack[mode][stack_depth],
        sizeof(HMM_Mat4)
    );
    graphics.matrix_stack_depth[mode] += 1;
}

void pe_graphics_pop_matrix(void) {
    int mode = graphics.matrix_mode;
    PE_ASSERT(graphics.matrix_stack_depth[mode] > 0);
    graphics.matrix_stack_depth[mode] -= 1;
    graphics.matrix_update[mode] = true;
}

void pe_graphics_update_matrix(void) {
    char *uniform_names[peMatrixMode_Count] = {
        "matrix_projection",
        "matrix_view",
        "matrix_model"
    };
    for (int m = 0; m < peMatrixMode_Count; m += 1) {
        if (graphics.matrix_update[m]) {
            int stack_depth = graphics.matrix_stack_depth[m];
            HMM_Mat4 *matrix = &graphics.matrix_stack[m][stack_depth];
            pe_shader_set_mat4(pe_opengl.shader_program, uniform_names[m], matrix);
            graphics.matrix_update[m] = false;
        }
    }
}

void pe_graphics_matrix_mode(peMatrixMode mode) {
    PE_ASSERT(mode >= 0);
    PE_ASSERT(mode < peMatrixMode_Count);
    graphics.matrix_mode = mode;
}

void pe_graphics_matrix_set(HMM_Mat4 *matrix) {
    int mode = graphics.matrix_mode;
    int stack_depth = graphics.matrix_stack_depth[mode];
    memcpy(&graphics.matrix_stack[mode][stack_depth], matrix, sizeof(HMM_Mat4));
    graphics.matrix_update[mode] = true;
}

void pe_graphics_matrix_identity(void) {
    HMM_Mat4 matrix = HMM_M4D(1.0f);
    pe_graphics_matrix_set(&matrix);
}

void pe_graphics_mode_3d_begin(peCamera camera) {
    pe_graphics_dynamic_draw_flush();
    
    pe_graphics_matrix_mode(peMatrixMode_Projection);
    pe_graphics_push_matrix();
    float aspect_ratio = (float)pe_screen_width()/(float)pe_screen_height();
    HMM_Mat4 matrix_perspective = pe_matrix_perspective(camera.fovy, aspect_ratio, 1.0f, 1000.0f);
    pe_graphics_matrix_set(&matrix_perspective);
    
    pe_graphics_matrix_mode(peMatrixMode_View);
    HMM_Mat4 matrix_view = HMM_LookAt_RH(camera.position, camera.target, camera.up);
    pe_graphics_matrix_set(&matrix_view);
    
    pe_graphics_matrix_mode(peMatrixMode_Model);
    HMM_Mat4 matrix_identity = HMM_M4D(1.0f);
    pe_graphics_matrix_set(&matrix_identity);
    
    glEnable(GL_DEPTH_TEST);
}

void pe_graphics_mode_3d_end(void) {
    pe_graphics_dynamic_draw_flush();
    
    pe_graphics_matrix_mode(peMatrixMode_Projection);
    pe_graphics_pop_matrix();
    
    HMM_Mat4 matrix_identity = HMM_M4D(1.0f);
    pe_graphics_matrix_mode(peMatrixMode_View);
    pe_graphics_matrix_set(&matrix_identity);
    pe_graphics_matrix_mode(peMatrixMode_Model);
    pe_graphics_matrix_set(&matrix_identity);
    
    glDisable(GL_DEPTH_TEST);
}

void pe_shader_set_vec3(GLuint shader_program, const GLchar *name, HMM_Vec3 value) {
    GLint uniform_location = glGetUniformLocation(shader_program, name);
    glUniform3fv(uniform_location, 1, value.Elements);
}

void pe_shader_get_mat4(GLuint shader_program, const GLchar *name, HMM_Mat4 *value) {
    GLint uniform_location = glGetUniformLocation(shader_program, name);
    glGetUniformfv(shader_program, uniform_location, (void*)value);
}

void pe_shader_set_mat4(GLuint shader_program, const GLchar *name, HMM_Mat4 *value) {
    GLint uniform_location = glGetUniformLocation(shader_program, name);
    glUniformMatrix4fv(uniform_location, 1, GL_FALSE, (void*)value);
}

void pe_shader_set_mat4_array(GLuint shader_program, const GLchar *name, HMM_Mat4 *values, GLsizei count) {
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

    peTexture texture = { 
        .texture_object = texture_object,
        .width = width,
        .height = height
    };
    return texture;
}

typedef struct peDynamicDrawVertex {
    HMM_Vec3 position;
    HMM_Vec3 normal;
    HMM_Vec2 texcoord;
    uint32_t color;
} peDynamicDrawVertex;

typedef struct peDynamicDrawBatch {
    int vertex_offset;
    int vertex_count;
    GLenum primitive;
    peTexture *texture;
} peDynamicDrawBatch;

#define PE_MAX_DYNAMIC_DRAW_VERTEX_COUNT 1024
#define PE_MAX_DYNAMIC_DRAW_BATCH_COUNT 512

struct peDynamicDrawState {
    GLenum primitive;
    peTexture *texture;
    HMM_Vec3 normal;
    HMM_Vec2 texcoord;
    peColor color;

    peDynamicDrawVertex vertex[PE_MAX_DYNAMIC_DRAW_VERTEX_COUNT];
    int vertex_used;
    
    peDynamicDrawBatch batch[PE_MAX_DYNAMIC_DRAW_BATCH_COUNT];
    int batch_current;
    
    GLuint vertex_array_object;
    GLuint vertex_buffer_object;
} dynamic_draw = {0};

static void pe_graphics_dynamic_draw_set_texture(peTexture *texture);

void pe_dynamic_draw_init(void) {
    glGenVertexArrays(1, &dynamic_draw.vertex_array_object);
    glBindVertexArray(dynamic_draw.vertex_array_object);
    
    glGenBuffers(1, &dynamic_draw.vertex_buffer_object);
    glBindBuffer(GL_ARRAY_BUFFER, dynamic_draw.vertex_buffer_object);
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
    
    pe_graphics_dynamic_draw_set_texture(&default_texture);
}

void pe_graphics_dynamic_draw_flush(void) {
    pe_graphics_dynamic_draw_end_batch();
    if (dynamic_draw.vertex_used > 0) {
        bool old_has_skeleton;
        bool old_do_lighting;
        
        pe_shader_set_bool(pe_opengl.shader_program, "has_skeleton", false);
        pe_shader_set_bool(pe_opengl.shader_program, "do_lighting", false);
        
        glBindVertexArray(dynamic_draw.vertex_array_object);
        glBindBuffer(GL_ARRAY_BUFFER, dynamic_draw.vertex_buffer_object);
        glBufferSubData(GL_ARRAY_BUFFER, 0, (size_t)dynamic_draw.vertex_used*sizeof(peDynamicDrawVertex), dynamic_draw.vertex);
        
        pe_graphics_update_matrix();
        
        for (int b = 0; b < dynamic_draw.batch_current; b += 1) {
            pe_texture_bind(*dynamic_draw.batch[b].texture);
        
            glDrawArrays(
                dynamic_draw.batch[b].primitive,
                dynamic_draw.batch[b].vertex_offset,
                dynamic_draw.batch[b].vertex_count
            );
        }
                
        glBindVertexArray(0);
        
        pe_shader_set_bool(pe_opengl.shader_program, "has_skeleton", old_has_skeleton);
        pe_shader_set_bool(pe_opengl.shader_program, "do_lighting", old_do_lighting);
    }
    
    dynamic_draw.vertex_used = 0;
    dynamic_draw.batch_current = 0;
    dynamic_draw.batch[0].vertex_count = 0;
}

void pe_graphics_dynamic_draw_end_batch(void) {
    PE_ASSERT(dynamic_draw.batch_current < PE_MAX_DYNAMIC_DRAW_BATCH_COUNT);
    if (dynamic_draw.batch[dynamic_draw.batch_current].vertex_count > 0) {
        dynamic_draw.batch_current += 1;
    }
}

static void pe_graphics_dynamic_draw_new_batch(void) {
    pe_graphics_dynamic_draw_end_batch();
    if (dynamic_draw.batch_current == PE_MAX_DYNAMIC_DRAW_BATCH_COUNT) {
        pe_graphics_dynamic_draw_flush();
    }
    dynamic_draw.batch[dynamic_draw.batch_current].primitive = dynamic_draw.primitive;
    dynamic_draw.batch[dynamic_draw.batch_current].texture = dynamic_draw.texture;
    dynamic_draw.batch[dynamic_draw.batch_current].vertex_offset = dynamic_draw.vertex_used;
    dynamic_draw.batch[dynamic_draw.batch_current].vertex_count = 0;
}

static void pe_graphics_dynamic_draw_set_primitive(GLenum primitive) {
    dynamic_draw.primitive = primitive;
    if (dynamic_draw.batch[dynamic_draw.batch_current].primitive != primitive) {
        pe_graphics_dynamic_draw_new_batch();
    }
}

static void pe_graphics_dynamic_draw_set_texture(peTexture *texture) {
    dynamic_draw.texture = texture;
    if (dynamic_draw.batch[dynamic_draw.batch_current].texture != texture) {
        pe_graphics_dynamic_draw_new_batch();
    }
}

static int pe_graphics_primitive_vertex_count(GLenum primitive) {
    int result;
    switch (primitive) {
        case GL_POINTS: result = 1; break;
        case GL_LINES: result = 2; break;
        case GL_TRIANGLES: result = 3; break;
        default:
            PE_PANIC_MSG("unsupported primitive: %d\n", primitive);
            break;
    }
    return result;
}

static bool pe_graphics_dynamic_draw_vertex_reserve(int count) {
    bool flushed = false;
    if (dynamic_draw.vertex_used + count > PE_MAX_DYNAMIC_DRAW_VERTEX_COUNT) {
        pe_graphics_dynamic_draw_flush();
        flushed = true;
    }
    return flushed;
}

void pe_graphics_dynamic_draw_set_normal(HMM_Vec3 normal) {
    dynamic_draw.normal = normal;
}

void pe_graphics_dynamic_draw_set_texcoord(HMM_Vec2 texcoord) {
    dynamic_draw.texcoord = texcoord;
}

void pe_graphics_dynamic_draw_set_color(peColor color) {
    dynamic_draw.color = color;
}

void pe_graphics_dynamic_draw_push_vec3(HMM_Vec3 position) {
    // TODO: apply model matrix
    GLenum batch_primitive = dynamic_draw.batch[dynamic_draw.batch_current].primitive;
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

void pe_graphics_dynamic_draw_push_vec2(HMM_Vec2 position) {
    HMM_Vec3 vec3 = HMM_V3(position.X, position.Y, 0.0f);
    pe_graphics_dynamic_draw_push_vec3(vec3);
}

void pe_graphics_draw_point(HMM_Vec2 position, peColor color) {
    pe_graphics_dynamic_draw_set_primitive(GL_POINTS);
    pe_graphics_dynamic_draw_set_color(color);
    pe_graphics_dynamic_draw_push_vec2(position);
}

void pe_graphics_draw_point_int(int pos_x, int pos_y, peColor color) {
    HMM_Vec2 position = HMM_V2((float)pos_x, (float)pos_y);
    pe_graphics_draw_point(position, color);    
}

void pe_graphics_draw_line(HMM_Vec2 start_position, HMM_Vec2 end_position, peColor color) {
    pe_graphics_dynamic_draw_set_primitive(GL_LINES);
    pe_graphics_dynamic_draw_set_color(color);
    pe_graphics_dynamic_draw_push_vec2(start_position);
    pe_graphics_dynamic_draw_push_vec2(end_position);
}

void pe_graphics_draw_rectangle(float x, float y, float width, float height, peColor color) {
    pe_graphics_dynamic_draw_set_primitive(GL_TRIANGLES);

    HMM_Vec2 top_left = HMM_V2(x, y);
    HMM_Vec2 top_right = HMM_V2(x + width, y);
    HMM_Vec2 bottom_left = HMM_V2(x, y + height);
    HMM_Vec2 bottom_right = HMM_V2(x + width, y + height);

    pe_graphics_dynamic_draw_vertex_reserve(6);
    pe_graphics_dynamic_draw_set_color(color);
    pe_graphics_dynamic_draw_push_vec2(top_left);
    pe_graphics_dynamic_draw_push_vec2(bottom_left);
    pe_graphics_dynamic_draw_push_vec2(bottom_right);
    pe_graphics_dynamic_draw_push_vec2(top_left);
    pe_graphics_dynamic_draw_push_vec2(bottom_right);
    pe_graphics_dynamic_draw_push_vec2(top_right);
}

void pe_graphics_draw_texture(peTexture *texture, float x, float y, peColor tint) {
    PE_ASSERT(texture != NULL);
    pe_graphics_dynamic_draw_set_primitive(GL_TRIANGLES);
    pe_graphics_dynamic_draw_set_texture(texture);

    HMM_Vec2 top_left = HMM_V2(x, y);
    HMM_Vec2 top_right = HMM_V2(x + texture->width, y);
    HMM_Vec2 bottom_left = HMM_V2(x, y + texture->height);
    HMM_Vec2 bottom_right = HMM_V2(x + texture->width, y + texture->height);

    pe_graphics_dynamic_draw_vertex_reserve(6);
    pe_graphics_dynamic_draw_set_color(tint);
    pe_graphics_dynamic_draw_set_texcoord(HMM_V2(0.0f, 1.0f));
    pe_graphics_dynamic_draw_push_vec2(top_left);
    pe_graphics_dynamic_draw_set_texcoord(HMM_V2(0.0f, 0.0f));
    pe_graphics_dynamic_draw_push_vec2(bottom_left);
    pe_graphics_dynamic_draw_set_texcoord(HMM_V2(1.0f, 0.0f));
    pe_graphics_dynamic_draw_push_vec2(bottom_right);
    pe_graphics_dynamic_draw_set_texcoord(HMM_V2(0.0f, 1.0f));
    pe_graphics_dynamic_draw_push_vec2(top_left);
    pe_graphics_dynamic_draw_set_texcoord(HMM_V2(1.0f, 0.0f));
    pe_graphics_dynamic_draw_push_vec2(bottom_right);
    pe_graphics_dynamic_draw_set_texcoord(HMM_V2(1.0f, 1.0f));
    pe_graphics_dynamic_draw_push_vec2(top_right);
}

void pe_graphics_draw_point_3D(HMM_Vec3 position, peColor color) {
    pe_graphics_dynamic_draw_set_primitive(GL_POINTS);
    pe_graphics_dynamic_draw_set_color(color);
    pe_graphics_dynamic_draw_push_vec3(position);
}

void pe_graphics_draw_line_3D(HMM_Vec3 start_position, HMM_Vec3 end_position, peColor color) {
    pe_graphics_dynamic_draw_set_primitive(GL_LINES);
    pe_graphics_dynamic_draw_set_color(color);
    pe_graphics_dynamic_draw_push_vec3(start_position);
    pe_graphics_dynamic_draw_push_vec3(end_position);
}
