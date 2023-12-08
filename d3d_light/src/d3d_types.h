#ifndef _D3D_TYPE_H_
#define _D3D_TYPE_H_
#include <stdafx.h>
#include <d3d11.h>
#include <d3d11shader.h>

struct Gpu_Buffer 
{
    ID3D11Buffer *Handle;
    unsigned int ElementCount;
    unsigned int ElementStride;
    unsigned int ElementOffset;
    int BindType;
};
int GpuBuffer_Create(struct Gpu_Buffer *self, ID3D11Device *device, void *Data, unsigned int ElementCount, unsigned int element_stride, int BindType);
void GpuBuffer_Release(struct Gpu_Buffer *self);
int GpuBuffer_Update(struct Gpu_Buffer *self, ID3D11DeviceContext *con, void *Data, unsigned int Size);

/// ============ GPU IMAGE ============ ///
struct Gpu_Image
{
    ID3D11Texture2D *Handle;
    unsigned int BindType;
    unsigned int Size[2];
};
int GpuImage_Create(struct Gpu_Image *self, ID3D11Device *device, void *Data, unsigned int width, unsigned int height, int BindType);
void GpuImage_Release(struct Gpu_Image *self);
void *GpuImage_CreateView(struct Gpu_Image *self, ID3D11Device *device);

/// ============ GPU SHADER ============ ///
typedef struct 
{
    void *Data;
    unsigned int Size;
} Shader_Buffer;

struct Gpu_Shader
{
    union {
        struct {
            ID3D11VertexShader *Handle;
            ID3D11InputLayout *Layout;
        } Vs;
        struct {
            ID3D11PixelShader *Handle;
        } Ps;
    };

    struct 
    {
        unsigned int Count;
        ID3D11Buffer **Handles;
        Shader_Buffer *Data;
    } CBuffers;
    int BindType;
};
int GpuShader_Create(struct Gpu_Shader *self, ID3D11Device *device, void *bytecode, size_t bytecode_size);
int GpuShader_Compile(struct Gpu_Shader *self, ID3D11Device *device, char *path);
void GpuShader_Release(struct Gpu_Shader *self);
void GpuShader_Bind(struct Gpu_Shader *self, ID3D11DeviceContext *con);

#endif