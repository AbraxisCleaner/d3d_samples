#ifndef _APPLICATION_H_
#define _APPLICATION_H_
#include "stdafx.h"
#include "d3d_types.h"
#include "material.h"

#include <dxgi1_3.h>
#include <dxgidebug.h>

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
    int hwnd_size[2];
    
    int8_t kbd[2][256];
    int mouse_position[2][2];
    int mouse_delta[2];
    int mouse_buttons[2];
    int mouse_wheel_delta[2];

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
                ID3D11Texture2D *gbuffer_color;
                ID3D11Texture2D *gbuffer_roughness;
                ID3D11Texture2D *gbuffer_metallic;
                ID3D11Texture2D *gbuffer_normal;
                ID3D11ShaderResourceView *gbuffer_color_view;
                ID3D11ShaderResourceView *gbuffer_roughness_view;
                ID3D11ShaderResourceView *gbuffer_metallic_view;
                ID3D11ShaderResourceView *gbuffer_normal_view;
                ID3D11RenderTargetView *gbuffer_color_render_target;
                ID3D11RenderTargetView *gbuffer_roughness_render_target;
                ID3D11RenderTargetView *gbuffer_metallic_render_target;
                ID3D11RenderTargetView *gbuffer_normal_render_target;
            };
        };

        ID3D11RasterizerState *wf_rasterizer, *cw_rasterizer;

        Material material;
    } pipeline;
};

inline int64_t get_clock()
{
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    return li.QuadPart;
}

////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
Application *Application::inst = nullptr;

void Application::init()
{
    // -- Windows Stuff.
    {
        LARGE_INTEGER Li;
        QueryPerformanceFrequency(&Li);
        clock_freq = Li.QuadPart;
    }
    
    hinstance = GetModuleHandleA(nullptr);
    
    ZeroThat(&wndclass);
    wndclass.cbSize = sizeof(WNDCLASSEXA);
    wndclass.hInstance = hinstance;
    wndclass.hCursor = LoadCursor(hinstance, IDC_ARROW);
    wndclass.lpszClassName = "D3D_Light_wndclass";
    wndclass.lpfnWndProc = Application::wndproc;
    wndclass.style = CS_VREDRAW|CS_HREDRAW;
    ASSERT(RegisterClassExA(&wndclass) != 0);
    
    RECT rect = { 0, 0, 1600, 1200 };
    {
        MONITORINFO monitor = { sizeof(MONITORINFO) };

        AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, false);
        GetMonitorInfoA(MonitorFromPoint({}, MONITOR_DEFAULTTOPRIMARY), &monitor);

        auto px = ((monitor.rcWork.right / 2) - (rect.right / 2));
        auto py = ((monitor.rcWork.bottom / 2) - (rect.bottom / 2));

        hwnd = CreateWindowExA(
            0,
            wndclass.lpszClassName,
            "D3D Light",
            WS_OVERLAPPEDWINDOW,
            px,
            py,
            rect.right,
            rect.bottom,
            nullptr,
            nullptr,
            hinstance,
            nullptr);
        ASSERT(hwnd);

        memset(kbd[0], 0, 256);
        mouse_position[0][0] = 0;
        mouse_position[0][1] = 0;
        mouse_wheel_delta[0] = 0;
        mouse_buttons[0] = 0;
        mouse_delta[0] = 0;
        mouse_delta[1] = 0;
        
        quit = false;
        resized = false;

        GetClientRect(hwnd, &rect);
        hwnd_size[0] = (rect.right - rect.left);
        hwnd_size[1] = (rect.bottom - rect.top);
    }

    // -- Init DXGI && D3D
    {
        auto feature_level = D3D_FEATURE_LEVEL_11_0;
        
        ASSERT(!FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&dx_factory))));
        ASSERT(!FAILED(D3D11CreateDevice(
                           nullptr,
                           D3D_DRIVER_TYPE_HARDWARE,
                           nullptr,
                           D3D11_CREATE_DEVICE_DEBUG,
                           &feature_level,
                           1,
                           D3D11_SDK_VERSION,
                           &d3d_device,
                           nullptr,
                           &d3d_context)));

        DXGI_SWAP_CHAIN_DESC1 sc_desc;
        ZeroThat(&sc_desc);
        sc_desc.BufferCount = 1;
        sc_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sc_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
        sc_desc.SampleDesc.Count = 1;
        sc_desc.Width = hwnd_size[0];
        sc_desc.Height = hwnd_size[1];
        sc_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

        ASSERT(!FAILED(dx_factory->CreateSwapChainForHwnd(d3d_device, hwnd, &sc_desc, nullptr, nullptr, &dx_swapchain)));

        D3D11_RASTERIZER_DESC rasterizer_desc;
        ZeroThat(&rasterizer_desc);
        rasterizer_desc.FillMode = D3D11_FILL_WIREFRAME;
        rasterizer_desc.CullMode = D3D11_CULL_NONE;
        rasterizer_desc.DepthBias = 1;
        d3d_device->CreateRasterizerState(&rasterizer_desc, &pipeline.wf_rasterizer);
        rasterizer_desc.FillMode = D3D11_FILL_SOLID;
        d3d_device->CreateRasterizerState(&rasterizer_desc, &pipeline.cw_rasterizer);

        D3D11_SAMPLER_DESC sampler_desc;
        ZeroThat(&sampler_desc);
        sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
        d3d_device->CreateSamplerState(&sampler_desc, &pipeline.material.image_sampler);

        create_pipeline_images();
    }
    
    inst = this;
    LOG("Finished.\n");
    ShowWindow(hwnd, SW_SHOW);
}

