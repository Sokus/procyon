#include "pe_model.h"

#include "core/pe_core.h"
#include "core/pe_file_io.h"
#include "platform/pe_platform.h"
#include "utility/pe_trace.h"
#include "platform/pe_time.h"

#include "pp3d.h"

#if defined(_WIN32)
	#include "pe_graphics_win32.h"

    #define COBJMACROS
    #include <d3d11.h>
    #include <dxgi1_2.h>

    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>

#elif defined(__linux__)
	#include "pe_graphics_linux.h"
#elif defined(PSP)
    #include "pe_graphics_psp.h"
    #include <pspkernel.h> // sceKernelDcacheWritebackInvalidateRange
    #include <pspgu.h>
    #include <pspgum.h>
#endif

#include "stb_image.h"

#include <string.h>

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

#if defined(_WIN32) || defined(__linux__)
void pe_model_alloc(peModel *model, peArena *arena, pp3dStaticInfo *p3d_static_info, pp3dAnimation *p3d_animation) {
    size_t num_index_size = p3d_static_info->num_meshes * sizeof(unsigned int);
    size_t index_offset_size = p3d_static_info->num_meshes * sizeof(unsigned int);
    size_t vertex_offset_size = p3d_static_info->num_meshes * sizeof(int);
    size_t material_size = p3d_static_info->num_meshes * sizeof(peMaterial);
    size_t bone_parent_index_size = p3d_static_info->num_bones * sizeof(int);
    size_t bone_inverse_model_space_pose_matrix_size = p3d_static_info->num_bones * sizeof(pMat4);
    size_t animation_size = p3d_static_info->num_animations * sizeof(peAnimation);

    model->num_index = pe_arena_alloc(arena, num_index_size);
    model->index_offset = pe_arena_alloc(arena, index_offset_size);
    model->vertex_offset = pe_arena_alloc(arena, vertex_offset_size);
    model->material = pe_arena_alloc(arena, material_size);
    model->bone_parent_index = pe_arena_alloc(arena, bone_parent_index_size);
    model->bone_inverse_model_space_pose_matrix = pe_arena_alloc(arena, bone_inverse_model_space_pose_matrix_size);

    model->animation = pe_arena_alloc(arena, animation_size);
    for (int a = 0; a < p3d_static_info->num_animations; a += 1) {
        unsigned int num_animation_joint = p3d_animation[a].num_frames * p3d_static_info->num_bones;
        size_t animation_joint_size = num_animation_joint * sizeof(peAnimationJoint);
        model->animation[a].frames = pe_arena_alloc(arena, animation_joint_size);
    }
}
#endif

#define VERT_MEM_ALIGN 16

#if defined(PSP)
void pe_model_alloc_psp(peModel *model, peArena *arena, pp3dStaticInfo *pp3d_static_info, pp3dMesh *pp3d_mesh, pp3dAnimation *pp3d_animation, size_t *pp3d_vertex_size, size_t *pp3d_index_size) {
	size_t material_size = pp3d_static_info->num_materials * sizeof(peMaterial);
	size_t subskeleton_size = pp3d_static_info->num_subskeletons * sizeof(peSubskeleton);
	size_t animation_size = pp3d_static_info->num_animations * sizeof(peAnimation);
	size_t bone_parent_index_size = pp3d_static_info->num_bones * sizeof(uint16_t);
	size_t bone_inverse_model_space_pose_matrix_size = pp3d_static_info->num_bones * sizeof(pMat4);
	size_t mesh_size = pp3d_static_info->num_meshes * sizeof(peMesh);
	size_t mesh_material_size = pp3d_static_info->num_meshes * sizeof(int);
	size_t mesh_subskeleton_size = pp3d_static_info->num_meshes * sizeof(int);

	model->material = pe_arena_alloc(arena, material_size);
	model->subskeleton = pe_arena_alloc(arena, subskeleton_size);

	model->animation = pe_arena_alloc(arena, animation_size);
	for (int a = 0; a < pp3d_static_info->num_animations; a += 1) {
		size_t num_animation_joint = pp3d_animation[a].num_frames * pp3d_static_info->num_bones;
		size_t animation_joint_size = num_animation_joint * sizeof(peAnimationJoint);
		model->animation[a].frames = pe_arena_alloc(arena, animation_joint_size);
	}

	model->bone_parent_index = pe_arena_alloc(arena, bone_parent_index_size);
	model->bone_inverse_model_space_pose_matrix = pe_arena_alloc(arena, bone_inverse_model_space_pose_matrix_size);

	model->mesh = pe_arena_alloc(arena, mesh_size);
	model->mesh_material = pe_arena_alloc(arena, mesh_material_size);
	model->mesh_subskeleton = pe_arena_alloc(arena, mesh_subskeleton_size);

	for (int m = 0; m < pp3d_static_info->num_meshes; m += 1) {
		model->mesh[m].vertex = pe_arena_alloc_align(arena, pp3d_vertex_size[m], VERT_MEM_ALIGN);

		if (pp3d_mesh[m].num_index > 0) {
			model->mesh[m].index = pe_arena_alloc_align(arena, pp3d_index_size[m], VERT_MEM_ALIGN);
		}
	}
}
#endif

static float pe_int16_to_float(int16_t value, float a, float b) {
    float float_negative_one_to_one;
    if (value >= 0) {
        float_negative_one_to_one = (float)value / (float)INT16_MAX;
    } else {
        float_negative_one_to_one = -1.0f * (float)value / (float)INT16_MIN;
    }
    PE_ASSERT(b > a);
    float float_a_to_b = (float_negative_one_to_one + 1.0f) * (b - a) / 2.0f + a;
    return float_a_to_b;
}

static float pe_uint16_to_float(uint16_t value, float a, float b) {
    float float_zero_to_one = (float)value / (float)UINT16_MAX;
    float float_a_to_b = float_zero_to_one * (b - a) + a;
    return float_a_to_b;
}

#if defined(PSP)

