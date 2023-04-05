#include "procyon.h"
#include "procyon_gfx.h"

#define PROCYON_PSP

#ifdef PROCYON_DESKTOP
    #include "glad/glad.h"
#endif

#ifdef PROCYON_PSP
    #include <pspdisplay.h>
    #include <pspgu.h>
    #include <pspgum.h>
    #include <pspkernel.h>
    #include <malloc.h>

    #define PSP_SCR_W 480
    #define PSP_SCR_H 272
    #define PSP_BUF_W 512
#endif

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef PROCYON_DESKTOP

typedef struct pgfx_gl_vertex
{
    papp_vec2 pos;
    papp_color color;
    papp_vec2 texcoord;
} pgfx_gl_vertex;

typedef struct pgfx_gl_draw_call
{
    pgfx_draw_flags flags;
    int vertex_data_offset;
    int vertex_count;
    int index_start;
    int index_count;

    GLuint texture_id;
} pgfx_gl_draw_call;

typedef struct pgfx_gl_state
{
    GLuint shader;
    GLuint vertex_buffer, vertex_array, element_buffer;

    unsigned char vertex_data[1024];
    int vertex_data_used;
    unsigned short indices[1024];
    int index_count;
    pgfx_gl_draw_call draws[32];
    int draw_count;

    int vertex_mark;
} pgfx_gl_state;

#endif // PROCYON_DESKTOP

#ifdef PROCYON_PSP

typedef struct pgfx_psp_vertex
{
    papp_vec2 texcoord;
	unsigned int color;
	papp_vec3 pos;
} pgfx_psp_vertex;

typedef struct pgfx_psp_state
{
    unsigned int __attribute__((aligned(16))) list[262144];

    unsigned int edram_used;

    void *current_drawbuffer;

    uint8_t __attribute__((aligned(16))) vertex_data[1024];
    int vertex_data_used;
    int vertex_data_start;
    int vertex_count;
    uint16_t __attribute__((aligned(16))) index_data[1024];
    int index_data_used;
    int index_data_start;

    int flags;
} pgfx_psp_state;

#endif // PROCYON_PSP

typedef struct pgfx_state
{
    papp_color color;
    papp_vec2 texcoord;

    #if defined(PROCYON_DESKTOP)
        pgfx_gl_state gl;
    #endif
    #if defined(PROCYON_PSP)
        pgfx_psp_state psp;
    #endif
} pgfx_state;
pgfx_state pgfx = {0};

#ifdef PROCYON_DESKTOP

static void pgfx_gl_uniform_mat4(GLuint program, const char *name, const float* mat4)
{
    GLint uniform_location = glGetUniformLocation(program, name);
    glUniformMatrix4fv(uniform_location, 1, GL_FALSE, mat4);
}

static GLuint pgfx_gl_compile_shader(GLenum type, const char *source)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, 0);
    glCompileShader(shader);

    int compile_status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compile_status);

    if(compile_status != GL_TRUE)
    {
        int info_log_length;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &info_log_length);
        char *info_log = (char *)malloc(info_log_length);
        glGetShaderInfoLog(shader, info_log_length, &info_log_length, info_log);
        const char *shader_type = (type == GL_VERTEX_SHADER ? "Vertex Shader" :
                                   type == GL_FRAGMENT_SHADER ? "Fragment Shader" : "???");
        fprintf(stderr, "GL: %s: %s\n", shader_type, info_log);
        free(info_log);
    }

    return shader;
}

static GLuint pgfx_gl_create_program(const char *vertex_shader_source, const char *fragment_shader_source)
{
    GLuint vertex_shader_handle = pgfx_gl_compile_shader(GL_VERTEX_SHADER, vertex_shader_source);
    GLuint fragment_shader_handle = pgfx_gl_compile_shader(GL_FRAGMENT_SHADER, fragment_shader_source);

    GLuint program_handle = glCreateProgram();
    glAttachShader(program_handle, vertex_shader_handle);
    glAttachShader(program_handle, fragment_shader_handle);
    glLinkProgram(program_handle);

    int success;
    glGetProgramiv(program_handle, GL_LINK_STATUS, &success);
    if(!success)
    {
        char info_log[512];
        glGetShaderInfoLog(program_handle, 512, 0, info_log);
        fprintf(stderr, "GL: PROGRAM: %s\n", info_log);
    }

    glDetachShader(program_handle, vertex_shader_handle);
    glDeleteShader(vertex_shader_handle);
    glDetachShader(program_handle, fragment_shader_handle);
    glDeleteShader(fragment_shader_handle);

    return program_handle;
}

