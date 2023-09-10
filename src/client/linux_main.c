#include "pe_core.h"

#include "pe_graphics.h"
#include "pe_platform.h"
#include "pe_file_io.h"

#include "glad/glad.h"

#include "HandmadeMath.h"

#include <stdio.h>

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

static GLuint pe_create_shader_from_file(GLenum type, const char *source_file_path) {
    peFileContents shader_source_file_contents = pe_file_read_contents(pe_heap_allocator(), source_file_path, true);
    GLuint shader = pe_shader_compile(type, shader_source_file_contents.data);
    pe_file_free_contents(shader_source_file_contents);
    return shader;
}

static void pe_shader_set_vec3(GLuint shader_program, const GLchar *name, HMM_Vec3 value) {
    GLint uniform_location = glGetUniformLocation(shader_program, name);
    glUniform3fv(uniform_location, 1, value.Elements);
}

static void pe_shader_set_mat4(GLuint shader_program, const GLchar *name, HMM_Mat4 *value) {
    GLint uniform_location = glGetUniformLocation(shader_program, name);
    glUniformMatrix4fv(uniform_location, 1, false, value->Elements[0]);
}

typedef struct peCamera {
    HMM_Vec3 position;
    HMM_Vec3 target;
    HMM_Vec3 up;
    float fovy;
} peCamera;



int main(int, char*[]) {
    pe_platform_init();
    pe_graphics_init(960, 540, "Procyon");

    glEnable(GL_DEPTH_TEST);

    //glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    GLuint vertex_shader = pe_create_shader_from_file(GL_VERTEX_SHADER, "res/shader.glsl");
    GLuint fragment_shader = pe_create_shader_from_file(GL_FRAGMENT_SHADER, "res/shader.glsl");

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



    GLuint vertex_array_object;
    glGenVertexArrays(1, &vertex_array_object);
    glBindVertexArray(vertex_array_object);

    float vertices[] = {
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,

        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,

        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,

         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,

        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f
    };

    GLuint vertex_buffer_object;
    glGenBuffers(1, &vertex_buffer_object);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_object);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

#if 0
    unsigned int indices[] = {  // note that we start from 0!
        0, 1, 3,  // first Triangle
        1, 2, 3   // second Triangle
    };

    GLuint element_buffer_object;
    glGenBuffers(1, &element_buffer_object);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, element_buffer_object);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
#endif

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);




    glBindVertexArray(0);


    HMM_Vec3 camera_offset = { -1.2f, 1.0f, 2.0f };
    peCamera camera = {
        .target = {0.0f, 0.0f, 0.0f},
        .up = {0.0f, 1.0f, 0.0f},
        .fovy = 55.0f
    };
    camera.position = HMM_AddV3(camera.target, camera_offset);


    float time = 0.0f;

    while (!pe_platform_should_quit()) {
        pe_platform_poll_events();

        pe_graphics_frame_begin();
        pe_clear_background(PE_COLOR_BLACK);

        glUseProgram(shader_program);
        pe_shader_set_vec3(shader_program, "light_vector", HMM_V3(0.5f, -1.0f, 0.5f));

        float aspect = (float)pe_screen_width() / (float)pe_screen_height();

        HMM_Mat4 matrix_perspective;
        {
            HMM_Mat4 perspective = HMM_Perspective_RH_NO(camera.fovy, aspect, 0.5f, 1000.0f);
            HMM_Mat4 rotation = HMM_Rotate_RH(180.0f * HMM_DegToRad, HMM_V3(0.0f, 0.0f, 1.0f));
            matrix_perspective = HMM_MulM4(rotation, perspective);
        }
        pe_shader_set_mat4(shader_program, "matrix_projection", &matrix_perspective);

        camera.position.Y = 1.2f * HMM_SinF(time * HMM_PI * 0.2f);
        time += 1/60.0f;

        HMM_Mat4 matrix_view = HMM_LookAt_RH(camera.position, camera.target, camera.up);
        pe_shader_set_mat4(shader_program, "matrix_view", &matrix_view);

        HMM_Mat4 matrix_model = HMM_M4D(1.0f);
        pe_shader_set_mat4(shader_program, "matrix_model", &matrix_model);



        glBindVertexArray(vertex_array_object);
        //glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glDrawArrays(GL_TRIANGLES, 0, 36);



        pe_graphics_frame_end(true);
    }

    pe_graphics_shutdown();


    return 0;
}