#include "pe_graphics.h"

#include "core/p_assert.h"
#include "pe_math.h"
#include "utility/pe_trace.h"

#include <stdbool.h>
#include <string.h>

peGraphics pe_graphics = {0};
peDynamicDrawState dynamic_draw = {0};

void pe_graphics_mode_3d_begin(peCamera camera) {
    pe_graphics_dynamic_draw_draw_batches();
    pe_graphics_dynamic_draw_new_batch();

    P_ASSERT(pe_graphics.mode == peGraphicsMode_2D);
    pe_graphics.mode = peGraphicsMode_3D;

    pe_graphics_matrix_mode(peMatrixMode_Projection);
    float aspect_ratio = (float)pe_screen_width()/(float)pe_screen_height();
    pMat4 matrix_perspective = pe_matrix_perspective(camera.fovy, aspect_ratio, 1.0f, 1000.0f);
    pe_graphics_matrix_set(&matrix_perspective);

    pe_graphics_matrix_mode(peMatrixMode_View);
    pMat4 matrix_view = p_look_at_rh(camera.position, camera.target, camera.up);
    pe_graphics_matrix_set(&matrix_view);

    pe_graphics_matrix_mode(peMatrixMode_Model);
    pe_graphics_matrix_identity();

    pe_graphics_matrix_update();

    pe_graphics_set_depth_test(true);
}

void pe_graphics_mode_3d_end(void) {
    pe_graphics_dynamic_draw_draw_batches();
    pe_graphics_dynamic_draw_new_batch();

    P_ASSERT(pe_graphics.mode == peGraphicsMode_3D);
    pe_graphics.mode = peGraphicsMode_2D;

    pe_graphics_matrix_mode(peMatrixMode_View);
    pe_graphics_matrix_identity();
    pe_graphics_matrix_mode(peMatrixMode_Model);
    pe_graphics_matrix_identity();
    pe_graphics.matrix_dirty[peGraphicsMode_2D][peMatrixMode_Projection] = true;

    pe_graphics_matrix_update();

    pe_graphics_set_depth_test(false);
}

void pe_graphics_matrix_mode(peMatrixMode mode) {
    P_ASSERT(mode >= 0);
    P_ASSERT(mode < peMatrixMode_Count);
    pe_graphics.matrix_mode = mode;
}

void pe_graphics_matrix_set(pMat4 *matrix) {
    memcpy(&pe_graphics.matrix[pe_graphics.mode][pe_graphics.matrix_mode], matrix, sizeof(pMat4));
    pe_graphics.matrix_dirty[pe_graphics.mode][pe_graphics.matrix_mode] = true;
    if (pe_graphics.matrix_mode == peMatrixMode_Model) {
        pe_graphics.matrix_model_is_identity[pe_graphics.mode] = false;
    }
}

void pe_graphics_matrix_identity(void) {
    pe_graphics.matrix[pe_graphics.mode][pe_graphics.matrix_mode] = p_mat4_i();
    pe_graphics.matrix_dirty[pe_graphics.mode][pe_graphics.matrix_mode] = true;
    if (pe_graphics.matrix_mode == peMatrixMode_Model) {
        pe_graphics.matrix_model_is_identity[pe_graphics.mode] = true;
    }
}

int pe_graphics_primitive_vertex_count(pePrimitive primitive) {
    P_ASSERT(primitive >= 0);
    P_ASSERT(primitive < pePrimitive_Count);
    int result;
    switch (primitive) {
        case pePrimitive_Points: result = 1; break;
        case pePrimitive_Lines: result = 2; break;
        case pePrimitive_Triangles: result = 3; break;
        default: result = 0; break;
    }
    return result;
}

bool pe_graphics_dynamic_draw_vertex_reserve(int count) {
    bool can_fit = (dynamic_draw.vertex_used + count <= PE_MAX_DYNAMIC_DRAW_VERTEX_COUNT);
    P_ASSERT(can_fit);
    return can_fit;
}

void pe_graphics_dynamic_draw_new_batch(void) {
    P_ASSERT(dynamic_draw.batch_current < PE_MAX_DYNAMIC_DRAW_BATCH_COUNT);
    if (dynamic_draw.batch[dynamic_draw.batch_current].vertex_count > 0) {
        dynamic_draw.batch_current += 1;
    }
    dynamic_draw.batch[dynamic_draw.batch_current].primitive = dynamic_draw.primitive;
    dynamic_draw.batch[dynamic_draw.batch_current].texture = dynamic_draw.texture;
    dynamic_draw.batch[dynamic_draw.batch_current].vertex_offset = dynamic_draw.vertex_used;
    dynamic_draw.batch[dynamic_draw.batch_current].vertex_count = 0;
}

