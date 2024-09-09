#include "pe_model.h"

#include "core/p_assert.h"
#include "core/p_heap.h"
#include "core/p_file.h"
#include "core/p_time.h"
#include "graphics/p3d.h"
#include "utility/pe_trace.h"

#include <string.h>

#include "stb_image.h"

peMaterial pe_default_material(void) {
    peMaterial result = {
		.diffuse_color = PE_COLOR_WHITE,
		.has_diffuse_map = false
	};
	return result;
}

peAnimationJoint pe_concatenate_animation_joints(peAnimationJoint parent, peAnimationJoint child) {
	peAnimationJoint result;
	result.scale = p_vec3_mul(child.scale, parent.scale);
	result.rotation = p_quat_mul(parent.rotation, child.rotation);
	pMat4 parent_rotation_matrix = p_quat_to_mat4(parent.rotation);
	pVec3 child_scaled_position = p_vec3_mul(child.translation, parent.scale);
	result.translation = p_vec3_add(p_mat4_mul_vec4(parent_rotation_matrix, p_vec4v(child_scaled_position, 1.0f)).xyz, parent.translation);
	return result;
}

static float pe_int16_to_float(int16_t value, float a, float b) {
    float float_negative_one_to_one;
    if (value >= 0) {
        float_negative_one_to_one = (float)value / (float)INT16_MAX;
    } else {
        float_negative_one_to_one = -1.0f * (float)value / (float)INT16_MIN;
    }
    P_ASSERT(b > a);
    float float_a_to_b = (float_negative_one_to_one + 1.0f) * (b - a) / 2.0f + a;
    return float_a_to_b;
}

static float pe_uint16_to_float(uint16_t value, float a, float b) {
    float float_zero_to_one = (float)value / (float)UINT16_MAX;
    float float_a_to_b = float_zero_to_one * (b - a) + a;
    return float_a_to_b;
}

static void* pe_push_buffer_pointer(void **ptr, size_t amount, size_t *contents_left) {
    if (amount > *contents_left) return NULL;
    void *result = *ptr;
    *ptr = (uint8_t*)*ptr + amount;
    *contents_left -= amount;
    return result;
}

