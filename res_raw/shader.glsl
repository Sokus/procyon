#ifdef PE_VERTEX_SHADER
uniform mat4 matrix_projection;
uniform mat4 matrix_view;
uniform mat4 matrix_model;

uniform bool do_lighting;
uniform vec3 light_vector;

uniform bool has_skeleton;
uniform mat4 matrix_bone[256];

uniform vec3 diffuse_color;

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec2 in_tex_coord;
layout (location = 3) in vec4 in_color;
layout (location = 4) in uvec4 in_bone_index;
layout (location = 5) in vec4 in_bone_weight;

out vec4 frag_color;
out vec2 tex_coord;

void main() {
    vec4 skinned_position = vec4(0.0, 0.0, 0.0, 0.0);
    vec3 skinned_normal = vec3(0.0, 0.0, 0.0);
    if (has_skeleton) {
        for (int b = 0; b < 4; b += 1) {
            if (in_bone_index[b] >= 255) {
                continue;
            }
            mat4 single_bone_matrix = matrix_bone[in_bone_index[b]];
            vec4 single_bone_skinned_position = single_bone_matrix * vec4(in_position, 1.0);
            skinned_position += single_bone_skinned_position * in_bone_weight[b];
            vec3 single_bone_normal_position = mat3(single_bone_matrix) * in_normal;
            skinned_normal += single_bone_normal_position * in_bone_weight[b];

        }
    } else {
        skinned_position = vec4(in_position, 1.0);
        skinned_normal = in_normal;
    }

    vec3 model_space_normal = mat3(matrix_model) * skinned_normal;

    float light_strength = 1.0;
    if (do_lighting) {
        light_strength = clamp(dot(normalize(-light_vector), model_space_normal), 0.0, 1.0) * 0.8 + 0.2;
    }

    vec4 unlit_color = in_color * vec4(diffuse_color, 1.0);
    frag_color = vec4(unlit_color.rgb * vec3(light_strength), unlit_color.a);
    tex_coord = in_tex_coord;

    gl_Position = matrix_projection * matrix_view * matrix_model * skinned_position;
}
#endif // PE_VERTEX_SHADER



#ifdef PE_FRAGMENT_SHADER

in vec4 frag_color;
in vec2 tex_coord;

out vec4 out_color;

uniform sampler2D diffuse_map;

void main() {
    vec4 diffuse_map_value = texture(diffuse_map, tex_coord);
    out_color = diffuse_map_value * frag_color;
}
#endif // PE_FRAGMENT_SHADER