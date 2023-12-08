

cbuffer cb0 : register(b0)
{
    matrix world_matrices[32];
    matrix view_matrix;
    matrix projection_matrix;
};


void main(
    float3 position : POSITION,
    float2 texcoord : TEXCOORD,
    float3 normal : NORMAL,
    out float4 out_position : SV_Position,
    out float2 out_texcoord : TEXCOORD,
    out float3 out_normal : NORMAL,
    unsigned int instance_id : SV_InstanceID)
{
    out_position = mul(float4(position, 1), mul(world_matrices[instance_id], mul(view_matrix, projection_matrix)));
    out_texcoord = texcoord;
    out_normal = normal;
}