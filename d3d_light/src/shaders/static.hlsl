
cbuffer cb0 : register(b0) {
    float4x4 world_matrix;
    float4x4 view_matrix;
    float4x4 proj_matrix;
};

void main(float3 position : POSITION, 
          float2 texcoord : TEXCOORD0,
          float3 normal   : TEXCOORD1,
          out float4 out_position  : SV_Position, 
          out float2 out_texcoord  : TEXCOORD0,
          out float3 out_normal    : TEXCOORD1,
          out float3 out_pixel_ws  : TEXCOORD2)
{
    //out_position = float4(position, 1);
    out_position = mul(float4(position, 1), mul(world_matrix, mul(view_matrix, proj_matrix)));
    out_texcoord = texcoord;
    out_normal = normal;
    out_pixel_ws = mul(position, (float3x3)world_matrix);
}
