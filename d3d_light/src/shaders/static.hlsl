
cbuffer cb0 : register(b0) {
    matrix world_matrix;
    matrix view_matrix;
    matrix proj_matrix;
};

void main(float3 position : POSITION, 
          float2 texcoord : TEXCOORD,
          float3 normal : NORMAL,
          out float4 out_position : SV_Position, 
          out float2 out_texcoord : TEXCOORD,
          out float3 out_normal : NORMAL)
{
    //out_position = float4(position, 1);
    out_position = mul(float4(position, 1), mul(world_matrix, mul(view_matrix, proj_matrix)));
    out_texcoord = texcoord;
    out_normal = normal;
}
