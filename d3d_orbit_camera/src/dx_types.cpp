#include "dx_types.h"

#include <d3dcompiler.h>
#include <d3d11shader.h>

////////////////////////////////////////////////
bool DxVertexBuffer::Create(ID3D11Device *device, void *data, uint element_stride, uint num_elements)
{
    D3D11_BUFFER_DESC bd;
    D3D11_SUBRESOURCE_DATA sd;
    ZeroThat(&bd);
    ZeroThat(&sd);

    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.ByteWidth = element_stride * num_elements;
    sd.pSysMem = data;

    if (FAILED(device->CreateBuffer(&bd, &sd, &m_handle))) {
        LOGF("Failed: %p, %d, %d\n", data, element_stride, num_elements);
        return false;
    }

    m_element_stride = element_stride;
    m_num_elements = num_elements;

    return true;
}

void DxVertexBuffer::Release()
{
    if (m_handle) {
        m_handle->Release();
        ZeroThat(this);
    }
}


bool DxIndexBuffer::Create(ID3D11Device *device, void *data, uint num_elements)
{
    D3D11_BUFFER_DESC bd;
    D3D11_SUBRESOURCE_DATA sd;
    ZeroThat(&bd);
    ZeroThat(&sd);

    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bd.ByteWidth = sizeof(uint) * num_elements;
    sd.pSysMem = data;

    if (FAILED(device->CreateBuffer(&bd, &sd, &m_handle))) {
        LOGF("Failed: %p, %d\n", data, num_elements);
        return false;
    }

    m_num_elements = num_elements;

    return true;
}

void DxIndexBuffer::Release()
{
    if (m_handle) {
        m_handle->Release();
        ZeroThat(this);
    }
}

////////////////////////////////////////////////
DxVertexShader::DxVertexShader()
{
    ZeroThat(this);
}

DxVertexShader::~DxVertexShader()
{
    if (m_handle)
    {
        m_handle->Release();
        m_layout->Release();

        if (m_num_cbuffers)
        {
            for (auto i = 0; i != m_num_cbuffers; ++i)
                m_cbuffer_handles[i]->Release();
            free(m_cbuffer_handles);
        }

        ZeroThat(this);
    }
}

bool DxVertexShader::Create(ID3D11Device *device, void *bc, size_t bc_size)
{
    ID3D11ShaderReflection *reflector;
    D3D11_SHADER_DESC sd;

    ASSERT(!FAILED(D3DReflect(bc, bc_size, IID_PPV_ARGS(&reflector))));
    reflector->GetDesc(&sd);

    // You could also do a check on the shader descriptor for whether or not 
    //   this really is a vertex shader, but since we're compiling shaders right
    //   before this function is called, that check is unneccessary.

    device->CreateVertexShader(bc, bc_size, nullptr, &m_handle);

    D3D11_INPUT_ELEMENT_DESC elements[8];
    D3D11_SIGNATURE_PARAMETER_DESC spd;

    for (auto i = 0; i != sd.InputParameters; ++i)
    {
        reflector->GetInputParameterDesc(i, &spd);
        ZeroThat(&elements[i]);

        elements[i].SemanticName = spd.SemanticName;
        elements[i].SemanticIndex = spd.SemanticIndex;
        elements[i].InputSlot = 0;
        elements[i].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
        elements[i].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
        elements[i].InstanceDataStepRate = 0;

        switch (spd.ComponentType)
        {
            case D3D_REGISTER_COMPONENT_SINT32:
            {
                switch (spd.Mask)
                {
                    case 1: elements[i].Format = DXGI_FORMAT_R32_SINT; break;
                    case 3: elements[i].Format = DXGI_FORMAT_R32G32_SINT; break;
                    case 7: elements[i].Format = DXGI_FORMAT_R32G32B32_SINT; break;
                    case 15: elements[i].Format = DXGI_FORMAT_R32G32B32A32_SINT; break;
                }
            } break;
            
            case D3D_REGISTER_COMPONENT_UINT32:
            {
                switch (spd.Mask)
                {
                    case 1: elements[i].Format = DXGI_FORMAT_R32_UINT; break;
                    case 3: elements[i].Format = DXGI_FORMAT_R32G32_UINT; break;
                    case 7: elements[i].Format = DXGI_FORMAT_R32G32B32_UINT; break;
                    case 15: elements[i].Format = DXGI_FORMAT_R32G32B32A32_UINT; break;
                }
            } break;
            
            case D3D_REGISTER_COMPONENT_FLOAT32:
            {
                switch (spd.Mask)
                {
                    case 1: elements[i].Format = DXGI_FORMAT_R32_FLOAT; break;
                    case 3: elements[i].Format = DXGI_FORMAT_R32G32_FLOAT; break;
                    case 7: elements[i].Format = DXGI_FORMAT_R32G32B32_FLOAT; break;
                    case 15: elements[i].Format = DXGI_FORMAT_R32G32B32A32_FLOAT; break;
                }
            } break;
        }
    }

    device->CreateInputLayout(elements, sd.InputParameters, bc, bc_size, &m_layout);

    if (sd.ConstantBuffers)
    {
        m_num_cbuffers = sd.ConstantBuffers;

        ID3D11ShaderReflectionConstantBuffer *cbuffer[8];
        D3D11_SHADER_BUFFER_DESC cbuffer_desc[8];

        auto total_size = (sd.ConstantBuffers * (sizeof(uintptr_t) + sizeof(ShaderBuffer)));

        for (auto i = 0; i != sd.ConstantBuffers; ++i)
        {
            cbuffer[i] = reflector->GetConstantBufferByIndex(i);
            cbuffer[i]->GetDesc(&cbuffer_desc[i]);
            total_size += cbuffer_desc[i].Size;
        }

        m_cbuffer_handles = (ID3D11Buffer **)malloc(total_size);
        m_cbuffers = (ShaderBuffer *)(((uchar *)m_cbuffer_handles) + (sd.ConstantBuffers * sizeof(uintptr_t)));
        auto temp = (uchar *)(((uchar *)m_cbuffer_handles) + (m_num_cbuffers * (sizeof(uintptr_t) + sizeof(ShaderBuffer))));

        for (auto i = 0; i != sd.ConstantBuffers; ++i)
        {
            D3D11_BUFFER_DESC bd = {};
            bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
            bd.ByteWidth = cbuffer_desc[i].Size;
            bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            bd.Usage = D3D11_USAGE_DYNAMIC;

            device->CreateBuffer(&bd, nullptr, &m_cbuffer_handles[i]);
            m_cbuffers[i].size = cbuffer_desc[i].Size;
            m_cbuffers[i].data = temp;
            memset(m_cbuffers[i].data, 0, m_cbuffers[i].size);
            temp += m_cbuffers[i].size;
        }
    }
    
    reflector->Release();
    return true;
}