static void pgfx_gl_render_batch()
{
    if (pgfx.gl.vertex_data_used > 0)
    {
        glBindVertexArray(pgfx.gl.vertex_array);

        float *vertex_buffer = (float *)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
        memcpy(vertex_buffer, pgfx.gl.vertex_data, pgfx.gl.vertex_data_used);
        glUnmapBuffer(GL_ARRAY_BUFFER);

        float *index_buffer = (float *)glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);
        memcpy(index_buffer, pgfx.gl.indices, pgfx.gl.index_count * sizeof(unsigned short));
        glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);

        glUseProgram(pgfx.gl.shader);

        for (int draw_idx = 0; draw_idx < pgfx.gl.draw_count; ++draw_idx)
        {
            pgfx_gl_draw_call *draw_call = pgfx.gl.draws + draw_idx;
            GLenum mode;
            switch (draw_call->flags & PGFX_PRIM_BITS)
            {
                case PGFX_PRIM_TRIANGLES: mode = GL_TRIANGLES; break;
                case PGFX_PRIM_LINE: mode = GL_LINE; break;
                default: mode = GL_POINTS; break;
            }

            glBindTexture(GL_TEXTURE_2D, draw_call->texture_id);

            if (draw_call->flags & PGFX_MODE_INDEXED)
            {
                void *index_offset = (void *)(draw_call->index_start * sizeof(GLushort));
                glDrawElements(mode, draw_call->index_count, GL_UNSIGNED_SHORT, index_offset);
            }
            else
            {
                // WARNING: Assumes that we are only using one type of vertex
                //          or that all types are of the same size.
                int vertex_start = draw_call->vertex_data_offset / sizeof(pgfx_gl_vertex);
                glDrawArrays(mode, vertex_start, draw_call->vertex_count);
            }
        }

        glBindVertexArray(0);
    }

    pgfx.gl.draw_count = 1;
    pgfx.gl.vertex_data_used = 0;
    pgfx.gl.index_count = 0;
    pgfx.gl.draws[0].vertex_count = 0;
    pgfx.gl.draws[0].index_count = 0;
}

static bool pgfx_gl_init()
{
    // shader program
    {
        const char *vertex_shader =
            "#version 420 core                                    \n"
            "layout (location = 0) in vec2 aPos;                  \n"
            "layout (location = 1) in vec4 aColor;                \n"
            "layout (location = 2) in vec2 aTexCoord;             \n"
            "                                                     \n"
            "out vec4 ourColor;                                   \n"
            "out vec2 TexCoord;                                   \n"
            "                                                     \n"
            "uniform mat4 projection;                             \n"
            "                                                     \n"
            "void main()                                          \n"
            "{                                                    \n"
            "    gl_Position = projection * vec4(aPos, 0.0, 1.0); \n"
            "    ourColor = aColor;                               \n"
            "    TexCoord = vec2(aTexCoord.x, aTexCoord.y);       \n"
            "}";

        const char *fragment_shader =
            "#version 420 core                                    \n"
            "out vec4 FragColor;                                  \n"
            "                                                     \n"
            "in vec4 ourColor;                                    \n"
            "in vec2 TexCoord;                                    \n"
            "                                                     \n"
            "uniform sampler2D texture1;                          \n"
            "                                                     \n"
            "void main()                                          \n"
            "{                                                    \n"
            "   vec4 texel = texture(texture1, TexCoord);         \n"
            "	FragColor = texel * (ourColor / 255.0);           \n"
            "}";
        pgfx.gl.shader = pgfx_gl_create_program(vertex_shader, fragment_shader);
    }

    glGenVertexArrays(1, &pgfx.gl.vertex_array);
    glGenBuffers(1, &pgfx.gl.vertex_buffer);
    glGenBuffers(1, &pgfx.gl.element_buffer);

    glBindVertexArray(pgfx.gl.vertex_array);

    glBindBuffer(GL_ARRAY_BUFFER, pgfx.gl.vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(pgfx.gl.vertex_data), NULL, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pgfx.gl.element_buffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(pgfx.gl.indices), NULL, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(pgfx_gl_vertex), (void*)offsetof(pgfx_gl_vertex, pos));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(pgfx_gl_vertex), (void*)offsetof(pgfx_gl_vertex, color));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(pgfx_gl_vertex), (void*)offsetof(pgfx_gl_vertex, texcoord));
    glEnableVertexAttribArray(2);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    pgfx_enable(PGFX_CULL_FACE);
    glCullFace(GL_BACK);
    pgfx_front_face(PGFX_CCW);

    glBindVertexArray(0);

    pgfx.gl.draw_count = 1;
    pgfx.gl.vertex_data_used = 0;
    pgfx.gl.draws[0].vertex_count = 0;

    return true;
}

