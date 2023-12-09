#ifndef _D3D_TYPE_H_
#define _D3D_TYPE_H_
#include <d3d11.h>
#include <d3d11shader.h>
#include <d3dcompiler.h>
#include "stdafx.h"

#include "Application.h"

struct Gpu_Buffer 
{
    ID3D11Buffer *handle;
    unsigned int element_count;
    unsigned int element_stride;
    unsigned int element_offset;
    int type;
};

bool create_gpu_buffer(Gpu_Buffer &it, ID3D11Device *device, void *data, uint element_count, uint element_stride, int type) {
    D3D11_BUFFER_DESC buffer_desc = {};
    D3D11_SUBRESOURCE_DATA buffer_data = {};
    auto data_ptr = &buffer_data;

    buffer_desc.BindFlags = type;
    buffer_desc.ByteWidth = (element_count * element_stride);

    buffer_data.pSysMem = data;
    if (!data)
        data_ptr = NULL;

    if (FAILED(device->CreateBuffer(&buffer_desc, data_ptr, &it.handle))) {
        LOGF("Failed: %p, %dx%d, %d\n", data, element_count, element_stride, type);
        return false;
    }

    it.element_count = element_count;
    it.element_stride = element_stride;
    it.element_offset = 0;
    it.type = type;
    
    return true;
}

void release_gpu_buffer(Gpu_Buffer &it) {
    it.handle->Release();
    ZeroThat(&it);
}

bool update_gpu_buffer(Gpu_Buffer &it, ID3D11DeviceContext *con, void *data, uint size) {
    if (size > (it.element_count * it.element_stride))
        size = (it.element_count * it.element_stride);

    D3D11_MAPPED_SUBRESOURCE subresource;
    if (!FAILED(con->Map(it.handle, 0, D3D11_MAP_WRITE_DISCARD, 0, &subresource))) {
        memcpy(subresource.pData, data, size);
        con->Unmap(it.handle, 0);
        return true;
    }
    
    return false;
}

void bind_gpu_buffer(Gpu_Buffer &it, ID3D11DeviceContext *con) {
    if (it.type == D3D11_BIND_VERTEX_BUFFER)
        con->IASetVertexBuffers(0, 1, &it.handle, &it.element_stride, &it.element_offset);
    else
        con->IASetIndexBuffer(it.handle, DXGI_FORMAT_R32_UINT, 0);
}

/// ============ GPU IMAGE ============ ///
struct Gpu_Image
{
    ID3D11Texture2D *handle;
    uint size[2];
};

bool create_gpu_image(Gpu_Image &it, ID3D11Device *device, void *data, uint width, uint height) {
    D3D11_TEXTURE2D_DESC texture_desc;
    D3D11_SUBRESOURCE_DATA texture_data;
    auto data_ptr = &texture_data;

    texture_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texture_desc.Width = width;
    texture_desc.Height = height;
    texture_desc.ArraySize = 1;
    texture_desc.MipLevels = 1;
    texture_desc.BindFlags = D3D11_BIND_RENDER_TARGET|D3D11_BIND_SHADER_RESOURCE;
    texture_desc.SampleDesc.Count = 1;
    
    texture_data.pSysMem = data;
    texture_data.SysMemPitch = (4 * width);
    if (!data)
        data_ptr = NULL;

    if (FAILED(device->CreateTexture2D(&texture_desc, data_ptr, &it.handle))) {
        LOGF("Failed: %p, %dx%d\n", data, width, height);
        return false;
    }

    it.size[0] = width;
    it.size[1] = height;
    
    return true;
}

void release_gpu_image(Gpu_Image &it)
{
    it.handle->Release();
    ZeroThat(&it);
}

/// ============ GPU SHADER ============ ///
struct Shader_Buffer
{
    void *data;
    uint size;
}; 

#define GPU_SHADER_TYPE_VS D3D11_SHVER_VERTEX_SHADER

struct Gpu_Shader
{
    union {
        struct {
            ID3D11VertexShader *handle;
            ID3D11InputLayout *layout;
        } vs;
        struct {
            ID3D11PixelShader *handle;
        } ps;
    };

