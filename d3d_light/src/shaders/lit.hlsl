
SamplerState s0 : register(s0);
Texture2D diffuse_image : register(t0);

void main(float4 position : SV_Position, float2 texcoord : TEXCOORD, out float4 out_pixel : SV_Target)
{
    out_pixel = diffuse_image.Sample(s0, texcoord);
}