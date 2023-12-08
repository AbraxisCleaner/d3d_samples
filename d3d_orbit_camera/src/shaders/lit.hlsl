

void main(
    float4 position : SV_Position,
    float2 texcoord : TEXCOORD,
    float3 normal : NORMAL,
    out float4 out_pixel : SV_Target)
{
    out_pixel = float4(1, 1, 0, 1);
}