    struct 
    {
        unsigned int count;
        ID3D11Buffer **handles;
        Shader_Buffer *data;
    } cbuffers;
    int type;
};

bool create_gpu_shader(Gpu_Shader &it, ID3D11Device *device, void *bytecode, size_t bytecode_size) {
    ID3D11ShaderReflection *reflector;
    if (FAILED(D3DReflect(bytecode, bytecode_size, IID_PPV_ARGS(&reflector)))) {
        LOGF("Failed to reflect.\n");
        return false;
    }

    D3D11_SHADER_DESC shader_desc;
    reflector->GetDesc(&shader_desc);

    if (D3D11_SHVER_GET_TYPE(shader_desc.Version) == D3D11_SHVER_VERTEX_SHADER) {
        device->CreateVertexShader(bytecode, bytecode_size, NULL, &it.vs.handle);

        D3D11_INPUT_ELEMENT_DESC input_elements[8];
        D3D11_SIGNATURE_PARAMETER_DESC signature_parameter;

        for (auto i = 0; i != shader_desc.InputParameters; ++i) {
            ZeroThat(&input_elements[i]);
            reflector->GetInputParameterDesc(i, &signature_parameter);

            input_elements[i].SemanticName = signature_parameter.SemanticName;
            input_elements[i].SemanticIndex = signature_parameter.SemanticIndex;
            input_elements[i].InputSlot = 0;
            input_elements[i].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
            input_elements[i].InstanceDataStepRate = 0;
            input_elements[i].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;

            switch (signature_parameter.ComponentType) {
                case D3D_REGISTER_COMPONENT_SINT32:
                    switch (signature_parameter.Mask) {
                        case 1: input_elements[i].Format = DXGI_FORMAT_R32_SINT; break;
                        case 3: input_elements[i].Format = DXGI_FORMAT_R32G32_SINT; break;
                        case 7: input_elements[i].Format = DXGI_FORMAT_R32G32B32_SINT; break;
                        case 15: input_elements[i].Format = DXGI_FORMAT_R32G32B32A32_SINT; break;
                    }
                    break;
                case D3D_REGISTER_COMPONENT_UINT32:
                    switch (signature_parameter.Mask) {
                        case 1: input_elements[i].Format = DXGI_FORMAT_R32_UINT; break;
                        case 3: input_elements[i].Format = DXGI_FORMAT_R32G32_UINT; break;
                        case 7: input_elements[i].Format = DXGI_FORMAT_R32G32B32_UINT; break;
                        case 15: input_elements[i].Format = DXGI_FORMAT_R32G32B32A32_UINT; break;
                    }
                    break;
                case D3D_REGISTER_COMPONENT_FLOAT32:
                    switch (signature_parameter.Mask) {
                        case 1: input_elements[i].Format = DXGI_FORMAT_R32_FLOAT; break;
                        case 3: input_elements[i].Format = DXGI_FORMAT_R32G32_FLOAT; break;
                        case 7: input_elements[i].Format = DXGI_FORMAT_R32G32B32_FLOAT; break;
                        case 15: input_elements[i].Format = DXGI_FORMAT_R32G32B32A32_FLOAT; break;
                    }
                    break;
            }
        }

        device->CreateInputLayout(input_elements, shader_desc.InputParameters, bytecode, bytecode_size, &it.vs.layout);
        it.type = D3D11_SHVER_VERTEX_SHADER;
    } else {
        device->CreatePixelShader(bytecode, bytecode_size, NULL, &it.ps.handle);
        it.type = D3D11_SHVER_PIXEL_SHADER;
    }

    it.cbuffers.count = 0;
    if (shader_desc.ConstantBuffers) {
        it.cbuffers.count = shader_desc.ConstantBuffers;
        it.cbuffers.data = (Shader_Buffer *)malloc(sizeof(Shader_Buffer) * it.cbuffers.count);
        it.cbuffers.handles = (ID3D11Buffer **)malloc(sizeof(intptr_t) * it.cbuffers.count);

        ID3D11ShaderReflectionConstantBuffer *cbuffer;
        D3D11_SHADER_BUFFER_DESC cbuffer_desc;

        for (auto i = 0; i != shader_desc.ConstantBuffers; ++i) {
            cbuffer = reflector->GetConstantBufferByIndex(i);
            cbuffer->GetDesc(&cbuffer_desc);

            it.cbuffers.data[i].size = cbuffer_desc.Size;
            it.cbuffers.data[i].data = malloc(cbuffer_desc.Size);
            memset(it.cbuffers.data[i].data, 0, cbuffer_desc.Size);

            D3D11_BUFFER_DESC buffer_desc = {};
            buffer_desc.ByteWidth = cbuffer_desc.Size;
            buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
            buffer_desc.Usage = D3D11_USAGE_DYNAMIC;
            buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            device->CreateBuffer(&buffer_desc, NULL, &it.cbuffers.handles[i]);
        }
    }

    reflector->Release();
    return true;
}

