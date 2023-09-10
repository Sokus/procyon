#ifdef PE_VERTEX_SHADER
uniform mat4 matrix_projection;
uniform mat4 matrix_view;
uniform mat4 matrix_model;
uniform vec3 light_vector;

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;

out vec3 frag_color;

void main() {
    gl_Position = matrix_projection * matrix_view * matrix_model * vec4(in_position, 1.0);

    vec3 model_space_normal = vec3(matrix_model * vec4(in_normal, 1.0));
    float light = clamp(dot(normalize(-light_vector), model_space_normal), 0.0, 1.0) * 0.8 + 0.2;




    frag_color = vec3(light, light, light);
}
#endif // PE_VERTEX_SHADER



#ifdef PE_FRAGMENT_SHADER
uniform vec3 diffuse_color;

in vec3 frag_color;

out vec3 out_color;

void main() {
    out_color = frag_color;
}
#endif // PE_FRAGMENT_SHADER