bool DxVertexShader::Compile(ID3D11Device *device, char *path)
{
    LARGE_INTEGER li;
    HANDLE f;
    char *buffer = nullptr;

    if ((f = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 128, nullptr)) == (HANDLE)-1)
    {
        LOGF("Failed: %s\n", path);
        return false;
    }

    GetFileSizeEx(f, &li);
    buffer = (char *)malloc(li.QuadPart + 1);
    buffer[li.QuadPart] = 0;

    ReadFile(f, buffer, li.LowPart, nullptr, nullptr);
    CloseHandle(f);

    /// COMPILE
    ID3DBlob *source_blob, *error_blob;
    if (FAILED(D3DCompile(buffer, li.QuadPart, nullptr, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "vs_5_0", D3DCOMPILE_PACK_MATRIX_ROW_MAJOR, 0, &source_blob, &error_blob)))
    {
        LOGF("%s\n", (char *)error_blob->GetBufferPointer());
        error_blob->Release();
        free(buffer);
        return false;
    }

    free(buffer);
    auto result = this->Create(device, source_blob->GetBufferPointer(), source_blob->GetBufferSize());
    source_blob->Release();

    return result;
}



DxPixelShader::DxPixelShader() {
    ZeroThat(this);
}

DxPixelShader::~DxPixelShader() {
    if (m_handle)
    {
        m_handle->Release();

        if (m_num_cbuffers) {
            for (auto i = 0; i != m_num_cbuffers; ++i)
                m_cbuffer_handles[i]->Release();
            free(m_cbuffer_handles);
        }

        ZeroThat(this);
    }
}

bool DxPixelShader::Create(ID3D11Device *device, void *bc, size_t bc_size)
{
    ID3D11ShaderReflection *reflector;
    D3D11_SHADER_DESC sd;

    ASSERT(!FAILED(D3DReflect(bc, bc_size, IID_PPV_ARGS(&reflector))));
    reflector->GetDesc(&sd);

    device->CreatePixelShader(bc, bc_size, nullptr, &m_handle);

    if (sd.ConstantBuffers)
    {
        m_num_cbuffers = sd.ConstantBuffers;

        ID3D11ShaderReflectionConstantBuffer *cbuffer[8];
        D3D11_SHADER_BUFFER_DESC cbuffer_desc[8];

        auto total_size = (sd.ConstantBuffers * (sizeof(uintptr_t) + sizeof(ShaderBuffer)));

        for (auto i = 0; i != sd.ConstantBuffers; ++i)
        {
            cbuffer[i] = reflector->GetConstantBufferByIndex(i);
            cbuffer[i]->GetDesc(&cbuffer_desc[i]);
            total_size += cbuffer_desc[i].Size;
        }

        m_cbuffer_handles = (ID3D11Buffer **)malloc(total_size);
        m_cbuffers = (ShaderBuffer *)(((uchar *)m_cbuffer_handles) + (sd.ConstantBuffers * sizeof(uintptr_t)));
        auto temp = (uchar *)(((uchar *)m_cbuffer_handles) + (m_num_cbuffers * (sizeof(uintptr_t) + sizeof(ShaderBuffer))));

        for (auto i = 0; i != sd.ConstantBuffers; ++i)
        {
            D3D11_BUFFER_DESC bd = {};
            bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
            bd.ByteWidth = cbuffer_desc[i].Size;
            bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            bd.Usage = D3D11_USAGE_DYNAMIC;

            device->CreateBuffer(&bd, nullptr, &m_cbuffer_handles[i]);
            m_cbuffers[i].size = cbuffer_desc[i].Size;
            m_cbuffers[i].data = temp;
            memset(m_cbuffers[i].data, 0, m_cbuffers[i].size);
            temp += m_cbuffers[i].size;
        }
    }
    
    reflector->Release();
    return true;
}

