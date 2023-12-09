#ifndef _MATERIAL_H_
#define _MATERIAL_H_
#include "stdafx.h"
#include "d3d_types.h"

struct Material
{
    ID3D11SamplerState *image_sampler;
    Gpu_Image diffuse;
    Gpu_Image roughness;
    Gpu_Image metallic;
    Gpu_Image normal;
};

#endif