bool pe_parse_pp3d(peArena *arena, peFileContents pp3d_file_contents, pp3dFile *pp3d) {
	uint8_t *pp3d_file_pointer = pp3d_file_contents.data;
	size_t pp3d_file_contents_left = pp3d_file_contents.size;

	if (sizeof(pp3dStaticInfo) > pp3d_file_contents_left) return false;
	pp3d->static_info = (pp3dStaticInfo *)pp3d_file_pointer;
	pp3d_file_pointer += sizeof(pp3dStaticInfo);
	pp3d_file_contents_left -= sizeof(pp3dStaticInfo);
	{
		int8_t bone_weight_size = pp3d->static_info->bone_weight_size;
		if (!(bone_weight_size == 1 || bone_weight_size == 2 || bone_weight_size == 4)) {
			return false;
		}
        if (pp3d->static_info->is_portable == 0) {
            return false;
		}
	}

	size_t mesh_size = pp3d->static_info->num_meshes * sizeof(pp3dMesh);
	if (mesh_size > pp3d_file_contents_left) return false;
	pp3d->mesh = (pp3dMesh *)pp3d_file_pointer;
	pp3d_file_pointer += mesh_size;
	pp3d_file_contents_left -= mesh_size;

	size_t material_size = pp3d->static_info->num_materials * sizeof(pp3dMaterial);
	if (material_size > pp3d_file_contents_left) return false;
	pp3d->material = (pp3dMaterial *)pp3d_file_pointer;
	pp3d_file_pointer += material_size;
	pp3d_file_contents_left -= material_size;

	size_t subskeleton_size = pp3d->static_info->num_subskeletons * sizeof(pp3dSubskeleton);
	if (subskeleton_size > pp3d_file_contents_left) return false;
	pp3d->subskeleton = (pp3dSubskeleton *)pp3d_file_pointer;
	pp3d_file_pointer += subskeleton_size;
	pp3d_file_contents_left -= subskeleton_size;

	size_t animation_size = pp3d->static_info->num_animations * sizeof(pp3dAnimation);
	if (animation_size > pp3d_file_contents_left) return false;
	pp3d->animation = (pp3dAnimation *)pp3d_file_pointer;
	pp3d_file_pointer += animation_size;
	pp3d_file_contents_left -= animation_size;

	pp3d->portable.vertex = (void**)pe_arena_alloc(arena, pp3d->static_info->num_meshes * sizeof(void*));
	pp3d->portable.vertex_size = (size_t *)pe_arena_alloc(arena, pp3d->static_info->num_meshes * sizeof(size_t));
	pp3d->portable.index = (void**)pe_arena_alloc(arena, pp3d->static_info->num_meshes * sizeof(void*));
	pp3d->portable.index_size = pe_arena_alloc(arena, pp3d->static_info->num_meshes * sizeof(size_t));
	for (int m = 0; m < pp3d->static_info->num_meshes; m += 1) {
		uint8_t num_weights = pp3d->subskeleton[pp3d->mesh[m].subskeleton_index].num_bones;
		bool has_diffuse_map = (
			pp3d->material[pp3d->mesh[m].material_index].diffuse_image_file_name[0] != '\0'
		);
		size_t single_vertex_size = (
			num_weights * (size_t)(pp3d->static_info->bone_weight_size) + // weights
			(num_weights % 2) * sizeof(uint8_t) + // weight padding
			(has_diffuse_map ? 2 * sizeof(int16_t) : 0) + // uv
			3 * sizeof(int16_t) +         // normal
			3 * sizeof(int16_t)	          // position
		);

		size_t vertex_size = single_vertex_size * pp3d->mesh[m].num_vertex;
		if (vertex_size > pp3d_file_contents_left) return false;
		pp3d->portable.vertex[m] = pp3d_file_pointer;
		pp3d->portable.vertex_size[m] = vertex_size;
		pp3d_file_pointer += vertex_size;
		pp3d_file_contents_left -= vertex_size;

		if (pp3d->mesh[m].num_index > 0) {
			size_t index_size = pp3d->mesh[m].num_index * sizeof(uint16_t);
			if (index_size > pp3d_file_contents_left) return false;
			pp3d->portable.index[m] = pp3d_file_pointer;
			pp3d->portable.index_size[m] = index_size;
			pp3d_file_pointer += index_size;
			pp3d_file_contents_left -= index_size;
		} else {
			pp3d->portable.index[m] = NULL;
			pp3d->portable.index_size[m] = 0;
		}
	}

	size_t bone_parent_index_size = pp3d->static_info->num_bones * sizeof(uint16_t);
	if (bone_parent_index_size > pp3d_file_contents_left) return false;
	pp3d->bone_parent_index = (uint16_t *)pp3d_file_pointer;
	pp3d_file_pointer += bone_parent_index_size;
	pp3d_file_contents_left -= bone_parent_index_size;

	size_t inverse_model_space_pose_matrix_size = pp3d->static_info->num_bones * sizeof(pp3dMatrix);
	if (inverse_model_space_pose_matrix_size > pp3d_file_contents_left) return false;
	pp3d->inverse_model_space_pose_matrix = (pp3dMatrix *)pp3d_file_pointer;
	pp3d_file_pointer += inverse_model_space_pose_matrix_size;
	pp3d_file_contents_left -= inverse_model_space_pose_matrix_size;

	size_t animation_joint_size = (size_t)pp3d->static_info->num_frames_total * (size_t)pp3d->static_info->num_bones * sizeof(pp3dAnimationJoint);
	if (animation_joint_size > pp3d_file_contents_left) return false;
	pp3d->animation_joint = (pp3dAnimationJoint *)pp3d_file_pointer;
	pp3d_file_pointer += animation_joint_size;
	pp3d_file_contents_left -= animation_joint_size;

	return true;
}