#endif // PROCYON_DESKTOP

#ifdef PROCYON_PSP

unsigned int pgfx_psp_get_buffer_size(unsigned int width, unsigned int height, unsigned int pixel_format)
{
	switch (pixel_format)
	{
		case GU_PSM_T4:
			return (width * height) >> 1;

		case GU_PSM_T8:
			return width * height;

		case GU_PSM_5650:
		case GU_PSM_5551:
		case GU_PSM_4444:
		case GU_PSM_T16:
			return 2 * width * height;

		case GU_PSM_8888:
		case GU_PSM_T32:
			return 4 * width * height;

		default:
			return 0;
	}
}

void *pgfx_psp_edram_push_size(unsigned int size)
{
    void *result = 0;
    if(pgfx.psp.edram_used + size <= sceGeEdramGetSize())
    {
        result = sceGeEdramGetAddr() + (int)pgfx.psp.edram_used;
        pgfx.psp.edram_used += size;
    }
    return result;
}

unsigned int pgfx_psp_edram_push_offset(unsigned int size)
{
    unsigned int result = 0;
    if(pgfx.psp.edram_used + size <= sceGeEdramGetSize())
    {
        result = pgfx.psp.edram_used;
        pgfx.psp.edram_used += size;
    }
    return result;
}

static void pgfx_psp_render_batch()
{
    sceGuFinish();
    sceGuSync(0, 0);

    pgfx.psp.vertex_data_used = 0;
    pgfx.psp.index_data_used = 0;
    sceKernelDcacheWritebackRange(pgfx.psp.vertex_data, sizeof(pgfx.psp.vertex_data));

    sceGuStart(GU_DIRECT, pgfx.psp.list);
}

static bool pgfx_psp_init()
{
	sceGuInit();
	sceGuStart(GU_DIRECT, pgfx.psp.list);

    unsigned int framebuffer_size = pgfx_psp_get_buffer_size(PSP_BUF_W, PSP_SCR_H, GU_PSM_8888);
    unsigned int depthbuffer_size = pgfx_psp_get_buffer_size(PSP_BUF_W, PSP_SCR_H, GU_PSM_4444);
    void *framebuffer0 = (void *)pgfx_psp_edram_push_offset(framebuffer_size);
    void *framebuffer1 = (void *)pgfx_psp_edram_push_offset(framebuffer_size);
    void *depthbuffer = (void *)pgfx_psp_edram_push_offset(depthbuffer_size);
    pgfx.psp.current_drawbuffer = framebuffer0;

	sceGuDrawBuffer(GU_PSM_8888, framebuffer0, PSP_BUF_W);
	sceGuDispBuffer(PSP_SCR_W, PSP_SCR_H, framebuffer1, PSP_BUF_W);
	sceGuDepthBuffer(depthbuffer, PSP_BUF_W);

    pgfx_enable(PGFX_CULL_FACE);
    pgfx_front_face(PGFX_CCW);

	sceGuEnable(GU_CLIP_PLANES);

	sceGuEnable(GU_SCISSOR_TEST);

	sceGuDepthRange(65535, 0);
	sceGuDepthFunc(GU_GEQUAL);
	sceGuDisable(GU_DEPTH_TEST);

	sceGuShadeModel(GU_SMOOTH);

    sceGuEnable(GU_BLEND);
	sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);

    sceGuEnable(GU_TEXTURE_2D);

	sceGuFinish();
	sceGuSync(0,0);

	sceDisplayWaitVblankStart();
	sceGuDisplay(GU_TRUE);

	sceGumMatrixMode(GU_VIEW);
    sceGumLoadIdentity();

	sceGumMatrixMode(GU_MODEL);
	sceGumLoadIdentity();

    return true;
}

