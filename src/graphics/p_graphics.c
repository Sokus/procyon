#include "p_graphics.h"

#include "core/p_assert.h"
#include "graphics/p_graphics_math.h"
#include "utility/p_trace.h"

#include <stdbool.h>
#include <string.h>

pGraphics p_graphics = {0};
pDynamicDrawState dynamic_draw = {0};

void p_graphics_mode_3d_begin(pCamera camera) {
    p_graphics_dynamic_draw_draw_batches();
    p_graphics_dynamic_draw_new_batch();

    P_ASSERT(p_graphics.mode == pGraphicsMode_2D);
    p_graphics.mode = pGraphicsMode_3D;

    p_graphics_matrix_mode(pMatrixMode_Projection);
    float aspect_ratio = (float)p_screen_width()/(float)p_screen_height();
    pMat4 matrix_perspective = p_matrix_perspective(camera.fovy, aspect_ratio, 1.0f, 1000.0f);
    p_graphics_matrix_set(&matrix_perspective);

    p_graphics_matrix_mode(pMatrixMode_View);
    pMat4 matrix_view = p_look_at_rh(camera.position, camera.target, camera.up);
    p_graphics_matrix_set(&matrix_view);

    p_graphics_matrix_mode(pMatrixMode_Model);
    p_graphics_matrix_identity();

    p_graphics_matrix_update();

    p_graphics_set_depth_test(true);
}

void p_graphics_mode_3d_end(void) {
    p_graphics_dynamic_draw_draw_batches();
    p_graphics_dynamic_draw_new_batch();

    P_ASSERT(p_graphics.mode == pGraphicsMode_3D);
    p_graphics.mode = pGraphicsMode_2D;

    p_graphics_matrix_mode(pMatrixMode_View);
    p_graphics_matrix_identity();
    p_graphics_matrix_mode(pMatrixMode_Model);
    p_graphics_matrix_identity();
    p_graphics.matrix_dirty[pGraphicsMode_2D][pMatrixMode_Projection] = true;

    p_graphics_matrix_update();

    p_graphics_set_depth_test(false);
}

void p_graphics_matrix_mode(pMatrixMode mode) {
    P_ASSERT(mode >= 0);
    P_ASSERT(mode < pMatrixMode_Count);
    p_graphics.matrix_mode = mode;
}

void p_graphics_matrix_set(pMat4 *matrix) {
    memcpy(&p_graphics.matrix[p_graphics.mode][p_graphics.matrix_mode], matrix, sizeof(pMat4));
    p_graphics.matrix_dirty[p_graphics.mode][p_graphics.matrix_mode] = true;
    if (p_graphics.matrix_mode == pMatrixMode_Model) {
        p_graphics.matrix_model_is_identity[p_graphics.mode] = false;
    }
}

void p_graphics_matrix_identity(void) {
    p_graphics.matrix[p_graphics.mode][p_graphics.matrix_mode] = p_mat4_i();
    p_graphics.matrix_dirty[p_graphics.mode][p_graphics.matrix_mode] = true;
    if (p_graphics.matrix_mode == pMatrixMode_Model) {
        p_graphics.matrix_model_is_identity[p_graphics.mode] = true;
    }
}

int p_graphics_primitive_vertex_count(pPrimitive primitive) {
    P_ASSERT(primitive >= 0);
    P_ASSERT(primitive < pPrimitive_Count);
    int result;
    switch (primitive) {
        case pPrimitive_Points: result = 1; break;
        case pPrimitive_Lines: result = 2; break;
        case pPrimitive_Triangles: result = 3; break;
        default: result = 0; break;
    }
    return result;
}

bool p_graphics_primitive_can_continue(pPrimitive primitive) {
    P_ASSERT(primitive >= 0);
    P_ASSERT(primitive < pPrimitive_Count);
    bool result = false;
    switch (primitive) {
        case pPrimitive_Points: result = true; break;
        case pPrimitive_Lines: result = true; break;
        case pPrimitive_Triangles: result = true; break;
        case pPrimitive_TriangleStrip: result = false; break;
        default: P_PANIC(); break;
    }
    return result;
}

bool p_graphics_dynamic_draw_vertex_reserve(int count) {
    bool can_fit = (dynamic_draw.vertex_used + count <= P_MAX_DYNAMIC_DRAW_VERTEX_COUNT);
    P_ASSERT(can_fit);
    return can_fit;
}

void p_graphics_dynamic_draw_new_batch(void) {
    P_ASSERT(dynamic_draw.batch_current < P_MAX_DYNAMIC_DRAW_BATCH_COUNT);
    if (dynamic_draw.batch[dynamic_draw.batch_current].vertex_count > 0) {
        dynamic_draw.batch_current += 1;
    }
    if (dynamic_draw.batch_current < P_MAX_DYNAMIC_DRAW_BATCH_COUNT) {
        dynamic_draw.batch[dynamic_draw.batch_current].primitive = dynamic_draw.primitive;
        dynamic_draw.batch[dynamic_draw.batch_current].texture = dynamic_draw.texture;
        dynamic_draw.batch[dynamic_draw.batch_current].vertex_offset = dynamic_draw.vertex_used;
        dynamic_draw.batch[dynamic_draw.batch_current].vertex_count = 0;
    }
}