static peModel pe_model_load_psp(peArena *temp_arena, const char *file_path) {
    peArenaTemp temp_arena_memory = pe_arena_temp_begin(temp_arena);

	pp3dFile pp3d;
	peFileContents pp3d_file_contents = pe_file_read_contents(temp_arena, file_path, false);
	pe_parse_pp3d(temp_arena, pp3d_file_contents, &pp3d);

    peModel model = {0};
	peArena model_arena;
    {
		// TODO: this is basically a memory leak (xD)
		// so far we are only loading a single model to test things out
		// so we don't really care, but we should allocate models in
		// another way in the future (pool allocator?)
		size_t model_memory_size = PE_KILOBYTES(512);
		void *model_memory = pe_heap_alloc_align(PE_KILOBYTES(512), VERT_MEM_ALIGN);
		pe_arena_init(&model_arena, model_memory, model_memory_size);
        pe_model_alloc_psp(&model, &model_arena, pp3d.static_info, pp3d.mesh, pp3d.animation, pp3d.portable.vertex_size, pp3d.portable.index_size);
    }

	// TODO: 8 and 16 bit UVs depending on texture size
	// TODO: 8 and 16 bit indices depending on vertex count

	model.scale = pp3d.static_info->scale;
	model.num_mesh = pp3d.static_info->num_meshes;
	model.num_material = pp3d.static_info->num_materials;
	model.num_subskeleton = pp3d.static_info->num_subskeletons;
	model.num_animations = pp3d.static_info->num_animations;
	model.num_bone = pp3d.static_info->num_bones;

    stbi_set_flip_vertically_on_load(false);
	for (int m = 0; m < pp3d.static_info->num_materials; m += 1) {
		model.material[m] = pe_default_material();
		model.material[m].diffuse_color.rgba = pp3d.material[m].diffuse_color;
		if (pp3d.material[m].diffuse_image_file_name[0] != '\0') {
			model.material[m].has_diffuse_map = true;

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
			size_t diffuse_map_file_length = strnlen(pp3d.material[m].diffuse_image_file_name, sizeof(pp3d.material[m].diffuse_image_file_name));
			memcpy(diffuse_texture_path + last_slash_index + 1, pp3d.material[m].diffuse_image_file_name, diffuse_map_file_length*sizeof(char));

			int w, h, channels;
			stbi_uc *stbi_data = stbi_load(diffuse_texture_path, &w, &h, &channels, STBI_rgb_alpha);
			model.material[m].diffuse_map = pe_texture_create(temp_arena, stbi_data, w, h, GU_PSM_8888);
			stbi_image_free(stbi_data);
		}
	}

	for (int s = 0; s < pp3d.static_info->num_subskeletons; s += 1) {
		model.subskeleton[s].num_bones = pp3d.subskeleton[s].num_bones;
		for (int b = 0; b < pp3d.subskeleton[s].num_bones; b += 1) {
			model.subskeleton[s].bone_indices[b] = pp3d.subskeleton[s].bone_indices[b];
		}
	}

	for (int a = 0; a < pp3d.static_info->num_animations; a += 1) {
		PE_ASSERT(sizeof(model.animation[a].name) == sizeof(pp3d.animation[a].name));
		memcpy(model.animation[a].name, pp3d.animation[a].name, sizeof(model.animation[a].name));

		model.animation[a].num_frames = pp3d.animation[a].num_frames;
	}

	for (int m = 0; m < pp3d.static_info->num_meshes; m += 1) {
		model.mesh_material[m] = pp3d.mesh[m].material_index;
		model.mesh_subskeleton[m] = pp3d.mesh[m].subskeleton_index;
		model.mesh[m].num_vertex = pp3d.mesh[m].num_vertex;
		model.mesh[m].num_index = pp3d.mesh[m].num_index;

		model.mesh[m].vertex_type = GU_NORMAL_16BIT|GU_VERTEX_16BIT|GU_TRANSFORM_3D;
		if (pp3d.material[pp3d.mesh[m].material_index].diffuse_image_file_name[0] != '\0') {
			model.mesh[m].vertex_type |= GU_TEXTURE_16BIT;
		}
		uint8_t num_weights = pp3d.subskeleton[pp3d.mesh[m].subskeleton_index].num_bones;
		if (num_weights > 0) {
			model.mesh[m].vertex_type |= GU_WEIGHTS(num_weights);
			switch (pp3d.static_info->bone_weight_size) {
				case 1: model.mesh[m].vertex_type |= GU_WEIGHT_8BIT; break;
				case 2: model.mesh[m].vertex_type |= GU_WEIGHT_16BIT; break;
				case 4: model.mesh[m].vertex_type |= GU_WEIGHT_32BITF; break;
				default: PE_PANIC(); break;
			}
		}

		memcpy(model.mesh[m].vertex, pp3d.portable.vertex[m], pp3d.portable.vertex_size[m]);

		if (pp3d.mesh[m].num_index > 0) {
			model.mesh[m].vertex_type |= GU_INDEX_16BIT;
			memcpy(model.mesh[m].index, pp3d.portable.index[m], pp3d.portable.index_size[m]);
		}
	}

	memcpy(model.bone_parent_index, pp3d.bone_parent_index, pp3d.static_info->num_bones * sizeof(uint16_t));

	memcpy(model.bone_inverse_model_space_pose_matrix, pp3d.inverse_model_space_pose_matrix, pp3d.static_info->num_bones * sizeof(pMat4));

	int pp3d_animation_joint_offset = 0;
	for (int a = 0; a < pp3d.static_info->num_animations; a += 1) {
		size_t num_animation_joints = pp3d.static_info->num_bones * pp3d.animation[a].num_frames;
		size_t animation_joints_size = num_animation_joints * sizeof(peAnimationJoint);
		memcpy(model.animation[a].frames, pp3d.animation_joint + pp3d_animation_joint_offset, animation_joints_size);
		pp3d_animation_joint_offset += animation_joints_size;
	}

	sceKernelDcacheWritebackInvalidateRange(model_arena.physical_start, model_arena.total_allocated);

	pe_arena_temp_end(temp_arena_memory);

	return model;
}
#endif // #if defined(PSP)

