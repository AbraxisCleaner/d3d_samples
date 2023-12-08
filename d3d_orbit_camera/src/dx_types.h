#ifndef _DX_TYPES_H_
#define _DX_TYPES_H_
#include "pch.h"
#include <d3d11.h>

////////////////// BUFFERS //////////////////
struct DxVertexBuffer 
{
    ID3D11Buffer *m_handle;
    uint m_element_stride;
    uint m_num_elements;
    uint m_offset;

    DxVertexBuffer() { ZeroThat(this); }
    ~DxVertexBuffer() { Release(); }

    bool Create(ID3D11Device *device, void *data, uint element_stride, uint num_elements);
    void Release();

    __forceinline DxVertexBuffer *Bind(ID3D11DeviceContext *con) {
        con->IASetVertexBuffers(0, 1, &m_handle, &m_element_stride, &m_offset);
        return this;
    }
};

struct DxIndexBuffer 
{
    ID3D11Buffer *m_handle;
    uint m_num_elements;

    DxIndexBuffer() { ZeroThat(this); }
    ~DxIndexBuffer() { Release(); }

    bool Create(ID3D11Device *device, void *data, uint num_elements);
    void Release();

    __forceinline DxIndexBuffer *Bind(ID3D11DeviceContext *con) {
        con->IASetIndexBuffer(m_handle, DXGI_FORMAT_R32_UINT, 0);
        return this;
    }
};

////////////////// SHADERS //////////////////
struct ShaderBuffer 
{
    void *data;
    uint  size;
};

struct DxVertexShader 
{
    ID3D11VertexShader *m_handle;
    ID3D11InputLayout *m_layout;
    ID3D11Buffer **m_cbuffer_handles;
    ShaderBuffer *m_cbuffers;
    uint m_num_cbuffers;

    DxVertexShader();
    ~DxVertexShader();

    bool Create(ID3D11Device *device, void *bc, size_t bc_size);
    bool Compile(ID3D11Device *device, char *path);

    __forceinline DxVertexShader *UpdateAllCBuffers(ID3D11DeviceContext *con) {   
        if (!m_num_cbuffers)
            return this;

        for (auto i = 0; i != m_num_cbuffers; ++i)
        {
            D3D11_MAPPED_SUBRESOURCE ms;
            if (!FAILED(con->Map(m_cbuffer_handles[i], 0, D3D11_MAP_WRITE_DISCARD, 0, &ms)))
            {
                memcpy(ms.pData, m_cbuffers[i].data, m_cbuffers[i].size);
                con->Unmap(m_cbuffer_handles[i], 0);
            }
        }
         return this;
    }
    __forceinline DxVertexShader *Bind(ID3D11DeviceContext *con) {
        con->VSSetShader(m_handle, nullptr, 0);
        con->IASetInputLayout(m_layout);
        con->VSSetConstantBuffers(0, m_num_cbuffers, m_cbuffer_handles);
        return this;
    }
};

struct DxPixelShader 
{
    ID3D11PixelShader *m_handle;
    ID3D11Buffer **m_cbuffer_handles;
    ShaderBuffer *m_cbuffers;
    uint m_num_cbuffers;

    DxPixelShader();
    ~DxPixelShader();

    bool Create(ID3D11Device *device, void *bc, size_t bc_size);
    bool Compile(ID3D11Device *device, char *path);
    void Release();

    __forceinline DxPixelShader *UpdateAllCBuffers(ID3D11DeviceContext *con) {   
        if (!m_num_cbuffers)
            return this;

        for (auto i = 0; i != m_num_cbuffers; ++i)
        {
            D3D11_MAPPED_SUBRESOURCE ms;
            if (!FAILED(con->Map(m_cbuffer_handles[i], 0, D3D11_MAP_WRITE_DISCARD, 0, &ms)))
            {
                memcpy(ms.pData, m_cbuffers[i].data, m_cbuffers[i].size);
                con->Unmap(m_cbuffer_handles[i], 0);
            }
        }
         return this;
    }
    __forceinline DxPixelShader *Bind(ID3D11DeviceContext *con) {
        con->PSSetShader(m_handle, nullptr, 0);
        con->PSSetConstantBuffers(0, m_num_cbuffers, m_cbuffer_handles);
        return this;
    }

};

////////////////// IMAGE //////////////////
enum DxImageType
{
    DxImageType_Null,
    DxImageType_ShaderResource,
    DxImageType_RenderTarget,
    DxImageType_DepthStencil,
};

struct DxImage 
{
    ID3D11Texture2D *m_handle;
    uint m_size[2];
    DxImageType m_type;

    DxImage() { ZeroThat(this); }
    ~DxImage() { Release(); }

    bool Create(ID3D11Device *device, void *data, uint width, uint height, DxImageType type);
    void *CreateView(ID3D11Device *device, DxImageType type);
    void Release();
};

#endif