cbuffer constant_projection : register(b0) {
    column_major float4x4 matrix_projection;
}

cbuffer constant_view : register(b1) {
    column_major float4x4 matrix_view;
}

cbuffer constant_model : register(b2) {
    column_major float4x4 matrix_model;
}

cbuffer constant_light : register(b3) {
    bool do_lighting;
    float3 light_vector;
}

cbuffer constant_material : register(b4) {
    float4 diffuse_color;
}

cbuffer constant_skeleton : register(b5) {
    bool has_skeleton;
    column_major float4x4 matrix_bone[256];
}

struct vs_in {
    float3 position : POS;
    float3 normal : NOR;
    float2 texcoord : TEX;
    float4 color : COL;
    uint bone_index[4] : BONEINDEX;
    float bone_weight[4] : BONEWEIGHT;
};

struct vs_out {
    float4 position : SV_POSITION;
    float2 texcoord : TEX;
    float4 color : COL;
};

Texture2D    mytexture : register(t0);
SamplerState mysampler : register(s0);

vs_out vs_main(vs_in input) {
    float brightness = 1.0f;
    if (do_lighting) {
        brightness = clamp(dot(normalize(-light_vector), mul(matrix_model, input.normal)), 0.0f, 1.0f) * 0.8f + 0.2f;
    }

    float4 skinned_position = float4(0.0f, 0.0f, 0.0f, 0.0f);
    if (has_skeleton) {
        for (int b = 0; b < 4; b += 1) {
            uint bone_index = input.bone_index[b];
            if (bone_index < 255) {
                float4 single_bone_skinned_position = mul(matrix_bone[input.bone_index[b]], float4(input.position, 1.0f));
                skinned_position += mul(single_bone_skinned_position, input.bone_weight[b]);
            }
        }
    } else {
        skinned_position = float4(input.position, 1.0f);
    }

    float4x4 projection = mul(mul(matrix_projection, matrix_view), matrix_model);
    vs_out output;
    output.position = mul(projection, skinned_position);
    output.texcoord = input.texcoord;
    float4 unlit_color = input.color * diffuse_color;
    output.color    = float4(unlit_color.xyz * brightness, input.color.w);
    return output;
}

float4 ps_main(vs_out input) : SV_TARGET {
    float4 diffuse_map_value = mytexture.Sample(mysampler, input.texcoord);
    float4 result = diffuse_map_value * input.color;
    return result;
}