bool pe_parse_p3d(peArena *arena, peFileContents p3d_file_contents, p3dFile *p3d) {
	void *p3d_file_pointer = p3d_file_contents.data;
	size_t p3d_file_contents_left = p3d_file_contents.size;

	p3d->static_info = pe_push_buffer_pointer(&p3d_file_pointer, sizeof(p3dStaticInfo), &p3d_file_contents_left);
	if (!pe_parse_p3d_static_info(p3d->static_info)) return false;

	size_t mesh_size = p3d->static_info->num_meshes * sizeof(p3dMesh);
	p3d->mesh = pe_push_buffer_pointer(&p3d_file_pointer, mesh_size, &p3d_file_contents_left);
	if (!p3d->mesh) return false;

    size_t material_size = p3d->static_info->num_materials * sizeof(p3dMaterial);
	p3d->material = pe_push_buffer_pointer(&p3d_file_pointer, material_size, &p3d_file_contents_left);
	if (!p3d->material) return false;

    if (p3d->static_info->is_portable) {
    	size_t subskeleton_size = p3d->static_info->num_subskeletons * sizeof(p3dSubskeleton);
    	p3d->subskeleton = pe_push_buffer_pointer(&p3d_file_pointer, subskeleton_size, &p3d_file_contents_left);
    	if (!p3d->subskeleton) return false;
    }

	size_t animation_size = p3d->static_info->num_animations * sizeof(p3dAnimation);
	p3d->animation = pe_push_buffer_pointer(&p3d_file_pointer, animation_size, &p3d_file_contents_left);
	if (!p3d->animation) return false;

    if (!p3d->static_info->is_portable) {
    	size_t vertex_size = p3d->static_info->num_vertex * sizeof(p3dVertex);
    	p3d->desktop.vertex = pe_push_buffer_pointer(&p3d_file_pointer, vertex_size, &p3d_file_contents_left);
    	if (!p3d->desktop.vertex) return false;

    	if (p3d->static_info->num_index > 0) {
    		size_t index_size = p3d->static_info->num_index * sizeof(uint32_t);
        	p3d->desktop.index = pe_push_buffer_pointer(&p3d_file_pointer, index_size, &p3d_file_contents_left);
        	if (!p3d->desktop.index) return false;
    	} else {
    		p3d->desktop.index = NULL;
    	}
    }

    if (p3d->static_info->is_portable) {
    	p3d->portable.vertex = (void**)pe_arena_alloc(arena, p3d->static_info->num_meshes * sizeof(void*));
    	p3d->portable.vertex_size = (size_t *)pe_arena_alloc(arena, p3d->static_info->num_meshes * sizeof(size_t));
    	p3d->portable.index = (void**)pe_arena_alloc(arena, p3d->static_info->num_meshes * sizeof(void*));
    	p3d->portable.index_size = pe_arena_alloc(arena, p3d->static_info->num_meshes * sizeof(size_t));
    	for (int m = 0; m < p3d->static_info->num_meshes; m += 1) {
    		uint8_t num_weights = p3d->subskeleton[p3d->mesh[m].subskeleton_index].num_bones;
    		bool has_diffuse_map = (
    			p3d->material[p3d->mesh[m].material_index].diffuse_image_file_name[0] != '\0'
    		);
    		size_t single_vertex_size = (
    			num_weights * (size_t)(p3d->static_info->bone_weight_size) + // weights
    			(num_weights % 2) * sizeof(uint8_t) + // weight padding
    			(has_diffuse_map ? 2 * sizeof(int16_t) : 0) + // uv
    			3 * sizeof(int16_t) +         // normal
    			3 * sizeof(int16_t)	          // position
    		);

    		size_t vertex_size = single_vertex_size * p3d->mesh[m].num_vertex;
        	p3d->portable.vertex[m] = pe_push_buffer_pointer(&p3d_file_pointer, vertex_size, &p3d_file_contents_left);
        	if (!p3d->portable.vertex[m]) return false;
    		p3d->portable.vertex_size[m] = vertex_size;

    		if (p3d->mesh[m].num_index > 0) {
    			size_t index_size = p3d->mesh[m].num_index * sizeof(uint16_t);
            	p3d->portable.index[m] = pe_push_buffer_pointer(&p3d_file_pointer, index_size, &p3d_file_contents_left);
            	if (!p3d->portable.index[m]) return false;
    			p3d->portable.index_size[m] = index_size;
    		} else {
    			p3d->portable.index[m] = NULL;
    			p3d->portable.index_size[m] = 0;
    		}
    	}
    }

	size_t bone_parent_index_size = p3d->static_info->num_bones * sizeof(uint16_t);
	p3d->bone_parent_index = pe_push_buffer_pointer(&p3d_file_pointer, bone_parent_index_size, &p3d_file_contents_left);
	if (!p3d->bone_parent_index) return false;

	size_t inverse_model_space_pose_matrix_size = p3d->static_info->num_bones * sizeof(p3dMatrix);
	p3d->inverse_model_space_pose_matrix = pe_push_buffer_pointer(&p3d_file_pointer, inverse_model_space_pose_matrix_size, &p3d_file_contents_left);
	if (!p3d->inverse_model_space_pose_matrix) return false;

	size_t animation_joint_size = (size_t)p3d->static_info->num_frames_total * (size_t)p3d->static_info->num_bones * sizeof(p3dAnimationJoint);
	p3d->animation_joint = pe_push_buffer_pointer(&p3d_file_pointer, animation_joint_size, &p3d_file_contents_left);
	if (!p3d->animation_joint) return false;

    return true;
}

void pe_model_alloc(peModel *model, peArena *arena, p3dFile *p3d) {
    size_t material_size = p3d->static_info->num_meshes * sizeof(peMaterial);
    size_t bone_inverse_model_space_pose_matrix_size = p3d->static_info->num_bones * sizeof(pMat4);
    size_t animation_size = p3d->static_info->num_animations * sizeof(peAnimation);
	size_t bone_parent_index_size = p3d->static_info->num_bones * sizeof(uint16_t);

    size_t num_index_size = p3d->static_info->num_meshes * sizeof(unsigned int); // desktop
    size_t index_offset_size = p3d->static_info->num_meshes * sizeof(unsigned int); // desktop
    size_t vertex_offset_size = p3d->static_info->num_meshes * sizeof(int); // desktop

    size_t mesh_size = p3d->static_info->num_meshes * sizeof(peMesh);

	model->material = pe_arena_alloc(arena, material_size);
	model->bone_inverse_model_space_pose_matrix = pe_arena_alloc(arena, bone_inverse_model_space_pose_matrix_size);

	model->animation = pe_arena_alloc(arena, animation_size);
	for (int a = 0; a < p3d->static_info->num_animations; a += 1) {
	    size_t num_frames = p3d->animation[a].num_frames;
	    size_t num_bones = p3d->static_info->num_bones;
		size_t num_animation_joint = num_frames * num_bones;
		size_t animation_joint_size = num_animation_joint * sizeof(peAnimationJoint);
		model->animation[a].frames = pe_arena_alloc(arena, animation_joint_size);
	}

    model->bone_parent_index = pe_arena_alloc(arena, bone_parent_index_size);
	model->mesh = pe_arena_alloc(arena, mesh_size);
	pe_model_alloc_mesh_data(model, arena, p3d);
}

