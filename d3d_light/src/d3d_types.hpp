#ifndef _D3D_TYPES_HPP_
#define _D3D_TYPES_HPP_
#include <stdafx.h>
#include <d3d11.h>

struct Gpu_Buffer 
{
    ID3D11Buffer *Handle;
    uint ElementCount;
    uint ElementStride;
    uint ElementOffset;
    int BindType;
    
    bool Create(ID3D11Device *device, void *data, uint element_stride, uint element_count, int bind_type);
    void Release();
    bool Update(ID3D11DeviceContext *con, void *data, uint size);
    void Bind();
};

#endif 