bool compile_gpu_shader(Gpu_Shader &it, ID3D11Device *device, char *path, int type) {
    char *buffer = NULL;
    LARGE_INTEGER li;
    HANDLE file;

    file = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 128, NULL);
    if (file == (HANDLE)-1) {
        LOGF("Failed: %s\n", path);
        return false;
    }

    GetFileSizeEx(file, &li);
    buffer = (char *)malloc(li.QuadPart + 1);
    buffer[li.QuadPart] = 0;

    ReadFile(file, buffer, li.LowPart, NULL, NULL);
    CloseHandle(file);

    ID3DBlob *source_blob, *error_blob;
    const char *model = "vs_5_0";
    if (type == D3D11_SHVER_PIXEL_SHADER)
        model = "ps_5_0";
    
    if (FAILED(D3DCompile(
                   buffer,
                   li.QuadPart,
                   NULL,
                   NULL,
                   D3D_COMPILE_STANDARD_FILE_INCLUDE,
                   "main",
                   model,
                   D3DCOMPILE_PACK_MATRIX_ROW_MAJOR|D3DCOMPILE_ENABLE_STRICTNESS,
                   0,
                   &source_blob,
                   &error_blob)))
    {
        LOGF("%s\n", (char *)error_blob->GetBufferPointer());
        error_blob->Release();
        free(buffer);
        return false;
    }

    bool result = create_gpu_shader(it, device, source_blob->GetBufferPointer(), source_blob->GetBufferSize());
    
    source_blob->Release();
    free(buffer);
    return result;
}

void release_gpu_shader(Gpu_Shader &it) {
    if (it.type == D3D11_SHVER_VERTEX_SHADER) {
        it.vs.handle->Release();
        it.vs.layout->Release();
    }
    else {
        it.ps.handle->Release();
    }

    if (it.cbuffers.count) {
        for (auto i = 0; i != it.cbuffers.count; ++i) {
            free(it.cbuffers.data[i].data);
            it.cbuffers.handles[i]->Release();
        }
        free(it.cbuffers.data);
        free(it.cbuffers.handles);
    }

    ZeroThat(&it);
}

void bind_gpu_shader(Gpu_Shader &it, ID3D11DeviceContext *con) {
    if (it.cbuffers.count) {
        D3D11_MAPPED_SUBRESOURCE subresource;
        for (auto i = 0; i != it.cbuffers.count; ++i) {
            con->Map(it.cbuffers.handles[i], 0, D3D11_MAP_WRITE_DISCARD, 0, &subresource);
            memcpy(subresource.pData, it.cbuffers.data[i].data, it.cbuffers.data[i].size);
            con->Unmap(it.cbuffers.handles[i], 0);
        }
    }
    
    if (it.type == D3D11_SHVER_VERTEX_SHADER) {
        con->VSSetShader(it.vs.handle, NULL, 0);
        con->IASetInputLayout(it.vs.layout);
        
        if (it.cbuffers.count)
            con->VSSetConstantBuffers(0, it.cbuffers.count, it.cbuffers.handles);
    }
    else {
        con->PSSetShader(it.ps.handle, NULL, 0);

        if (it.cbuffers.count)
            con->PSSetConstantBuffers(0, it.cbuffers.count, it.cbuffers.handles);
    }
}


#endif