bool pe_parse_p3d(peFileContents p3d_file_contents, pp3dFile *p3d) {
	uint8_t *p3d_file_pointer = p3d_file_contents.data;
	size_t p3d_file_contents_left = p3d_file_contents.size;

	if (p3d_file_contents_left < sizeof(pp3dStaticInfo)) return false;
	p3d->static_info = (pp3dStaticInfo*)p3d_file_pointer;
	p3d_file_pointer += sizeof(pp3dStaticInfo);
	p3d_file_contents_left -= sizeof(pp3dStaticInfo);

	size_t mesh_size = p3d->static_info->num_meshes * sizeof(pp3dMesh);
	if (p3d_file_contents_left < mesh_size) return false;
	p3d->mesh = (pp3dMesh*)p3d_file_pointer;
	p3d_file_pointer += mesh_size;
	p3d_file_contents_left -= mesh_size;

    size_t material_size = p3d->static_info->num_materials * sizeof(pp3dMaterial);
    if (p3d_file_contents_left < material_size) return false;
    p3d->material = (pp3dMaterial*)p3d_file_pointer;
    p3d_file_pointer += material_size;
    p3d_file_contents_left -= material_size;

	size_t animation_size = p3d->static_info->num_animations * sizeof(pp3dAnimation);
	if (p3d_file_contents_left < animation_size) return false;
	p3d->animation = (pp3dAnimation*)p3d_file_pointer;
	p3d_file_pointer += animation_size;
	p3d_file_contents_left -= animation_size;

	size_t position_size = 3 * p3d->static_info->num_vertex * sizeof(int16_t);
	if (p3d_file_contents_left < position_size) return false;
	p3d->desktop.position = (int16_t*)p3d_file_pointer;
	p3d_file_pointer += position_size;
	p3d_file_contents_left -= position_size;

	size_t normal_size = 3 * p3d->static_info->num_vertex * sizeof(int16_t);
	if (p3d_file_contents_left < normal_size) return false;
	p3d->desktop.normal = (int16_t*)p3d_file_pointer;
	p3d_file_pointer += normal_size;
	p3d_file_contents_left -= normal_size;

	size_t texcoord_size = 2 * p3d->static_info->num_vertex * sizeof(int16_t);
	if (p3d_file_contents_left < texcoord_size) return false;
	p3d->desktop.texcoord = (int16_t*)p3d_file_pointer;
	p3d_file_pointer += texcoord_size;
	p3d_file_contents_left -= texcoord_size;

	size_t bone_index_size = 4 * p3d->static_info->num_vertex * sizeof(uint8_t);
	if (p3d_file_contents_left < bone_index_size) return false;
	p3d->desktop.bone_index = (uint8_t *)p3d_file_pointer;
	p3d_file_pointer += bone_index_size;
	p3d_file_contents_left -= bone_index_size;

	size_t bone_weight_size = 4 * p3d->static_info->num_vertex * sizeof(uint16_t);
	if (p3d_file_contents_left < bone_weight_size) return false;
	p3d->desktop.bone_weight = (uint16_t *)p3d_file_pointer;
	p3d_file_pointer += bone_weight_size;
	p3d_file_contents_left -= bone_weight_size;

	size_t index_size = p3d->static_info->num_index * sizeof(uint32_t);
	if (p3d_file_contents_left < index_size) return false;
	p3d->desktop.index = (uint32_t *)p3d_file_pointer;
	p3d_file_pointer += index_size;
	p3d_file_contents_left -= index_size;

	size_t bone_parent_index_size = p3d->static_info->num_bones * sizeof(uint16_t);
	if (p3d_file_contents_left < bone_parent_index_size) return false;
	p3d->bone_parent_index = (uint16_t *)p3d_file_pointer;
	p3d_file_pointer += bone_parent_index_size;
	p3d_file_contents_left -= bone_parent_index_size;

	size_t inverse_model_space_pose_matrix_size = p3d->static_info->num_bones * sizeof(pp3dMatrix);
	if (p3d_file_contents_left < inverse_model_space_pose_matrix_size) return false;
	p3d->inverse_model_space_pose_matrix = (pp3dMatrix *)p3d_file_pointer;
	p3d_file_pointer += inverse_model_space_pose_matrix_size;
	p3d_file_contents_left -= inverse_model_space_pose_matrix_size;

	size_t animation_joint_size = (size_t)p3d->static_info->num_frames_total * (size_t)p3d->static_info->num_bones * sizeof(pp3dAnimationJoint);
	if (p3d_file_contents_left < animation_joint_size) return false;
	p3d->animation_joint = (pp3dAnimationJoint *)p3d_file_pointer;
	p3d_file_pointer += animation_joint_size;
	p3d_file_contents_left -= animation_joint_size;

	return true;
}