void Application::release()
{
    for (auto i = 0; i != 4; ++i)
    {
        pipeline.render_target_views[i]->Release();
        pipeline.shader_resource_views[i]->Release();
        pipeline.gbuffer_images[i]->Release();
    }
    pipeline.material.image_sampler->Release();
    
    pipeline.cw_rasterizer->Release();
    pipeline.wf_rasterizer->Release();

    pipeline.backbuffer_view->Release();
    pipeline.backbuffer->Release();
    pipeline.depthbuffer_view->Release();
    pipeline.depthbuffer->Release();

    dx_swapchain->Release();
    d3d_context->Release();
    d3d_device->Release();
    dx_factory->Release();

    IDXGIDebug1 *dxgi_debug;
    DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgi_debug));
    dxgi_debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
    dxgi_debug->Release();

    DestroyWindow(hwnd);
    UnregisterClassA(wndclass.lpszClassName, hinstance);
    Application::inst = nullptr;
}

void Application::pump_events()
{
    // Save the previous frame state.
    memcpy(kbd[1], kbd[0], 256);
    memcpy(mouse_position[1], mouse_position[0], 8);
    mouse_buttons[1] = mouse_buttons[0];
    mouse_wheel_delta[1] = mouse_wheel_delta[0];
    mouse_wheel_delta[0] = 0;
    mouse_delta[0] = 0;
    mouse_delta[1] = 0;
    resized = false;
    
    MSG msg;
    while (PeekMessageA(&msg, nullptr, 0, 0, PM_NOREMOVE))
    {
        GetMessageA(&msg, nullptr, 0, 0);
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    mouse_delta[0] = mouse_position[0][0] - mouse_position[1][0];
    mouse_delta[1] = mouse_position[0][1] - mouse_position[1][1];
}

void Application::create_pipeline_images()
{
    dx_swapchain->GetBuffer(0, IID_PPV_ARGS(&pipeline.backbuffer));
    d3d_device->CreateRenderTargetView(pipeline.backbuffer, nullptr, &pipeline.backbuffer_view);

    D3D11_TEXTURE2D_DESC texture_desc;
    ZeroThat(&texture_desc);
    
    texture_desc.Width = hwnd_size[0];
    texture_desc.Height = hwnd_size[1];
    texture_desc.MipLevels = 1;
    texture_desc.ArraySize = 1;
    texture_desc.SampleDesc.Count = 1;
        
    // Depth Stencil
    texture_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL|D3D11_BIND_SHADER_RESOURCE;
    texture_desc.Format = DXGI_FORMAT_R24G8_TYPELESS;
    ASSERT(!FAILED(d3d_device->CreateTexture2D(&texture_desc, nullptr, &pipeline.depthbuffer)));

    D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc;
    ZeroThat(&dsv_desc);
    dsv_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    ASSERT(!FAILED(d3d_device->CreateDepthStencilView(pipeline.depthbuffer, &dsv_desc, &pipeline.depthbuffer_view)));

    // GBuffers
    texture_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE|D3D11_BIND_RENDER_TARGET;
    texture_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    ASSERT(!FAILED(d3d_device->CreateTexture2D(&texture_desc, nullptr, &pipeline.gbuffer_color)));
    ASSERT(!FAILED(d3d_device->CreateTexture2D(&texture_desc, nullptr, &pipeline.gbuffer_roughness)));
    ASSERT(!FAILED(d3d_device->CreateTexture2D(&texture_desc, nullptr, &pipeline.gbuffer_metallic)));
    ASSERT(!FAILED(d3d_device->CreateTexture2D(&texture_desc, nullptr, &pipeline.gbuffer_normal))); // @TODO: Replace with 'normal map' format.
    ASSERT(!FAILED(d3d_device->CreateShaderResourceView(pipeline.gbuffer_color, nullptr, &pipeline.gbuffer_color_view)));
    ASSERT(!FAILED(d3d_device->CreateShaderResourceView(pipeline.gbuffer_roughness, nullptr, &pipeline.gbuffer_roughness_view)));
    ASSERT(!FAILED(d3d_device->CreateShaderResourceView(pipeline.gbuffer_metallic, nullptr, &pipeline.gbuffer_metallic_view)));
    ASSERT(!FAILED(d3d_device->CreateShaderResourceView(pipeline.gbuffer_normal, nullptr, &pipeline.gbuffer_normal_view)));
    ASSERT(!FAILED(d3d_device->CreateRenderTargetView(pipeline.gbuffer_color, nullptr, &pipeline.gbuffer_color_render_target)));
    ASSERT(!FAILED(d3d_device->CreateRenderTargetView(pipeline.gbuffer_roughness, nullptr, &pipeline.gbuffer_roughness_render_target)));
    ASSERT(!FAILED(d3d_device->CreateRenderTargetView(pipeline.gbuffer_metallic, nullptr, &pipeline.gbuffer_metallic_render_target)));
    ASSERT(!FAILED(d3d_device->CreateRenderTargetView(pipeline.gbuffer_normal, nullptr, &pipeline.gbuffer_normal_render_target)));
}

void Application::resize_pipeline_images()
{
    for (auto i = 0; i != 4; ++i)
    {
        pipeline.render_target_views[i]->Release();
        pipeline.shader_resource_views[i]->Release();
        pipeline.gbuffer_images[i]->Release();
    }

    pipeline.backbuffer_view->Release();
    pipeline.backbuffer->Release();
    pipeline.depthbuffer_view->Release();
    pipeline.depthbuffer->Release();

    dx_swapchain->ResizeBuffers(1, hwnd_size[0], hwnd_size[1], DXGI_FORMAT_R8G8B8A8_UNORM, 0);
    
    create_pipeline_images();
}

LRESULT CALLBACK Application::wndproc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
    if (inst)
    {
        switch (umsg)
        {
            case WM_CLOSE: inst->quit = true; break;
            case WM_SIZE:
            {
                RECT rect;
                GetClientRect(hwnd, &rect);
                inst->hwnd_size[0] = (rect.right - rect.left);
                inst->hwnd_size[1] = (rect.bottom - rect.top);
                inst->resized = true;
            } break;
            
            case WM_KEYDOWN: inst->kbd[0][wparam] = true; break;
            case WM_KEYUP: inst->kbd[0][wparam] = false; break;
            case WM_MOUSEWHEEL: inst->mouse_wheel_delta[0] = GET_WHEEL_DELTA_WPARAM(wparam) / 120; break;
            case WM_MOUSEMOVE: {
                POINTS pts = MAKEPOINTS(lparam);
                inst->mouse_position[0][0] = pts.x;
                inst->mouse_position[0][1] = pts.y;
            } break;
            case WM_LBUTTONDOWN: inst->mouse_buttons[0] |= 1; break;
            case WM_RBUTTONDOWN: inst->mouse_buttons[0] |= 2; break;
            case WM_MBUTTONDOWN: inst->mouse_buttons[0] |= 4; break;
            case WM_LBUTTONUP: inst->mouse_buttons[0] &= ~1; break;
            case WM_RBUTTONUP: inst->mouse_buttons[0] &= ~2; break;
            case WM_MBUTTONUP: inst->mouse_buttons[0] &= ~4; break;
        }
    }
    return DefWindowProcA(hwnd, umsg, wparam, lparam);
}


#endif 

