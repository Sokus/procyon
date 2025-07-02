#include "graphics/p_graphics.h"

#include "math/p_math.h"

#warning p_graphics unimplemented

//
// GENERAL IMPLEMENTATIONS
//

void p_graphics_init(int, int) {
}

void p_graphics_shutdown(void) {
}

int p_screen_width(void) {
    return 0;
}

int p_screen_height(void) {
    return 0;
}

void p_clear_background(pColor color) {
}

void p_graphics_frame_begin(void) {
}

void p_graphics_frame_end(void) {
}

void p_graphics_set_depth_test(bool enable) {
}

void p_graphics_set_lighting(bool enable) {
}

void p_graphics_set_light_vector(pVec3 light_vector) {
}

void p_graphics_set_diffuse_color(pColor color) {
}

void p_graphics_matrix_update(void) {
}

pMat4 p_matrix_perspective(float fovy, float aspect_ratio, float near_z, float far_z) {
	return p_perspective_rh_no(fovy * P_DEG2RAD, aspect_ratio, near_z, far_z);
}

pMat4 p_matrix_orthographic(float left, float right, float bottom, float top, float near_z, float far_z) {
    return p_orthographic_rh_no(left, right, bottom, top, near_z, far_z);
}

pTexture p_texture_create(void *data, int width, int height) {
}

void p_texture_bind(pTexture texture) {
}

void p_texture_bind_default(void) {
}

void p_graphics_dynamic_draw_draw_batches(void) {
}

void p_graphics_dynamic_draw_push_vec3(pVec3 position) {
}

//
// INTERNAL IMPLEMENTATIONS
//