peModel pe_model_load(peArena *temp_arena, char *file_path) {
	PE_TRACE_FUNCTION_BEGIN();
#if defined(_WIN32) || defined(__linux__)
    peArenaTemp temp_arena_memory = pe_arena_temp_begin(temp_arena);

	pp3dFile p3d;
    peFileContents p3d_file_contents = pe_file_read_contents(temp_arena, file_path, false);
	pe_parse_p3d(p3d_file_contents, &p3d);

    peModel model = {0};
    {
		// TODO: this is basically a memory leak (xD)
		// so far we are only loading a single model to test things out
		// so we don't really care, but we should allocate models in
		// another way in the future (pool allocator?)
		peArena model_arena;
		size_t model_memory_size = PE_KILOBYTES(512);
		pe_arena_init(&model_arena, pe_heap_alloc(model_memory_size), model_memory_size);
        pe_model_alloc(&model, &model_arena, p3d.static_info, p3d.animation);
    }

    model.num_mesh = (int)p3d.static_info->num_meshes;
    // for P3D number of meshes is always equal to the number of materials
    // (at least it should be... lol)
    model.num_material = (int)p3d.static_info->num_materials;
    model.num_bone = (int)p3d.static_info->num_bones;
    model.num_animations = (int)p3d.static_info->num_animations;
    model.num_vertex = p3d.static_info->num_vertex;

    stbi_set_flip_vertically_on_load(false);
    for (int m = 0; m < p3d.static_info->num_meshes; m += 1) {
        model.num_index[m] = p3d.mesh[m].num_index;
        model.index_offset[m] = p3d.mesh[m].index_offset;
        PE_ASSERT(p3d.mesh[m].vertex_offset <= INT32_MAX);
        model.vertex_offset[m] = p3d.mesh[m].vertex_offset;
    }

    for (int m = 0; m < p3d.static_info->num_materials; m += 1) {
        model.material[m] = pe_default_material();
        model.material[m].diffuse_color.rgba = p3d.material[m].diffuse_color;
        if (p3d.material[m].diffuse_image_file_name[0] != '\0') {
			model.material[m].has_diffuse_map = true;

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
			size_t diffuse_image_file_length = strnlen(p3d.material[m].diffuse_image_file_name, sizeof(p3d.material[m].diffuse_image_file_name));
			memcpy(diffuse_texture_path + last_slash_index + 1, p3d.material[m].diffuse_image_file_name, diffuse_image_file_length*sizeof(char));

			int w, h, channels;
			stbi_uc *stbi_data = stbi_load(diffuse_texture_path, &w, &h, &channels, STBI_rgb_alpha);
			model.material[m].diffuse_map = pe_texture_create(stbi_data, w, h, channels);
			stbi_image_free(stbi_data);
		}
    }

    for (int a = 0; a < p3d.static_info->num_animations; a += 1) {
        PE_ASSERT(sizeof(model.animation[a].name) == sizeof(p3d.animation[a].name));
        memcpy(model.animation[a].name, p3d.animation[a].name, sizeof(model.animation[a].name));
        model.animation[a].num_frames = p3d.animation[a].num_frames;
    }

	size_t pos_buffer_size = 3*p3d.static_info->num_vertex*sizeof(float);
	size_t nor_buffer_size = 3*p3d.static_info->num_vertex*sizeof(float);
	size_t tex_buffer_size = 2*p3d.static_info->num_vertex*sizeof(float);
	size_t col_buffer_size = 1*p3d.static_info->num_vertex*sizeof(uint32_t);
	size_t bone_index_buffer_size = 4*p3d.static_info->num_vertex*sizeof(uint32_t);
	size_t bone_weight_buffer_size = 4*p3d.static_info->num_vertex*sizeof(float);

    float *pos_buffer = pe_arena_alloc(temp_arena, pos_buffer_size);
    float *nor_buffer = pe_arena_alloc(temp_arena, nor_buffer_size);
    float *tex_buffer = pe_arena_alloc(temp_arena, tex_buffer_size);
    uint32_t *col_buffer = pe_arena_alloc(temp_arena, col_buffer_size);
    uint32_t *bone_index_buffer = pe_arena_alloc(temp_arena, bone_index_buffer_size);
    float *bone_weight_buffer = pe_arena_alloc(temp_arena, bone_weight_buffer_size);

    for (unsigned int p = 0; p < 3*p3d.static_info->num_vertex; p += 1) {
        pos_buffer[p] = p3d.static_info->scale * pe_int16_to_float(p3d.desktop.position[p], -1.0f, 1.0f);
    }

    for (unsigned int n = 0; n < 3*p3d.static_info->num_vertex; n += 1) {
        nor_buffer[n] = pe_int16_to_float(p3d.desktop.normal[n], -1.0f, 1.0f);
    }

    for (unsigned int t = 0; t < 2*p3d.static_info->num_vertex; t += 1) {
        tex_buffer[t] = pe_int16_to_float(p3d.desktop.texcoord[t], -1.0f, 1.0f);
    }

    for (unsigned int c = 0; c < p3d.static_info->num_vertex; c += 1) {
        col_buffer[c] = PE_COLOR_WHITE.rgba;
    }

    for (unsigned int b = 0; b < 4*p3d.static_info->num_vertex; b += 1) {
        bone_index_buffer[b] = (uint32_t)p3d.desktop.bone_index[b];
    }

    for (unsigned int b = 0; b < 4*p3d.static_info->num_vertex; b += 1) {
        bone_weight_buffer[b] = pe_uint16_to_float(p3d.desktop.bone_weight[b], 0.0f, 1.0f);
    }

    for (unsigned int b = 0; b < p3d.static_info->num_bones; b += 1) {
        model.bone_parent_index[b] = (uint8_t)p3d.bone_parent_index[b]; // WARNING: casting from uint16_t to uint8_t
    }

    memcpy(model.bone_inverse_model_space_pose_matrix, p3d.inverse_model_space_pose_matrix, p3d.static_info->num_bones*sizeof(pp3dMatrix));

    int p3d_animation_joint_offset = 0;
    for (int a = 0; a < p3d.static_info->num_animations; a += 1) {
        for (int f = 0; f < model.animation[a].num_frames; f += 1) {
            for (int j = 0; j < p3d.static_info->num_bones; j += 1) {
                peAnimationJoint *pe_animation_joint = &model.animation[a].frames[f * p3d.static_info->num_bones + j];
                pp3dAnimationJoint *p3d_animation_joint_ptr = p3d.animation_joint + p3d_animation_joint_offset;
                memcpy(pe_animation_joint->translation.elements, &p3d_animation_joint_ptr->position, 3*sizeof(float));
                memcpy(pe_animation_joint->rotation.elements, &p3d_animation_joint_ptr->rotation, 4*sizeof(float));
                memcpy(pe_animation_joint->scale.elements, &p3d_animation_joint_ptr->scale, 3*sizeof(float));

                p3d_animation_joint_offset += 1;
            }
        }
    }

	size_t p3d_index_size = p3d.static_info->num_index * sizeof(*p3d.desktop.index);
#if defined(_WIN32)
    model.pos_buffer = pe_d3d11_create_buffer(pos_buffer, (UINT)pos_buffer_size, D3D11_USAGE_IMMUTABLE, D3D11_BIND_VERTEX_BUFFER, 0);
    model.norm_buffer = pe_d3d11_create_buffer(nor_buffer, (UINT)nor_buffer_size, D3D11_USAGE_IMMUTABLE, D3D11_BIND_VERTEX_BUFFER, 0);
    model.tex_buffer = pe_d3d11_create_buffer(tex_buffer, (UINT)tex_buffer_size, D3D11_USAGE_IMMUTABLE, D3D11_BIND_VERTEX_BUFFER, 0);
    model.color_buffer = pe_d3d11_create_buffer(col_buffer, (UINT)col_buffer_size, D3D11_USAGE_IMMUTABLE, D3D11_BIND_VERTEX_BUFFER, 0);
    model.bone_index_buffer = pe_d3d11_create_buffer(bone_index_buffer, (UINT)bone_index_buffer_size, D3D11_USAGE_IMMUTABLE, D3D11_BIND_VERTEX_BUFFER, 0);
    model.bone_weight_buffer = pe_d3d11_create_buffer(bone_weight_buffer, (UINT)bone_weight_buffer_size, D3D11_USAGE_IMMUTABLE, D3D11_BIND_VERTEX_BUFFER, 0);
    model.index_buffer = pe_d3d11_create_buffer(p3d.desktop.index, (UINT)p3d_index_size, D3D11_USAGE_IMMUTABLE, D3D11_BIND_INDEX_BUFFER, 0);
#endif
#if defined(__linux__)
	size_t total_size = pos_buffer_size + nor_buffer_size + tex_buffer_size + col_buffer_size + bone_index_buffer_size + bone_weight_buffer_size;

	glGenVertexArrays(1, &model.vertex_array_object);
    glBindVertexArray(model.vertex_array_object);

    glGenBuffers(1, &model.vertex_buffer_object);
    glBindBuffer(GL_ARRAY_BUFFER, model.vertex_buffer_object);
    glBufferData(GL_ARRAY_BUFFER, total_size, NULL, GL_STATIC_DRAW);

    glBufferSubData(GL_ARRAY_BUFFER, 0,                                                                                      pos_buffer_size, pos_buffer);
    glBufferSubData(GL_ARRAY_BUFFER, pos_buffer_size,                                                                        nor_buffer_size, nor_buffer);
	glBufferSubData(GL_ARRAY_BUFFER, pos_buffer_size+nor_buffer_size,                                                        tex_buffer_size, tex_buffer);
	glBufferSubData(GL_ARRAY_BUFFER, pos_buffer_size+nor_buffer_size+tex_buffer_size,                                        col_buffer_size, col_buffer);
	glBufferSubData(GL_ARRAY_BUFFER, pos_buffer_size+nor_buffer_size+tex_buffer_size+col_buffer_size,                        bone_index_buffer_size, bone_index_buffer);
	glBufferSubData(GL_ARRAY_BUFFER, pos_buffer_size+nor_buffer_size+tex_buffer_size+col_buffer_size+bone_index_buffer_size, bone_weight_buffer_size, bone_weight_buffer);

    glVertexAttribPointer( 0, 3, GL_FLOAT,        GL_FALSE, 0,    (void*)0);
    glVertexAttribPointer( 1, 3, GL_FLOAT,        GL_FALSE, 0,    (void*)(pos_buffer_size));
	glVertexAttribPointer( 2, 2, GL_FLOAT,        GL_FALSE, 0,    (void*)(pos_buffer_size+nor_buffer_size));
	glVertexAttribPointer( 3, 4, GL_UNSIGNED_BYTE,         GL_TRUE,  0,    (void*)(pos_buffer_size+nor_buffer_size+tex_buffer_size));
	glVertexAttribIPointer(4, 4, GL_UNSIGNED_INT,           0,    (void*)(pos_buffer_size+nor_buffer_size+tex_buffer_size+col_buffer_size));
	glVertexAttribPointer( 5, 4, GL_FLOAT,        GL_FALSE, 0,    (void*)(pos_buffer_size+nor_buffer_size+tex_buffer_size+col_buffer_size+bone_index_buffer_size));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glEnableVertexAttribArray(3);
	glEnableVertexAttribArray(4);
	glEnableVertexAttribArray(5);

    glGenBuffers(1, &model.element_buffer_object);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model.element_buffer_object);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, p3d_index_size, p3d.desktop.index, GL_STATIC_DRAW);

    glBindVertexArray(0);