void pe_graphics_dynamic_draw_clear(void) {
    if (dynamic_draw.vertex_used > 0) {
        bool all_batches_drawn = (dynamic_draw.batch_drawn_count > dynamic_draw.batch_current);
        bool current_batch_empty = (dynamic_draw.batch[dynamic_draw.batch_current].vertex_count == 0);
        P_ASSERT(all_batches_drawn || current_batch_empty);
    }
    dynamic_draw.vertex_used = 0;
    dynamic_draw.batch_current = 0;
    dynamic_draw.batch_drawn_count = 0;
    dynamic_draw.batch[0].vertex_count = 0;
}

void pe_graphics_dynamic_draw_set_primitive(pePrimitive primitive) {
    dynamic_draw.primitive = primitive;
    if (dynamic_draw.batch[dynamic_draw.batch_current].primitive != primitive) {
        pe_graphics_dynamic_draw_new_batch();
    }
}

void pe_graphics_dynamic_draw_set_texture(peTexture *texture) {
    dynamic_draw.texture = texture;
    if (dynamic_draw.batch[dynamic_draw.batch_current].texture != texture) {
        pe_graphics_dynamic_draw_new_batch();
    }
}

void pe_graphics_dynamic_draw_set_normal(pVec3 normal) {
    dynamic_draw.normal = normal;
}

void pe_graphics_dynamic_draw_set_texcoord(pVec2 texcoord) {
    dynamic_draw.texcoord = texcoord;
}

void pe_graphics_dynamic_draw_set_color(peColor color) {
    dynamic_draw.color = color;
}

void pe_graphics_dynamic_draw_do_lighting(bool do_lighting) {
    dynamic_draw.do_lighting = do_lighting;
    if (dynamic_draw.batch[dynamic_draw.batch_current].do_lighting != do_lighting) {
        pe_graphics_dynamic_draw_new_batch();
    }
}

void pe_graphics_dynamic_draw_begin_primitive(pePrimitive primitive) {
    pe_graphics_dynamic_draw_set_primitive(primitive);
    pe_graphics_dynamic_draw_set_texture(NULL);
    pe_graphics_dynamic_draw_set_color(PE_COLOR_WHITE);
    pe_graphics_dynamic_draw_do_lighting(false);
}

void pe_graphics_dynamic_draw_begin_primitive_textured(pePrimitive primitive, peTexture *texture) {
    pe_graphics_dynamic_draw_set_primitive(primitive);
    pe_graphics_dynamic_draw_set_texture(texture);
    pe_graphics_dynamic_draw_set_color(PE_COLOR_WHITE);
    pe_graphics_dynamic_draw_do_lighting(false);
}

void pe_graphics_dynamic_draw_push_vec2(pVec2 position) {
    pVec3 vec3 = p_vec3(position.x, position.y, 0.0f);
    pe_graphics_dynamic_draw_push_vec3(vec3);
}

void pe_graphics_draw_point(pVec2 position, peColor color) {
    pe_graphics_dynamic_draw_begin_primitive(pePrimitive_Points);
    pe_graphics_dynamic_draw_set_color(color);
    pe_graphics_dynamic_draw_push_vec2(position);
}

void pe_graphics_draw_point_int(int pos_x, int pos_y, peColor color) {
    pVec2 position = p_vec2((float)pos_x, (float)pos_y);
    pe_graphics_draw_point(position, color);
}

void pe_graphics_draw_line(pVec2 start_position, pVec2 end_position, peColor color) {
    pe_graphics_dynamic_draw_begin_primitive(pePrimitive_Lines);
    pe_graphics_dynamic_draw_set_color(color);
    pe_graphics_dynamic_draw_push_vec2(start_position);
    pe_graphics_dynamic_draw_push_vec2(end_position);
}

void pe_graphics_draw_rectangle(float x, float y, float width, float height, peColor color) {
    pe_graphics_dynamic_draw_begin_primitive(pePrimitive_Triangles);
    pe_graphics_dynamic_draw_set_color(color);

    pVec2 positions[4] = {
        p_vec2(x, y), // top left
        p_vec2(x + width, y), // top right
        p_vec2(x, y + height), // bottom left
        p_vec2(x + width, y + height) // bottom_right
    };
    int indices[6] = { 0, 2, 3, 0, 3, 1 };

    for (int i = 0; i < 6; i += 1) {
        pe_graphics_dynamic_draw_push_vec2(positions[indices[i]]);
    }
}

