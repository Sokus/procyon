cbuffer constant_projection : register(b0)
{
    column_major float4x4 matrix_projection;
}

cbuffer constant_view : register(b1)
{
    column_major float4x4 matrix_view;
}

cbuffer constant_model : register(b2)
{
    column_major float4x4 matrix_model;
}

cbuffer constant_light : register(b3)
{
    float3 lightvector;
}

cbuffer constant_material : register(b4) {
    bool has_diffuse;
    float4 diffuse_color;
}

struct vs_in
{
    float3 position : POS;
    float3 normal   : NOR;
    float2 texcoord : TEX;
    float3 color    : COL;
};

struct vs_out
{
    float4 position : SV_POSITION;
    float2 texcoord : TEX;
    float4 color    : COL;
};

Texture2D    mytexture : register(t0);
SamplerState mysampler : register(s0);

vs_out vs_main(vs_in input)
{
    float light = clamp(dot(normalize(-lightvector), mul(matrix_model, input.normal)), 0.0f, 1.0f) * 0.8f + 0.2f;

    float4x4 projection = mul(mul(matrix_projection, matrix_view), matrix_model);
    vs_out output;
    output.position = mul(projection, float4(input.position, 1.0f));
    output.texcoord = input.texcoord;
    //float4 base_color = has_diffuse ? diffuse_color : float4(input.color, 1.0f);
    float4 base_color = diffuse_color;
    output.color    = base_color * light;

    return output;
}

float4 ps_main(vs_out input) : SV_TARGET
{
    return mytexture.Sample(mysampler, input.texcoord) * input.color;
}