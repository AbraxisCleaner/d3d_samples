#ifndef _APPLICATION_H_
#define _APPLICATION_H_
#include <stdafx.h>
#include <dxgi1_3.h>
#include <d3d11.h>

#include "d3d_types.h"

struct Application 
{
    void init();
    void release();
    void pump_events();
    void create_pipeline_images();
    void resize_pipeline_images();
    static LRESULT CALLBACK wndproc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam);

public:
    static Application *inst;
    int64_t clock_freq;
    HINSTANCE hinstance;
    WNDCLASSEXA wndclass;
    HWND hwnd;

    int8_t kbd[256];
    int hwnd_size[2];
    int8_t quit;
    int8_t resized;

    IDXGIFactory2 *dx_factory;
    ID3D11Device *d3d_device;
    ID3D11DeviceContext *d3d_context;
    IDXGISwapChain1 *dx_swapchain;

    struct
    {
        ID3D11Resource *backbuffer;
        ID3D11Texture2D *depthbuffer;
        ID3D11RenderTargetView *backbuffer_view;
        ID3D11DepthStencilView *depthbuffer_view;

        union
        {
            struct {
                ID3D11Texture2D *gbuffer_images[4];
                ID3D11ShaderResourceView *shader_resource_views[4];
                ID3D11RenderTargetView *render_target_views[4];
            };
            struct {
                ID3D11Texture2D *color_buffer;
                ID3D11Texture2D *roughness_buffer;
                ID3D11Texture2D *metallic_buffer;
                ID3D11Texture2D *normal_buffer;
                ID3D11ShaderResourceView *color_buffer_view;
                ID3D11ShaderResourceView *roughness_buffer_view;
                ID3D11ShaderResourceView *metallic_buffer_view;
                ID3D11ShaderResourceView *normal_buffer_view;
                ID3D11RenderTargetView *color_buffer_render_target;
                ID3D11RenderTargetView *roughness_buffer_render_target;
                ID3D11RenderTargetView *metallic_buffer_render_target;
                ID3D11RenderTargetView *normal_buffer_render_target;
            };
        };

        ID3D11RasterizerState *wf_rasterizer, *cw_rasterizer;
        
        Gpu_Buffer vbo, ibo;
        Gpu_Shader vs, ps;

        ID3D11SamplerState *image_sampler;
        struct {
            Gpu_Image diffuse;
            Gpu_Image roughness;
            Gpu_Image normal;
            Gpu_Image metallic;
        } material;
    } pipeline;
};

inline int64_t get_clock()
{
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    return li.QuadPart;
}

#endif 