static void pgfx_psp_copy_texture_data(void *dest, const void *src, const int pW, const int width, const int height)
{
    for (unsigned int y = 0; y < height; y++)
    {
        for (unsigned int x = 0; x < width; x++)
        {
            ((unsigned int*)dest)[x + y * pW] = ((unsigned int *)src)[x + y * width];
        }
    }
}

static void pgfx_psp_swizzle_fast(u8 *out, const u8 *in, const unsigned int width, const unsigned int height)
{
    unsigned int width_blocks = (width / 16);
    unsigned int height_blocks = (height / 8);

    unsigned int src_pitch = (width - 16) / 4;
    unsigned int src_row = width * 8;

    const u8 *ysrc = in;
    u32 *dst = (u32 *)out;

    for (unsigned int blocky = 0; blocky < height_blocks; ++blocky)
    {
        const u8 *xsrc = ysrc;
        for (unsigned int blockx = 0; blockx < width_blocks; ++blockx)
        {
            const u32 *src = (u32 *)xsrc;
            for (unsigned int j = 0; j < 8; ++j)
            {
                *(dst++) = *(src++);
                *(dst++) = *(src++);
                *(dst++) = *(src++);
                *(dst++) = *(src++);
                src += src_pitch;
            }
            xsrc += 16;
        }
        ysrc += src_row;
    }
}

#endif

bool pgfx_init()
{
    #if defined(PROCYON_DESKTOP)
        return pgfx_gl_init();
    #elif defined(PROCYON_PSP)
        return pgfx_psp_init();
    #endif
}

void pgfx_terminate()
{
    #if defined(PROCYON_DESKTOP)
        glDeleteVertexArrays(1, &pgfx.gl.vertex_array);
        glDeleteBuffers(1, &pgfx.gl.vertex_buffer);
        glDeleteBuffers(1, &pgfx.gl.element_buffer);
    #elif defined(PROCYON_PSP)
        sceGuTerm();
    #endif
}

void pgfx_start_frame()
{
    #if defined(PROCYON_DESKTOP)

    #elif defined(PROCYON_PSP)
        sceGuStart(GU_DIRECT, pgfx.psp.list);
    #endif
}

void pgfx_end_frame()
{
    pgfx_render_batch();

    #if defined(PROCYON_PSP)

        sceGuFinish();
        sceGuSync(0, 0);
        sceDisplayWaitVblankStart();
        pgfx.psp.current_drawbuffer = sceGuSwapBuffers();
    #endif
}

void pgfx_render_batch()
{
    #if defined(PROCYON_DESKTOP)
        pgfx_gl_render_batch();
    #elif defined(PROCYON_PSP)
        pgfx_psp_render_batch();
    #endif
}

void pgfx_set_clear_color(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
    #if defined(PROCYON_DESKTOP)
        glClearColor((float)r/255.0f, (float)g/255.0f, (float)b/255.0f, (float)a/255.0f);
    #elif defined(PROCYON_PSP)
        unsigned int color = r | g << 8 | b << 16 | a << 24;
        sceGuClearColor(color);
    #endif
}

void pgfx_clear()
{
    #if defined(PROCYON_DESKTOP)
        glClear(GL_COLOR_BUFFER_BIT);
    #elif defined(PROCYON_PSP)
        sceGuClear(GU_COLOR_BUFFER_BIT);
    #endif
}

void pgfx_update_viewport(int width, int height)
{
    #if defined(PROCYON_DESKTOP)
        glViewport(0, 0, width, height);
    #elif defined(PROCYON_PSP)
        sceGuScissor(0, 0, width, height); // probably out
        sceGuOffset(2048 - (width/2), 2048 - (height/2));
        sceGuViewport(2048, 2048, width, height);
    #endif
}

void pgfx_begin_drawing(int mode)
{
    #if defined(PROCYON_DESKTOP)
        pgfx_gl_draw_call *last_draw_call = &pgfx.gl.draws[pgfx.gl.draw_count - 1];
        if (last_draw_call->flags != mode)
        {
            GLuint current_texture_id = last_draw_call->texture_id;

            if (last_draw_call->vertex_count > 0)
                pgfx.gl.draw_count++;

            if (pgfx.gl.draw_count >= PROCYON_ARRAY_COUNT(pgfx.gl.draws))
                pgfx_gl_render_batch();

            last_draw_call = &pgfx.gl.draws[pgfx.gl.draw_count - 1];
            last_draw_call->flags = mode;
            last_draw_call->vertex_count = 0;
            last_draw_call->index_count = 0;
            last_draw_call->texture_id = current_texture_id;
        }
    #elif defined(PROCYON_PSP)
        pgfx.psp.flags = mode;
        pgfx.psp.vertex_data_start = pgfx.psp.vertex_data_used;
        pgfx.psp.index_data_start = pgfx.psp.index_data_used;
        pgfx.psp.vertex_count = 0;
    #endif
}