void pe_graphics_draw_texture(peTexture *texture, float x, float y, peColor tint) {
    pe_graphics_dynamic_draw_begin_primitive_textured(pePrimitive_Triangles, texture);
    pe_graphics_dynamic_draw_set_color(tint);

    pVec2 positions[4] = {
        p_vec2(x, y), // top left
        p_vec2(x + texture->width, y), // top right
        p_vec2(x, y + texture->height), // bottom left
        p_vec2(x + texture->width, y + texture->height) // bottom_right
    };
    pVec2 texcoords[4] = {
        p_vec2(0.0f, 0.0f), // top left
        p_vec2(1.0f, 0.0f), // top right
        p_vec2(0.0f, 1.0f), // bottom left
        p_vec2(1.0f, 1.0f), // bottom right
    };
    int indices[6] = { 0, 2, 3, 0, 3, 1 };

    for (int i = 0; i < 6; i += 1) {
        pe_graphics_dynamic_draw_set_texcoord(texcoords[indices[i]]);
        pe_graphics_dynamic_draw_push_vec2(positions[indices[i]]);
    }
}

void pe_graphics_draw_point_3D(pVec3 position, peColor color) {
    pe_graphics_dynamic_draw_begin_primitive(pePrimitive_Points);
    pe_graphics_dynamic_draw_set_color(color);
    pe_graphics_dynamic_draw_push_vec3(position);
}

void pe_graphics_draw_line_3D(pVec3 start_position, pVec3 end_position, peColor color) {
    pe_graphics_dynamic_draw_begin_primitive(pePrimitive_Lines);
    pe_graphics_dynamic_draw_set_color(color);
    pe_graphics_dynamic_draw_push_vec3(start_position);
    pe_graphics_dynamic_draw_push_vec3(end_position);
}

void pe_graphics_draw_grid_3D(int slices, float spacing) {
    pe_graphics_dynamic_draw_begin_primitive(pePrimitive_Lines);

    int half_slices = slices/2;
    for (int slice = -half_slices; slice <= half_slices; slice += 1) {
        int channel = (slice == 0 ? 192 : 64);
        pe_graphics_dynamic_draw_set_color((peColor){ channel, channel, channel, 255 });
        pe_graphics_dynamic_draw_push_vec3(p_vec3((float)slice*spacing, 0.0f, (float)-half_slices*spacing));
        pe_graphics_dynamic_draw_push_vec3(p_vec3((float)slice*spacing, 0.0f, (float)half_slices*spacing));
        pe_graphics_dynamic_draw_push_vec3(p_vec3((float)-half_slices*spacing, 0.0f, (float)slice*spacing));
        pe_graphics_dynamic_draw_push_vec3(p_vec3((float)half_slices*spacing, 0.0f, (float)slice*spacing));
    }
}

pVec4 pe_color_to_vec4(peColor color) {
    pVec4 vec4 = {
        .r = (float)color.r / 255.0f,
        .g = (float)color.g / 255.0f,
        .b = (float)color.b / 255.0f,
        .a = (float)color.a / 255.0f,
    };
    return vec4;
}

uint16_t pe_color_to_5650(peColor color) {
	uint16_t max_5_bit = (1U << 5) - 1;
	uint16_t max_6_bit = (1U << 6) - 1;
	uint16_t max_8_bit = (1U << 8) - 1;
	uint16_t r = (((uint16_t)color.r * max_5_bit) / max_8_bit) & max_5_bit;
	uint16_t g = (((uint16_t)color.g * max_6_bit) / max_8_bit) & max_6_bit;
	uint16_t b = (((uint16_t)color.b * max_5_bit) / max_8_bit) & max_5_bit;
	uint16_t result = b << 11 | g << 5 | r;
	return result;
}

uint32_t pe_color_to_8888(peColor color) {
	uint32_t r = (uint32_t)color.r;
	uint32_t g = (uint32_t)color.g;
	uint32_t b = (uint32_t)color.b;
	uint32_t a = (uint32_t)color.a;
	uint32_t result = r | g << 8 | b << 16 | a << 24;
	return result;
}

peRay pe_get_mouse_ray(pVec2 mouse, peCamera camera) {
    float window_width_float = (float)pe_screen_width();
    float window_height_float = (float)pe_screen_height();
    pMat4 projection = pe_matrix_perspective(55.0f, window_width_float/window_height_float, 1.0f, 1000.0f);
    pMat4 view = p_look_at_rh(camera.position, camera.target, camera.up);

    pVec3 near_point = pe_unproject_vec3(p_vec3(mouse.x, mouse.y, 0.0f), 0.0f, 0.0f, window_width_float, window_height_float, 0.0f, 1.0f, projection, view);
    pVec3 far_point = pe_unproject_vec3(p_vec3(mouse.x, mouse.y, 1.0f), 0.0f, 0.0f, window_width_float, window_height_float, 0.0f, 1.0f, projection, view);
    pVec3 direction = p_vec3_norm(p_vec3_sub(far_point, near_point));
    peRay ray = {
        .position = camera.position,
        .direction = direction,
    };
    return ray;
}