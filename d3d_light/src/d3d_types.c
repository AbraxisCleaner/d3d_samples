#include "d3d_types.h"
#include <d3dcompiler.h>

int GpuBuffer_Create(struct Gpu_Buffer *self, ID3D11Device *device, void *Data, unsigned int ElementCount, unsigned int ElementStride, int BindType)
{
    D3D11_BUFFER_DESC bd;
    D3D11_SUBRESOURCE_DATA sd;
    ZeroThat(&bd);
    ZeroThat(&bd);

    bd.BindFlags = BindType;
    bd.ByteWidth = (ElementCount * ElementStride);
    sd.pSysMem = Data;

    D3D11_SUBRESOURCE_DATA *psd = &sd;
    if (!Data)
        psd = NULL;

    if (FAILED(ID3D11Device_CreateBuffer(device, &bd, psd, &self->Handle)))
    {
        LOGF("Failed: %p, %dx%d, %d\n", Data, ElementCount, ElementStride, BindType);
        return FALSE;
    }

    self->ElementCount = ElementCount;
    self->ElementStride = ElementStride;
    self->ElementOffset = 0;
    self->BindType = BindType;

    return TRUE;
}

void GpuBuffer_Release(struct Gpu_Buffer *self)
{
    ID3D11Buffer_Release(self->Handle);
    ZeroMemory(self, sizeof(*self));
}

int GpuBuffer_Update(struct Gpu_Buffer *self, ID3D11DeviceContext *con, void *Data, unsigned int Size)
{
    if (Size > (self->ElementCount * self->ElementStride))
        Size = (self->ElementCount * self->ElementStride);

    D3D11_MAPPED_SUBRESOURCE ms;
    if (!FAILED(ID3D11DeviceContext_Map(con, (ID3D11Resource *)self->Handle, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms)))
    {
        memcpy(ms.pData, Data, Size);
        ID3D11DeviceContext_Unmap(con, (ID3D11Resource *)self->Handle, 0);
        return TRUE;
    }
    return FALSE;
}



int GpuImage_Create(struct Gpu_Image *self, ID3D11Device *device, void *Data, unsigned int width, unsigned int height, int BindType)
{
    D3D11_TEXTURE2D_DESC td;
    D3D11_SUBRESOURCE_DATA sd;
    D3D11_SUBRESOURCE_DATA *psd = &sd;

    ZeroThat(&td);
    ZeroThat(&sd);

    td.ArraySize = 1;
    td.MipLevels = 1;
    td.SampleDesc.Count = 1;
    td.Width = width;
    td.Height = height;
    td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    switch (BindType)
    {
        case D3D11_BIND_RENDER_TARGET: 
        td.BindFlags |= D3D11_BIND_RENDER_TARGET; 
        break;
        case D3D11_BIND_DEPTH_STENCIL:
        td.BindFlags |= D3D11_BIND_DEPTH_STENCIL;
        td.Format = DXGI_FORMAT_R24G8_TYPELESS;
        break;
    }

    sd.pSysMem = Data;
    sd.SysMemPitch = sizeof(float) * width;
    if (!Data)
        psd = NULL;

    if (FAILED(ID3D11Device_CreateTexture2D(device, &td, psd, &self->Handle)))
    {
        LOGF("Failed: %p, %dx%d, %d\n", Data, width, height, BindType);
        return FALSE;
    }

    self->Size[0] = width;
    self->Size[1] = height;
    self->BindType = BindType;

    return TRUE;
}

void GpuImage_Release(struct Gpu_Image *self)
{
    ID3D11Texture2D_Release(self->Handle);
    ZeroThat(self);
}

void *GpuImage_CreateView(struct Gpu_Image *self, ID3D11Device *device)
{
    switch (self->BindType)
    {
        case D3D11_BIND_RENDER_TARGET:
        {
            ID3D11RenderTargetView *rtv;
            ID3D11Device_CreateRenderTargetView(device, (ID3D11Resource *)self->Handle, NULL, &rtv);
            return rtv;
        } break;

        case D3D11_BIND_DEPTH_STENCIL:
        {
            ID3D11DepthStencilView *dsv;
            D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc;
            ZeroThat(&dsv_desc);
            dsv_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
            dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
            ID3D11Device_CreateDepthStencilView(device, (ID3D11Resource *)self->Handle, &dsv_desc, &dsv);
            return dsv;
        } break;
    }

    ID3D11ShaderResourceView *srv;
    ID3D11Device_CreateShaderResourceView(device, (ID3D11Resource *)self->Handle, NULL, &srv);
    return srv;
}