void pgfx_end_drawing()
{
    #if defined(PROCYON_DESKTOP)

    #elif defined(PROCYON_PSP)
        int prim;
        switch(pgfx.psp.flags & PGFX_PRIM_BITS)
        {
            case PGFX_PRIM_TRIANGLES: prim = GU_TRIANGLES; break;
            case PGFX_PRIM_LINE: prim = GU_LINES; break;
            case PGFX_PRIM_AABB: prim = GU_SPRITES; break;
            default: prim = GU_POINTS; break;
        }

        int vtype = GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D;
        int count = 0;
        void *indices = 0;
        void *vertices = pgfx.psp.vertex_data + pgfx.psp.vertex_data_start;

        switch(pgfx.psp.flags & PGFX_MODE_BITS)
        {
            case PGFX_MODE_INDEXED:
                vtype |= GU_INDEX_16BIT;
                count = pgfx.psp.index_data_used - pgfx.psp.index_data_start;
                indices = pgfx.psp.index_data + pgfx.psp.index_data_start;
                break;

            default:
                count = pgfx.psp.vertex_count;

        }

        sceGumDrawArray(prim, vtype, count, indices, vertices);
    #endif
}

void pgfx_reserve(int vertex_data_size, int index_count)
{
    #if defined(PROCYON_DESKTOP)
        pgfx_gl_draw_call *last_draw_call = &pgfx.gl.draws[pgfx.gl.draw_count - 1];

        if (pgfx.gl.vertex_data_used + vertex_data_size >= PROCYON_ARRAY_COUNT(pgfx.gl.vertex_data) ||
            pgfx.gl.index_count + index_count >= PROCYON_ARRAY_COUNT(pgfx.gl.indices))
        {
            pgfx_draw_flags current_flags = last_draw_call->flags;
            GLuint current_texture_id = last_draw_call->texture_id;

            pgfx_gl_render_batch();
            last_draw_call = &pgfx.gl.draws[pgfx.gl.draw_count - 1];
            last_draw_call->flags = current_flags;
            last_draw_call->texture_id = current_texture_id;
        }

        // WARNING: Assumes that we are only using one type of vertex
        //          or that all types are of the same size.
        pgfx.gl.vertex_mark = pgfx.gl.vertex_data_used / sizeof(pgfx_gl_vertex);

        if (vertex_data_size > 0 && last_draw_call->vertex_count == 0)
            last_draw_call->vertex_data_offset = pgfx.gl.vertex_data_used;

        if (index_count > 0 && last_draw_call->index_count == 0)
            last_draw_call->index_start = pgfx.gl.index_count;
    #elif defined(PROCYON_PSP)
        if(pgfx.psp.vertex_data_used + vertex_data_size > sizeof(pgfx.psp.vertex_data) ||
        pgfx.psp.index_data_used + index_count > PROCYON_ARRAY_COUNT(pgfx.psp.index_data))
        {
            pgfx_psp_render_batch();
        }
    #endif
}

void pgfx_use_texture(papp_texture *texture)
{
    #if defined(PROCYON_DESKTOP)
        pgfx_gl_draw_call *last_draw_call = &pgfx.gl.draws[pgfx.gl.draw_count - 1];
        if (texture->id != 0 && texture->id != last_draw_call->texture_id)
        {
            pgfx_draw_flags current_flags = last_draw_call->flags;

            if (last_draw_call->vertex_count > 0)
                pgfx.gl.draw_count++;

            if (pgfx.gl.draw_count >= PROCYON_ARRAY_COUNT(pgfx.gl.draws))
                pgfx_gl_render_batch();

            last_draw_call = &pgfx.gl.draws[pgfx.gl.draw_count - 1];
            last_draw_call->flags = current_flags;
            last_draw_call->vertex_count = 0;
            last_draw_call->index_count = 0;
            last_draw_call->texture_id = texture->id;
        }
    #elif defined(PROCYON_PSP)
        sceGuTexMode(GU_PSM_8888, 0, 0, texture->swizzled);
        sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
        sceGuTexFilter(GU_NEAREST, GU_NEAREST);
        sceGuTexWrap(GU_REPEAT, GU_REPEAT);
        sceGuTexImage(0, texture->padded_width, texture->padded_height,
                    texture->padded_width, texture->data);
    #endif
}