peVertexSkinned pe_vertex_skinned_from_p3d(p3dVertex vertex_p3d, float scale) {
    peVertexSkinned result;
    for (int e = 0; e < 3; e += 1) {
        result.position[e] = scale * pe_int16_to_float(vertex_p3d.position[e], -1.0f, 1.0f);
    }
    for (int e = 0; e < 3; e += 1) {
        result.normal[e] = pe_int16_to_float(vertex_p3d.normal[e], -1.0f, 1.0f);
    }
    for (int e = 0; e < 2; e += 1) {
        result.texcoord[e] = pe_int16_to_float(vertex_p3d.texcoord[e], -1.0f, 1.0f);
    }
    result.color = PE_COLOR_WHITE.rgba;
    for (int e = 0; e < 4; e += 1) {
        result.bone_index[e] = (uint32_t)vertex_p3d.bone_index[e];
        result.bone_weight[e] = pe_uint16_to_float(vertex_p3d.bone_weight[e], 0.0f, 1.0f);
    }
    return result;
}

static void pe_model_load_materials(peModel *model, peArena *temp_arena, char *file_path, p3dStaticInfo *static_info, p3dMaterial *material) {
    stbi_set_flip_vertically_on_load(false);
	for (int m = 0; m < static_info->num_materials; m += 1) {
		model->material[m] = pe_default_material();
		model->material[m].diffuse_color.rgba = material[m].diffuse_color;
		if (material[m].diffuse_image_file_name[0] != '\0') {
			model->material[m].has_diffuse_map = true;

			int last_slash_index = 0;
			for (int c = 0; c < 256; c += 1) {
				if (file_path[c] == '\0') {
					break;
				}
				if (file_path[c] == '/') {
					last_slash_index = c;
				}
			}
			char diffuse_texture_path[512] = {0};
			memcpy(diffuse_texture_path, file_path, (last_slash_index+1) * sizeof(char));
			size_t diffuse_map_file_length = strnlen(material[m].diffuse_image_file_name, sizeof(material[m].diffuse_image_file_name));
			memcpy(diffuse_texture_path + last_slash_index + 1, material[m].diffuse_image_file_name, diffuse_map_file_length*sizeof(char));

			int w, h, channels;
			stbi_uc *stbi_data = stbi_load(diffuse_texture_path, &w, &h, &channels, STBI_rgb_alpha);
			model->material[m].diffuse_map = pe_texture_create(temp_arena, stbi_data, w, h);
			stbi_image_free(stbi_data);
		}
	}
}

peModel pe_model_load(peArena *temp_arena, char *file_path) {
    peArenaTemp temp_arena_memory = pe_arena_temp_begin(temp_arena);

	p3dFile p3d;
	peFileContents p3d_file_contents = pe_file_read_contents(temp_arena, file_path, false);
	pe_parse_p3d(temp_arena, p3d_file_contents, &p3d);

    peModel model = {0};
	peArena model_arena;
    {
		// TODO: this is basically a memory leak (xD)
		// so far we are only loading a single model to test things out
		// so we don't really care, but we should allocate models in
		// another way in the future (pool allocator?)
		size_t model_memory_size = P_KILOBYTES(512);
		void *model_memory = pe_heap_alloc(P_KILOBYTES(512));
		pe_arena_init(&model_arena, model_memory, model_memory_size);
        pe_model_alloc(&model, &model_arena, &p3d);
    }

    pe_model_load_static_info(&model, p3d.static_info);
    pe_model_load_materials(&model, temp_arena, file_path, p3d.static_info, p3d.material);
	pe_model_load_skeleton(&model, &p3d);
    pe_model_load_animations(&model, p3d.static_info, p3d.animation, p3d.animation_joint);
    pe_model_load_mesh_data(&model, temp_arena, &p3d);
    pe_model_load_writeback_arena(&model_arena);

	pe_arena_temp_end(temp_arena_memory);

	return model;
}