int GpuShader_Create(struct Gpu_Shader *self, ID3D11Device *device, void *bytecode, size_t bytecode_size)
{
    ID3D11ShaderReflection *reflector;
    D3D11_SHADER_DESC sd;
    D3D11_INPUT_ELEMENT_DESC ie[8];
    ID3D11ShaderReflectionConstantBuffer *CBuffers[8];
    D3D11_SHADER_BUFFER_DESC cbuffer_desc[8];

    if (FAILED(D3DReflect(bytecode, bytecode_size, &IID_ID3D11ShaderReflection, &reflector)))
    {
        LOG("Failed: Shader reflection.\n");
        return FALSE;
    }

    reflector->lpVtbl->GetDesc(reflector, &sd);

    switch (D3D11_SHVER_GET_TYPE(sd.Version))
    {
        case D3D11_SHVER_VERTEX_SHADER:
        {
            ID3D11Device_CreateVertexShader(device, bytecode, bytecode_size, NULL, &self->Vs.Handle);
            self->BindType = D3D11_SHVER_VERTEX_SHADER;

            for (auto i = 0; i != sd.InputParameters; ++i)
            {
                ZeroThat(&ie[i]);
                D3D11_SIGNATURE_PARAMETER_DESC pd;
                reflector->lpVtbl->GetInputParameterDesc(reflector, i, &pd);

                ie[i].SemanticName = pd.SemanticName;
                ie[i].SemanticIndex = pd.SemanticIndex;
                ie[i].InputSlot = 0;
                ie[i].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
                ie[i].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
                ie[i].InstanceDataStepRate = 0;

                switch (pd.ComponentType)
                {
                    case D3D_REGISTER_COMPONENT_SINT32:
                    {
                        switch (pd.Mask)
                        {
                            case 1: ie[i].Format = DXGI_FORMAT_R32_SINT; break;
                            case 3: ie[i].Format = DXGI_FORMAT_R32G32_SINT; break;
                            case 7: ie[i].Format = DXGI_FORMAT_R32G32B32_SINT; break;
                            case 15: ie[i].Format = DXGI_FORMAT_R32G32B32A32_SINT; break;
                        }
                    } break;
                    case D3D_REGISTER_COMPONENT_UINT32:
                    {
                        switch (pd.Mask)
                        {
                            case 1: ie[i].Format = DXGI_FORMAT_R32_UINT; break;
                            case 3: ie[i].Format = DXGI_FORMAT_R32G32_UINT; break;
                            case 7: ie[i].Format = DXGI_FORMAT_R32G32B32_UINT; break;
                            case 15: ie[i].Format = DXGI_FORMAT_R32G32B32A32_UINT; break;
                        }
                    } break;
                    case D3D_REGISTER_COMPONENT_FLOAT32:
                    {
                        switch (pd.Mask)
                        {
                            case 1: ie[i].Format = DXGI_FORMAT_R32_FLOAT; break;
                            case 3: ie[i].Format = DXGI_FORMAT_R32G32_FLOAT; break;
                            case 7: ie[i].Format = DXGI_FORMAT_R32G32B32_FLOAT; break;
                            case 15: ie[i].Format = DXGI_FORMAT_R32G32B32A32_FLOAT; break;
                        }
                    } break;
                }
            }

            ID3D11Device_CreateInputLayout(device, ie, sd.InputParameters, bytecode, bytecode_size, &self->Vs.Layout);
        } break;

        case D3D11_SHVER_PIXEL_SHADER:
        {
            ID3D11Device_CreatePixelShader(device, bytecode, bytecode_size, NULL, &self->Ps.Handle);
            self->BindType = D3D11_SHVER_PIXEL_SHADER;
        } break;
    }

    self->CBuffers.Count = 0;
    if (sd.ConstantBuffers)
    {
        self->CBuffers.Count = sd.ConstantBuffers;
        self->CBuffers.Data = (Shader_Buffer *)malloc(sd.ConstantBuffers * sizeof(Shader_Buffer));
        self->CBuffers.Handles = (ID3D11Buffer **)malloc(sd.ConstantBuffers * sizeof(intptr_t));
        
        for (auto i = 0; i != sd.ConstantBuffers; ++i)
        {
            CBuffers[i] = reflector->lpVtbl->GetConstantBufferByIndex(reflector, i);
            CBuffers[i]->lpVtbl->GetDesc(CBuffers[i], &cbuffer_desc[i]);

            self->CBuffers.Data[i].Data = malloc(cbuffer_desc[i].Size);
            self->CBuffers.Data[i].Size = cbuffer_desc[i].Size;
            memset(self->CBuffers.Data[i].Data, 0, self->CBuffers.Data[i].Size);

            D3D11_BUFFER_DESC bd;
            ZeroThat(&bd);
            bd.ByteWidth = self->CBuffers.Data[i].Size;
            bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
            bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            bd.Usage = D3D11_USAGE_DYNAMIC;
            ID3D11Device_CreateBuffer(device, &bd, NULL, &self->CBuffers.Handles[i]);
        }
    }

    reflector->lpVtbl->Release(reflector);
    return TRUE;
}