void pgfx_batch_vec2(float x, float y)
{
    #if defined(PROCYON_DESKTOP)
        pgfx_gl_vertex *vertex = (pgfx_gl_vertex *)(pgfx.gl.vertex_data + pgfx.gl.vertex_data_used);
        vertex->pos.x = x;
        vertex->pos.y = y;
        vertex->color = pgfx.color;
        vertex->texcoord = pgfx.texcoord;
        pgfx.gl.vertex_data_used += sizeof(pgfx_gl_vertex);

        pgfx.gl.draws[pgfx.gl.draw_count - 1].vertex_count++;
    #elif defined(PROCYON_PSP)
        pgfx_psp_vertex *vertex = (pgfx_psp_vertex *)(pgfx.psp.vertex_data + pgfx.psp.vertex_data_used);
        vertex->texcoord = pgfx.texcoord;
        unsigned int color = (pgfx.color.r | pgfx.color.g << 8 | pgfx.color.b << 16 | pgfx.color.a << 24);
        vertex->color = color;
        vertex->pos.x = x;
        vertex->pos.y = y;
        vertex->pos.z = 0.0f;
        pgfx.psp.vertex_data_used += sizeof(pgfx_psp_vertex);
        pgfx.psp.vertex_count++;
    #endif
}

void pgfx_batch_index(unsigned short index)
{
    #if defined(PROCYON_DESKTOP)
        pgfx.gl.indices[pgfx.gl.index_count] = pgfx.gl.vertex_mark + index;
        pgfx.gl.index_count++;
        pgfx.gl.draws[pgfx.gl.draw_count - 1].index_count++;
    #elif defined(PROCYON_PSP)
        pgfx.psp.index_data[pgfx.psp.index_data_used] = index;
        pgfx.psp.index_data_used++;
    #endif
}

void pgfx_batch_color(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
    pgfx.color.r = r;
    pgfx.color.g = g;
    pgfx.color.b = b;
    pgfx.color.a = a;
}

void pgfx_batch_texcoord(float u, float v)
{
    pgfx.texcoord.x = u;
    pgfx.texcoord.y = v;
}

papp_texture pgfx_create_texture(void *data, int width, int height, bool swizzle)
{
    papp_texture texture = {0};

    #if defined(PROCYON_DESKTOP)
        GLuint texture_id;
        glGenTextures(1, &texture_id);
        glBindTexture(GL_TEXTURE_2D, texture_id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

        texture.id = texture_id;
        texture.width = width;
        texture.height = height;
    #elif defined(PROCYON_PSP)
        int padded_width = papp_closest_greater_pow2(width);
        int padded_height = papp_closest_greater_pow2(height);
        unsigned int size = pgfx_psp_get_buffer_size(padded_width, padded_height, GU_PSM_8888);

        #if 1 // 1 - vram, 0 - ram
            void *texture_data = pgfx_psp_edram_push_size(size);
        #else
            void *texture_data = memalign(16, size);
        #endif

        if(data)
        {
            if(swizzle)
            {
                void *data_buffer = memalign(16, size);
                pgfx_psp_copy_texture_data(data_buffer, data, padded_width, width, height);
                pgfx_psp_swizzle_fast((u8 *)texture_data, (const u8 *)data_buffer, padded_width * 4, padded_height);
                free(data_buffer);
            }
            else
            {
                pgfx_psp_copy_texture_data(texture_data, data, width, height, padded_width);
            }
        }
        else
        {
            memset(texture_data, 0xFF, size);
        }

        sceKernelDcacheWritebackInvalidateAll();

        texture.data = texture_data;
        texture.width = width;
        texture.height = height;
        texture.padded_width = padded_width;
        texture.padded_height = padded_height;
        texture.swizzled = swizzle;
    #endif

    return texture;
}

unsigned int papp_closest_greater_pow2(const unsigned int value)
{
    if(value == 0 || value > (1 << 31))
        return 0;
    unsigned int poweroftwo = 1;
    while (poweroftwo < value)
        poweroftwo <<= 1;
    return poweroftwo;
}