void p_graphics_dynamic_draw_clear(void) {
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

void p_graphics_dynamic_draw_set_primitive(pPrimitive primitive) {
    dynamic_draw.primitive = primitive;
    pPrimitive current_primitive = dynamic_draw.batch[dynamic_draw.batch_current].primitive;
    if (current_primitive != primitive || !p_graphics_primitive_can_continue(current_primitive)) {
        p_graphics_dynamic_draw_new_batch();
    }
}

void p_graphics_dynamic_draw_set_texture(pTexture *texture) {
    dynamic_draw.texture = texture;
    if (dynamic_draw.batch[dynamic_draw.batch_current].texture != texture) {
        p_graphics_dynamic_draw_new_batch();
    }
}

void p_graphics_dynamic_draw_set_normal(pVec3 normal) {
    dynamic_draw.normal = normal;
}

void p_graphics_dynamic_draw_set_texcoord(pVec2 texcoord) {
    dynamic_draw.texcoord = texcoord;
}

void p_graphics_dynamic_draw_set_color(pColor color) {
    dynamic_draw.color = color;
}

void p_graphics_dynamic_draw_do_lighting(bool do_lighting) {
    dynamic_draw.do_lighting = do_lighting;
    if (dynamic_draw.batch[dynamic_draw.batch_current].do_lighting != do_lighting) {
        p_graphics_dynamic_draw_new_batch();
    }
}

void p_graphics_dynamic_draw_begin_primitive(pPrimitive primitive) {
    p_graphics_dynamic_draw_set_primitive(primitive);
    p_graphics_dynamic_draw_set_texture(NULL);
    p_graphics_dynamic_draw_set_texcoord(p_vec2(0.0f, 0.0f));
    p_graphics_dynamic_draw_set_color(P_COLOR_WHITE);
    p_graphics_dynamic_draw_do_lighting(false);
}

void p_graphics_dynamic_draw_begin_primitive_textured(pPrimitive primitive, pTexture *texture) {
    p_graphics_dynamic_draw_set_primitive(primitive);
    p_graphics_dynamic_draw_set_texture(texture);
    p_graphics_dynamic_draw_set_texcoord(p_vec2(0.0f, 0.0f));
    p_graphics_dynamic_draw_set_color(P_COLOR_WHITE);
    p_graphics_dynamic_draw_do_lighting(false);
}

void p_graphics_dynamic_draw_push_vec2(pVec2 position) {
    pVec3 vec3 = p_vec3(position.x, position.y, 0.0f);
    p_graphics_dynamic_draw_push_vec3(vec3);
}

void p_graphics_draw_point(pVec2 position, pColor color) {
    p_graphics_dynamic_draw_begin_primitive(pPrimitive_Points);
    p_graphics_dynamic_draw_set_color(color);
    p_graphics_dynamic_draw_push_vec2(position);
}

void p_graphics_draw_point_int(int pos_x, int pos_y, pColor color) {
    pVec2 position = p_vec2((float)pos_x, (float)pos_y);
    p_graphics_draw_point(position, color);
}

void p_graphics_draw_line(pVec2 start_position, pVec2 end_position, pColor color) {
    p_graphics_dynamic_draw_begin_primitive(pPrimitive_Lines);
    p_graphics_dynamic_draw_set_color(color);
    p_graphics_dynamic_draw_push_vec2(start_position);
    p_graphics_dynamic_draw_push_vec2(end_position);
}

void p_graphics_draw_rectangle(float x, float y, float width, float height, pColor color) {
    p_graphics_dynamic_draw_begin_primitive(pPrimitive_Triangles);
    p_graphics_dynamic_draw_set_color(color);

    pVec2 positions[4] = {
        p_vec2(x, y), // top left
        p_vec2(x + width, y), // top right
        p_vec2(x, y + height), // bottom left
        p_vec2(x + width, y + height) // bottom_right
    };
    int indices[6] = { 0, 2, 3, 0, 3, 1 };

    for (int i = 0; i < 6; i += 1) {
        p_graphics_dynamic_draw_push_vec2(positions[indices[i]]);
    }
}

void p_graphics_draw_texture(pTexture *texture, float x, float y, float scale, pColor tint) {
    float tw = (float)texture->width;
    float th = (float)texture->height;
    pRect src = {
        .x = 0.0f, .y = 0.0f,
        .width = tw, .height = th,
    };
    pRect dst = {
        .x = x, .y = y,
        .width = scale*tw, .height = scale*th,
    };
    p_graphics_draw_texture_ex(texture, src, dst, tint);
}

void p_graphics_draw_texture_ex(pTexture *texture, pRect src, pRect dst, pColor tint) {
    p_graphics_dynamic_draw_begin_primitive_textured(pPrimitive_Triangles, texture);
    p_graphics_dynamic_draw_set_color(tint);

    pVec2 positions[4] = {
        p_vec2(          dst.x,            dst.y), // top left
        p_vec2(dst.x+dst.width,            dst.y), // top right
        p_vec2(          dst.x, dst.y+dst.height), // bottom left
        p_vec2(dst.x+dst.width, dst.y+dst.height), // bottom_right
    };
    pRect tc_rect = {
        .x = src.x / (float)texture->width,
        .y = src.y / (float)texture->height,
        .width = src.width / (float)texture->width,
        .height = src.height / (float)texture->height
    };
    pVec2 texcoords[4] = {
        p_vec2(              tc_rect.x,                tc_rect.y), // top left
        p_vec2(tc_rect.x+tc_rect.width,                tc_rect.y), // top right
        p_vec2(              tc_rect.x, tc_rect.y+tc_rect.height), // bottom left
        p_vec2(tc_rect.x+tc_rect.width, tc_rect.y+tc_rect.height), // bottom right
    };
    int indices[6] = { 0, 2, 3, 0, 3, 1 };

    for (int i = 0; i < 6; i += 1) {
        p_graphics_dynamic_draw_set_texcoord(texcoords[indices[i]]);
        p_graphics_dynamic_draw_push_vec2(positions[indices[i]]);
    }
}

void p_graphics_draw_point_3D(pVec3 position, pColor color) {
    p_graphics_dynamic_draw_begin_primitive(pPrimitive_Points);
    p_graphics_dynamic_draw_set_color(color);
    p_graphics_dynamic_draw_push_vec3(position);
}

void p_graphics_draw_line_3D(pVec3 start_position, pVec3 end_position, pColor color) {
    p_graphics_dynamic_draw_begin_primitive(pPrimitive_Lines);
    p_graphics_dynamic_draw_set_color(color);
    p_graphics_dynamic_draw_push_vec3(start_position);
    p_graphics_dynamic_draw_push_vec3(end_position);
}

void p_graphics_draw_grid_3D(int slices, float spacing) {
    p_graphics_dynamic_draw_begin_primitive(pPrimitive_Lines);

    int half_slices = slices/2;
    for (int slice = -half_slices; slice <= half_slices; slice += 1) {
        int channel = (slice == 0 ? 192 : 64);
        p_graphics_dynamic_draw_set_color((pColor){ channel, channel, channel, 255 });
        p_graphics_dynamic_draw_push_vec3(p_vec3((float)slice*spacing, 0.0f, (float)-half_slices*spacing));
        p_graphics_dynamic_draw_push_vec3(p_vec3((float)slice*spacing, 0.0f, (float)half_slices*spacing));
        p_graphics_dynamic_draw_push_vec3(p_vec3((float)-half_slices*spacing, 0.0f, (float)slice*spacing));
        p_graphics_dynamic_draw_push_vec3(p_vec3((float)half_slices*spacing, 0.0f, (float)slice*spacing));
    }
}

pVec4 p_color_to_vec4(pColor color) {
    pVec4 vec4 = {
        .r = (float)color.r / 255.0f,
        .g = (float)color.g / 255.0f,
        .b = (float)color.b / 255.0f,
        .a = (float)color.a / 255.0f,
    };
    return vec4;
}

uint16_t p_color_to_5650(pColor color) {
	uint16_t max_5_bit = (1U << 5) - 1;
	uint16_t max_6_bit = (1U << 6) - 1;
	uint16_t max_8_bit = (1U << 8) - 1;
	uint16_t r = (((uint16_t)color.r * max_5_bit) / max_8_bit) & max_5_bit;
	uint16_t g = (((uint16_t)color.g * max_6_bit) / max_8_bit) & max_6_bit;
	uint16_t b = (((uint16_t)color.b * max_5_bit) / max_8_bit) & max_5_bit;
	uint16_t result = b << 11 | g << 5 | r;
	return result;
}

uint32_t p_color_to_8888(pColor color) {
	uint32_t r = (uint32_t)color.r;
	uint32_t g = (uint32_t)color.g;
	uint32_t b = (uint32_t)color.b;
	uint32_t a = (uint32_t)color.a;
	uint32_t result = r | g << 8 | b << 16 | a << 24;
	return result;
}

pRay p_get_mouse_ray(pVec2 mouse, pCamera camera) {
    float window_width_float = (float)p_screen_width();
    float window_height_float = (float)p_screen_height();
    pMat4 projection = p_matrix_perspective(55.0f, window_width_float/window_height_float, 1.0f, 1000.0f);
    pMat4 view = p_look_at_rh(camera.position, camera.target, camera.up);

    pVec3 near_point = pe_unproject_vec3(p_vec3(mouse.x, mouse.y, 0.0f), 0.0f, 0.0f, window_width_float, window_height_float, 0.0f, 1.0f, projection, view);
    pVec3 far_point = pe_unproject_vec3(p_vec3(mouse.x, mouse.y, 1.0f), 0.0f, 0.0f, window_width_float, window_height_float, 0.0f, 1.0f, projection, view);
    pVec3 direction = p_vec3_norm(p_vec3_sub(far_point, near_point));
    pRay ray = {
        .position = camera.position,
        .direction = direction,
    };
    return ray;
}