int GpuShader_Compile(struct Gpu_Shader *self, ID3D11Device *device, char *path, int type)
{
    LARGE_INTEGER li;
    void *buffer = NULL;
    
    HANDLE f = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 128, NULL);
    if (f == (HANDLE)-1)
    {
        LOGF("Failed: %s\n", path);
        return FALSE;
    }

    GetFileSizeEx(f, &li);
    buffer = malloc(li.QuadPart + 1);
    ((char *)buffer)[li.QuadPart] = 0;

    ReadFile(f, buffer, li.LowPart, NULL, NULL);
    CloseHandle(f);

    const char *version = "vs_5_0";
    if (type == D3D11_SHVER_PIXEL_SHADER)
        version = "ps_5_0";

    ID3DBlob *source_blob, *error_blob;
    if (FAILED(D3DCompile(buffer, li.QuadPart, NULL, NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", version, D3DCOMPILE_PACK_MATRIX_ROW_MAJOR, 0, &source_blob, &error_blob)))
    {
        printf("Failed: %s\n", ((char *)error_blob->lpVtbl->GetBufferPointer(error_blob)));
        error_blob->lpVtbl->Release(error_blob);
        free(buffer);
        return FALSE;
    }

    int result = GpuShader_Create(self, device, source_blob->lpVtbl->GetBufferPointer(source_blob), source_blob->lpVtbl->GetBufferSize(source_blob));
    source_blob->lpVtbl->Release(source_blob);
    free(buffer);
    return result;
}

void GpuShader_Release(struct Gpu_Shader *self)
{
    switch (self->BindType)
    {
        case D3D11_SHVER_VERTEX_SHADER:
        ID3D11VertexShader_Release(self->Vs.Handle);
        ID3D11InputLayout_Release(self->Vs.Layout);
        break;

        case D3D11_SHVER_PIXEL_SHADER:
        ID3D11PixelShader_Release(self->Ps.Handle);
        break;
    }

    if (self->CBuffers.Count)
    {
        for (unsigned int i = 0; i != self->CBuffers.Count; ++i) {
            ID3D11Buffer_Release(self->CBuffers.Handles[i]);
            free(self->CBuffers.Data[i].Data);
        }
        free(self->CBuffers.Handles);
        free(self->CBuffers.Data);
    }

    ZeroThat(self);
}

void GpuShader_Bind(struct Gpu_Shader *self, ID3D11DeviceContext *con)
{
    if (self->CBuffers.Count)
    {
        D3D11_MAPPED_SUBRESOURCE ms;

        for (auto i = 0; i != self->CBuffers.Count; ++i)
        {
            ID3D11DeviceContext_Map(con, (ID3D11Resource *)self->CBuffers.Handles[i], 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
            memcpy(ms.pData, self->CBuffers.Data[i].Data, self->CBuffers.Data[i].Size);
            ID3D11DeviceContext_Unmap(con, (ID3D11Resource *)self->CBuffers.Handles[i], 0);
        }
    }

    switch (self->BindType)
    {
        case D3D11_SHVER_VERTEX_SHADER:
        ID3D11DeviceContext_VSSetShader(con, self->Vs.Handle, NULL, 0);
        ID3D11DeviceContext_IASetInputLayout(con, self->Vs.Layout);
        if (self->CBuffers.Count)
            ID3D11DeviceContext_VSSetConstantBuffers(con, 0, self->CBuffers.Count, self->CBuffers.Handles);
        break;

        case D3D11_SHVER_PIXEL_SHADER:
        ID3D11DeviceContext_PSSetShader(con, self->Ps.Handle, NULL, 0);
        if (self->CBuffers.Count)
            ID3D11DeviceContext_PSSetConstantBuffers(con, 0, self->CBuffers.Count, self->CBuffers.Handles);
        break;
    }
}