bool DxPixelShader::Compile(ID3D11Device *device, char *path)
{
    LARGE_INTEGER li;
    HANDLE f;
    char *buffer = nullptr;

    if ((f = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 128, nullptr)) == (HANDLE)-1)
    {
        LOGF("Failed: %s\n", path);
        return false;
    }

    GetFileSizeEx(f, &li);
    buffer = (char *)malloc(li.QuadPart + 1);
    buffer[li.QuadPart] = 0;

    ReadFile(f, buffer, li.LowPart, nullptr, nullptr);
    CloseHandle(f);

    /// COMPILE
    ID3DBlob *source_blob, *error_blob;
    if (FAILED(D3DCompile(buffer, li.QuadPart, nullptr, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "ps_5_0", D3DCOMPILE_PACK_MATRIX_ROW_MAJOR, 0, &source_blob, &error_blob)))
    {
        LOGF("%s\n", (char *)error_blob->GetBufferPointer());
        error_blob->Release();
        free(buffer);
        return false;
    }

    free(buffer);
    auto result = this->Create(device, source_blob->GetBufferPointer(), source_blob->GetBufferSize());
    source_blob->Release();

    return result;
}

void DxPixelShader::Release()
{   
    if (m_handle) {
        m_handle->Release();

        if (m_num_cbuffers) {
            for (auto i = 0; i != m_num_cbuffers; ++i) {
                m_cbuffer_handles[i]->Release();
            }
            free(m_cbuffer_handles);
        }

        ZeroThat(this);
    }
}

////////////////////////////////////////////////
bool DxImage::Create(ID3D11Device *device, void *data, uint width, uint height, DxImageType type)
{
    D3D11_TEXTURE2D_DESC td;
    D3D11_SUBRESOURCE_DATA sd;
    D3D11_SUBRESOURCE_DATA *psd = &sd;
    ZeroThat(&td);
    ZeroThat(&sd);

    switch (type) {
        case DxImageType_ShaderResource: 
            td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            break;
        case DxImageType_RenderTarget:
            td.BindFlags = D3D11_BIND_RENDER_TARGET|D3D11_BIND_SHADER_RESOURCE;
            td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            break;
        case DxImageType_DepthStencil:
            td.BindFlags = D3D11_BIND_DEPTH_STENCIL|D3D11_BIND_SHADER_RESOURCE;
            td.Format = DXGI_FORMAT_R24G8_TYPELESS;
            break;
    }

    td.ArraySize = 1;
    td.MipLevels = 1;
    td.SampleDesc.Count = 1;
    td.Width = width;
    td.Height = height;
    
    sd.pSysMem = data;
    sd.SysMemPitch = sizeof(float) * width;
    if (!data)
        psd = nullptr;

    if (FAILED(device->CreateTexture2D(&td, psd, &m_handle))) {
        LOGF("Failed: %p, %dx%d, %d\n", data, width, height, type);
        return false;
    }

    m_size[0] = width;
    m_size[1] = height;
    m_type = type;

    return true;
}

void *DxImage::CreateView(ID3D11Device *device, DxImageType type)
{
    switch (type)
    {
        case DxImageType_ShaderResource: {
            ID3D11ShaderResourceView *srv;
            device->CreateShaderResourceView(m_handle, nullptr, &srv);
            return srv;
        } break;

        case DxImageType_RenderTarget: {
            ID3D11RenderTargetView *srv;
            device->CreateRenderTargetView(m_handle, nullptr, &srv);
            return srv;
        } break;

        case DxImageType_DepthStencil: {
            ID3D11DepthStencilView *dsv;
            D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc;
            ZeroThat(&dsv_desc);

            dsv_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
            dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
            device->CreateDepthStencilView(m_handle, &dsv_desc, &dsv);
            return dsv;
        } break;
    }

    LOGF("Failed: %d\n", type);
    return nullptr;
}

void DxImage::Release()
{
    if (m_handle) {
        m_handle->Release();
        ZeroThat(this);
    }
}
