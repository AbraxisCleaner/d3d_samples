#include "d3d_types.hpp"

bool Gpu_Buffer::Create(ID3D11Device *device, void *data, uint element_stride, uint element_count, int bind_type)
{
    D3D11_BUFFER_DESC bd = {};
    D3D11_SUBRESOURCE_DATA sd = {};
    auto psd = &sd;

    bd.ByteWidth = element_count * element_stride;
    bd.BindFlags = bind_type;
    sd.pSysMem = data;

    if (!data)
        psd = nullptr;

    if (FAILED(device->CreateBuffer(&bd, psd, &Handle)))
    {
        LOGF("Failed: %p, %dx%d, %d\n", data, element_count, element_stride, bind_type);
        return false;
    }

    ElementCount = element_count;
    ElementStride = element_stride;
    ElementOffset = 0;
    BindType = bind_type;
    
    return true;
}

void Gpu_Buffer::Release()
{
    Handle->Release();
    ZeroThat(this);
}

bool Gpu_Buffer::Update(ID3D11DeviceContext *con, void *data, uint size)
{
    if (size > (ElementStride * ElementCount))
        size = (ElementStride * ElementCount);

    D3D11_MAPPED_SUBRESOURCE ms;
    if (!FAILED(con->Map(Handle, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms)))
    {
        memcpy(ms.pData, data, size);
        con->Unmap(Handle, 0);
        return true;
    }
    return false;
}

void Gpu_Buffer::Bind()
{
    switch (BindType)
    {
        case D3D11_BIND_VERTEX_BUFFER: con->IASetVertexBuffers(0, 1, &Handle, &ElementStride, &ElementOffset); break;
        case D3D11_BIND_INDEX_BUFFER: con->IASetIndexBuffer(Handle, DXGI_FORMAT_R32_UINT, 0); break;
    }
}
