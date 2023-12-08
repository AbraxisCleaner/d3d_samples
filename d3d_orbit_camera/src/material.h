#pragma once
#include <pch.h>

#include <dx_types.h>

struct Material
{
    DxPixelShader *ps;
    DxImage *diffuse_image;
};