float frame_time = 1.0f/25.0f;
float current_frame_time = 0.0f;
int frame_index = 0;
uint64_t last_frame_time;
bool last_frame_time_initialized = false;

void pe_model_draw_set_model_matrix(peModel *model, pVec3 position, pVec3 rotation) {
    PE_TRACE_FUNCTION_BEGIN();
    pe_graphics_matrix_mode(peMatrixMode_Model);
    pMat4 translate = p_translate(position);
    pMat4 rotate = p_rotate_xyz(rotation);
    pMat4 scale = p_scale(p_vec3(model->scale, model->scale, model->scale));
    pMat4 model_matrix = p_mat4_mul(translate, p_mat4_mul(rotate, scale));
    pe_graphics_matrix_set(&model_matrix);
    pe_graphics_matrix_update();
    PE_TRACE_FUNCTION_END();
}

peAnimationJoint *pe_model_draw_get_model_space_animation_joints(peModel *model, peArena *arena) {
    PE_TRACE_FUNCTION_BEGIN();
    peAnimationJoint *model_space_joints = pe_arena_alloc(arena, model->num_bone * sizeof(peAnimationJoint));
    peAnimationJoint *animation_joints = &model->animation[0].frames[frame_index * model->num_bone];
    for (int b = 0; b < model->num_bone; b += 1) {
        if (model->bone_parent_index[b] < UINT16_MAX) {
            peAnimationJoint parent_transform = model_space_joints[model->bone_parent_index[b]];
            model_space_joints[b] = pe_concatenate_animation_joints(parent_transform, animation_joints[b]);
        } else {
            model_space_joints[b] = animation_joints[b];
        }
    }
    PE_TRACE_FUNCTION_END();
    return model_space_joints;
}

pMat4 *pe_model_draw_get_final_bone_matrix(peModel *model, peArena *arena, peAnimationJoint *model_space_joints) {
    PE_TRACE_FUNCTION_BEGIN();
    pMat4 *final_bone_matrix = pe_arena_alloc(arena, model->num_bone * sizeof(pMat4));
    for (int b = 0; b < model->num_bone; b += 1) {
        peAnimationJoint *animation_joint = &model_space_joints[b];
		pMat4 translation = p_translate(animation_joint->translation);
		pMat4 rotation = p_quat_to_mat4(animation_joint->rotation);
		pMat4 scale = p_scale(animation_joint->scale);
		pMat4 transform = p_mat4_mul(translation, p_mat4_mul(scale, rotation));
        final_bone_matrix[b] = p_mat4_mul(transform, model->bone_inverse_model_space_pose_matrix[b]);
    }
    PE_TRACE_FUNCTION_END();
    return final_bone_matrix;
}

void pe_model_draw(peModel *model, peArena *temp_arena, pVec3 position, pVec3 rotation) {
	PE_TRACE_FUNCTION_BEGIN();
    peArenaTemp temp_arena_memory = pe_arena_temp_begin(temp_arena);
	if (!last_frame_time_initialized) {
		last_frame_time = ptime_now();
		last_frame_time_initialized = true;
	}
	current_frame_time += (float)ptime_sec(ptime_laptime(&last_frame_time));
	if (current_frame_time >= frame_time) {
		frame_index = (frame_index + 1) % model->animation[0].num_frames;
		current_frame_time -= frame_time;
	}

	pe_model_draw_set_model_matrix(model, position, rotation);

    pe_graphics_set_lighting(true);
    pe_graphics_set_light_vector(PE_LIGHT_VECTOR_DEFAULT);

    peAnimationJoint *model_space_joints = pe_model_draw_get_model_space_animation_joints(model, temp_arena);
    pMat4 *final_bone_matrix = pe_model_draw_get_final_bone_matrix(model, temp_arena, model_space_joints);

    pe_model_draw_meshes(model, final_bone_matrix);

	pe_arena_temp_end(temp_arena_memory);
	PE_TRACE_FUNCTION_END();
}