#endif
    pe_arena_temp_end(temp_arena_memory);
#elif defined(PSP)
	peModel model = pe_model_load_psp(temp_arena, file_path);
#endif
	PE_TRACE_FUNCTION_END();
	return model;
}

float frame_time = 1.0f/25.0f;
float current_frame_time = 0.0f;
int frame_index = 0;
uint64_t last_frame_time;
bool last_frame_time_initialized = false;

void pe_model_draw(peModel *model, peArena *temp_arena, pVec3 position, pVec3 rotation) {
	PE_TRACE_FUNCTION_BEGIN();
    peArenaTemp temp_arena_memory = pe_arena_temp_begin(temp_arena);
	if (!last_frame_time_initialized) {
		last_frame_time = pe_time_now();
		last_frame_time_initialized = true;
	}
	current_frame_time += (float)pe_time_sec(pe_time_laptime(&last_frame_time));
	if (current_frame_time >= frame_time) {
		frame_index = (frame_index + 1) % model->animation[0].num_frames;
		current_frame_time -= frame_time;
	}

#if defined(_WIN32)
    pMat4 rotate_x = p_rotate_rh(rotation.x, (pVec3){1.0f, 0.0f, 0.0f});
    pMat4 rotate_y = p_rotate_rh(rotation.y, (pVec3){0.0f, 1.0f, 0.0f});
    pMat4 rotate_z = p_rotate_rh(rotation.z, (pVec3){0.0f, 0.0f, 1.0f});
    pMat4 translate = p_translate(position);

    pMat4 model_matrix = p_mat4_mul(p_mat4_mul(p_mat4_mul(translate, rotate_z), rotate_y), rotate_x);
    peShaderConstant_Matrix *constant_model = pe_shader_constant_begin_map(pe_d3d.context, constant_buffers_d3d.model);
    constant_model->value = model_matrix;
    pe_shader_constant_end_map(pe_d3d.context, constant_buffers_d3d.model);

    peShaderConstant_Light *constant_light = pe_shader_constant_begin_map(pe_d3d.context, constant_buffers_d3d.light);
    constant_light->do_lighting = true;
    constant_light->light_vector = PE_LIGHT_VECTOR_DEFAULT;
    pe_shader_constant_end_map(pe_d3d.context, constant_buffers_d3d.light);

    ID3D11Buffer *buffs[] = {
        model->pos_buffer,
        model->norm_buffer,
        model->tex_buffer,
        model->color_buffer,
        model->bone_index_buffer,
        model->bone_weight_buffer,
    };
    uint32_t strides[] = {
        3*sizeof(float),
        3*sizeof(float),
        2*sizeof(float),
        sizeof(uint32_t),
        4*sizeof(uint32_t),
        4*sizeof(float),
    };
    uint32_t offsets[6] = {0, 0, 0, 0, 0, 0};

    ID3D11DeviceContext_IASetPrimitiveTopology(pe_d3d.context, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ID3D11DeviceContext_IASetVertexBuffers(pe_d3d.context, 0, 6, buffs, strides, offsets);
    ID3D11DeviceContext_IASetIndexBuffer(pe_d3d.context, model->index_buffer, DXGI_FORMAT_R32_UINT, 0);

    {
        peAnimationJoint *model_space_joints = pe_arena_alloc(temp_arena, model->num_bone * sizeof(peAnimationJoint));
        peAnimationJoint *animation_joints = &model->animation[0].frames[frame_index * model->num_bone];
        for (int b = 0; b < model->num_bone; b += 1) {
            if (model->bone_parent_index[b] < UINT8_MAX) {
                peAnimationJoint parent_transform = model_space_joints[model->bone_parent_index[b]];
                model_space_joints[b] = pe_concatenate_animation_joints(parent_transform, animation_joints[b]);
            } else {
                model_space_joints[b] = animation_joints[b];
            }
        }

        peShaderConstant_Skeleton *constant_skeleton = pe_shader_constant_begin_map(pe_d3d.context, constant_buffers_d3d.skeleton);
        constant_skeleton->has_skeleton = true;
        for (int b = 0; b < model->num_bone; b += 1) {
            peAnimationJoint *animation_joint = &model_space_joints[b];
			pMat4 translation = p_translate(animation_joint->translation);
			pMat4 rotation = p_quat_to_mat4(animation_joint->rotation);
			pMat4 scale = p_scale(animation_joint->scale);
			pMat4 transform = p_mat4_mul(translation, p_mat4_mul(scale, rotation));

            pMat4 final_bone_matrix = p_mat4_mul(transform, model->bone_inverse_model_space_pose_matrix[b]);
            constant_skeleton->matrix_bone[b] = final_bone_matrix;
        }
        pe_shader_constant_end_map(pe_d3d.context, constant_buffers_d3d.skeleton);
    }

    for (int m = 0; m < model->num_mesh; m += 1) {
        peShaderConstant_Material *constant_material = pe_shader_constant_begin_map(pe_d3d.context, constant_buffers_d3d.material);
        constant_material->diffuse_color = pe_color_to_vec4(model->material[m].diffuse_color);
        pe_shader_constant_end_map(pe_d3d.context, constant_buffers_d3d.material);

		if (model->material[m].has_diffuse_map) {
			pe_texture_bind(model->material[m].diffuse_map);
		} else {
            pe_texture_bind_default();
		}

        ID3D11DeviceContext_DrawIndexed(pe_d3d.context, model->num_index[m], model->index_offset[m], model->vertex_offset[m]);
    }
#endif
#if defined(__linux__)
    pMat4 rotate_x = p_rotate_rh(rotation.x, (pVec3){1.0f, 0.0f, 0.0f});
    pMat4 rotate_y = p_rotate_rh(rotation.y, (pVec3){0.0f, 1.0f, 0.0f});
    pMat4 rotate_z = p_rotate_rh(rotation.z, (pVec3){0.0f, 0.0f, 1.0f});
    pMat4 translate = p_translate(position);
    pMat4 model_matrix = p_mat4_mul(p_mat4_mul(p_mat4_mul(translate, rotate_z), rotate_y), rotate_x);

    pMat4 old_model_matrix;
    pe_shader_get_mat4(pe_opengl.shader_program, "matrix_model", &old_model_matrix);
	pe_shader_set_mat4(pe_opengl.shader_program, "matrix_model", &model_matrix);

    {
        peAnimationJoint *model_space_joints = pe_arena_alloc(temp_arena, model->num_bone * sizeof(peAnimationJoint));
        peAnimationJoint *animation_joints = &model->animation[0].frames[frame_index * model->num_bone];
        for (int b = 0; b < model->num_bone; b += 1) {
            if (model->bone_parent_index[b] < UINT8_MAX) {
                peAnimationJoint parent_transform = model_space_joints[model->bone_parent_index[b]];
                model_space_joints[b] = pe_concatenate_animation_joints(parent_transform, animation_joints[b]);
            } else {
                model_space_joints[b] = animation_joints[b];
            }
        }


		pMat4 *final_bone_matrix = pe_arena_alloc(temp_arena, model->num_bone * sizeof(pMat4));
        for (int b = 0; b < model->num_bone; b += 1) {
            peAnimationJoint *animation_joint = &model_space_joints[b];
			pMat4 translation = p_translate(animation_joint->translation);
			pMat4 rotation = p_quat_to_mat4(animation_joint->rotation);
			pMat4 scale = p_scale(animation_joint->scale);
			pMat4 transform = p_mat4_mul(translation, p_mat4_mul(scale, rotation));
            final_bone_matrix[b] = p_mat4_mul(transform, model->bone_inverse_model_space_pose_matrix[b]);
        }

		pe_shader_set_bool(pe_opengl.shader_program, "has_skeleton", true);
		pe_shader_set_mat4_array(pe_opengl.shader_program, "matrix_bone", final_bone_matrix, model->num_bone);
    }

	glBindVertexArray(model->vertex_array_object);
    for (int m = 0; m < model->num_mesh; m += 1) {
        pe_shader_set_vec3(pe_opengl.shader_program, "diffuse_color", pe_color_to_vec4(model->material[m].diffuse_color).rgb);

		if (model->material[m].has_diffuse_map) {
			pe_texture_bind(model->material[m].diffuse_map);
		} else {
            pe_texture_bind_default();
		}

		glDrawElementsBaseVertex(
			GL_TRIANGLES, model->num_index[m], GL_UNSIGNED_INT,
			(void*)(model->index_offset[m]*sizeof(uint32_t)), model->vertex_offset[m]
		);
	}

	pe_shader_set_mat4(pe_opengl.shader_program, "matrix_model", &old_model_matrix);

	glBindVertexArray(0);
#endif
#if defined(PSP)
	peTraceMark tm_sce_gum = PE_TRACE_MARK_BEGIN("SCE GUM");
	sceGumMatrixMode(GU_MODEL);
	sceGumPushMatrix();
	sceGumLoadIdentity();
	sceGumTranslate((ScePspFVector3 *)&position);
	ScePspFVector3 scale_vector = { model->scale, model->scale, model->scale };
	sceGumRotateXYZ((ScePspFVector3 *)&rotation);
	sceGumScale(&scale_vector);
	PE_TRACE_MARK_END(tm_sce_gum);

	peTraceMark tm_concat_anim_joints = PE_TRACE_MARK_BEGIN("concatenate animation joints");
	peAnimationJoint *model_space_joints = pe_arena_alloc(temp_arena, model->num_bone * sizeof(peAnimationJoint));
	peAnimationJoint *animation_joints = &model->animation[0].frames[frame_index * model->num_bone];
	for (int b = 0; b < model->num_bone; b += 1) {
		if (model->bone_parent_index[b] < UINT16_MAX) {
			peAnimationJoint parent_transform = model_space_joints[model->bone_parent_index[b]];
			model_space_joints[b] = pe_concatenate_animation_joints(parent_transform, animation_joints[b]);
		} else {
			model_space_joints[b] = animation_joints[b];
		}
	}
	PE_TRACE_MARK_END(tm_concat_anim_joints);

	peTraceMark tm_calc_matrices = PE_TRACE_MARK_BEGIN("calculate matrices");
	pMat4 *final_bone_matrix = pe_arena_alloc(temp_arena, model->num_bone * sizeof(pMat4));
	PE_ASSERT(final_bone_matrix != NULL);
	for (int b = 0; b < model->num_bone; b += 1) {
		peAnimationJoint *animation_joint = &model_space_joints[b];
		pMat4 translation = p_translate(animation_joint->translation);
		pMat4 rotation = p_quat_to_mat4(animation_joint->rotation);
		pMat4 scale = p_scale(animation_joint->scale);
		pMat4 transform = p_mat4_mul(translation, p_mat4_mul(scale, rotation));

		pMat4 *inverse_model_space = &model->bone_inverse_model_space_pose_matrix[b];

		final_bone_matrix[b] = p_mat4_mul(transform, *inverse_model_space);
	}
	PE_TRACE_MARK_END(tm_calc_matrices);

	ScePspFMatrix4 bone_matrix[8];

	int gu_texture_2d_status = sceGuGetStatus(GU_TEXTURE_2D);
	for (int m = 0; m < model->num_mesh; m += 1) {
		peTraceMark tm_draw_mesh = PE_TRACE_MARK_BEGIN("draw mesh");
		peSubskeleton *subskeleton = &model->subskeleton[model->mesh_subskeleton[m]];
		peTraceMark tm_assign_matrices = PE_TRACE_MARK_BEGIN("assign matrices");
		for (int b = 0; b < subskeleton->num_bones; b += 1) {
			sceGuBoneMatrix(b, &final_bone_matrix[subskeleton->bone_indices[b]]._sce);
			//sceGuMorphWeight(b, 1.0f);
		}
		PE_TRACE_MARK_END(tm_assign_matrices);

		peMaterial *mesh_material = &model->material[model->mesh_material[m]];

		peTraceMark tm_diffuse_color = PE_TRACE_MARK_BEGIN("diffuse color");
		uint32_t diffuse_color = mesh_material->diffuse_color.rgba;
		sceGuColor(diffuse_color);
		PE_TRACE_MARK_END(tm_diffuse_color);

		peTraceMark tm_bind_texture = PE_TRACE_MARK_BEGIN("bind texture");
		if (mesh_material->has_diffuse_map) {
            pe_texture_bind(mesh_material->diffuse_map);
		} else {
            pe_texture_bind_default();
		}
		PE_TRACE_MARK_END(tm_bind_texture);

		peTraceMark tm_draw_array = PE_TRACE_MARK_BEGIN("draw array");
		int count = (model->mesh[m].index != NULL) ? model->mesh[m].num_index : model->mesh[m].num_vertex;
		sceGumDrawArray(GU_TRIANGLES, model->mesh[m].vertex_type, count, model->mesh[m].index, model->mesh[m].vertex);
		PE_TRACE_MARK_END(tm_draw_array);
		PE_TRACE_MARK_END(tm_draw_mesh);
	}
	peTraceMark tm_diffuse_color = PE_TRACE_MARK_BEGIN("diffuse color");
	sceGuColor(0xFFFFFFFF);
	PE_TRACE_MARK_END(tm_diffuse_color);
	sceGuSetStatus(GU_TEXTURE_2D, gu_texture_2d_status);
	sceGumPopMatrix();
#endif
	pe_arena_temp_end(temp_arena_memory);
	PE_TRACE_FUNCTION_END();
}