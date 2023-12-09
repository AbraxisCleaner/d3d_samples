
cbuffer cb0 : register(b0) {
    float3 view_pos;
    float3 light_pos;
    float4x4 inverse_transpose_world_matrix;
    int use_lighting;
};

void main(float4 position      : SV_Position,
          float2 texcoord      : TEXCOORD0,
          float3 normal        : TEXCOORD1,
          float3 pixel_ws      : TEXCOORD2,
          out float4 out_pixel : SV_Target)
{
    if (use_lighting)
    {
        float3 light_color = float3(1, 1, 1);
        float  ambient_light = 0.1;    
        float3 object_color = float3(1, 1, 0);
        float specular_strength = 0.5; // spec
    
        float3 ambient = ambient_light * light_color;
        float3 light_dir = normalize(light_pos - pixel_ws);

        // diffuse
        normal = mul((float3x3)inverse_transpose_world_matrix, normal);
        float3 norm = normalize(normal);
        float diff = max(dot(norm, light_dir), 0.0);
        float3 diffuse = diff * light_color;
    
        // specular
        float3 view_dir  = normalize(view_pos - pixel_ws); 
        float3 reflection_dir = reflect(-light_dir, norm);
        float spec = pow(max(dot(view_dir, reflection_dir), 0.0), 32);
        float3 specular = specular_strength * spec * light_color;
    
        float3 result = (ambient + diffuse + specular) * object_color; 
    
        out_pixel = float4(result, 1);
    }
    else
    {
        out_pixel = float4(1, 0.5, 0.5, 1);
    }
}
