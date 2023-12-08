#include "Application.h"
#include <dxgidebug.h>

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

        quit = false;
        resized = false;

        GetClientRect(hwnd, &rect);
        hwnd_size[0] = (rect.right - rect.left);
        hwnd_size[1] = (rect.bottom - rect.top);

        memset(kbd, 0, 256);
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
        d3d_device->CreateSamplerState(&sampler_desc, &pipeline.image_sampler);

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
    pipeline.image_sampler->Release();
    
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
    resized = false;
    
    MSG msg;
    while (PeekMessageA(&msg, nullptr, 0, 0, PM_NOREMOVE))
    {
        GetMessageA(&msg, nullptr, 0, 0);
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
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
    ASSERT(!FAILED(d3d_device->CreateTexture2D(&texture_desc, nullptr, &pipeline.color_buffer)));
    ASSERT(!FAILED(d3d_device->CreateTexture2D(&texture_desc, nullptr, &pipeline.roughness_buffer)));
    ASSERT(!FAILED(d3d_device->CreateTexture2D(&texture_desc, nullptr, &pipeline.metallic_buffer)));
    ASSERT(!FAILED(d3d_device->CreateTexture2D(&texture_desc, nullptr, &pipeline.normal_buffer))); // @TODO: Replace with 'normal map' format.
    ASSERT(!FAILED(d3d_device->CreateShaderResourceView(pipeline.color_buffer, nullptr, &pipeline.color_buffer_view)));
    ASSERT(!FAILED(d3d_device->CreateShaderResourceView(pipeline.roughness_buffer, nullptr, &pipeline.roughness_buffer_view)));
    ASSERT(!FAILED(d3d_device->CreateShaderResourceView(pipeline.metallic_buffer, nullptr, &pipeline.metallic_buffer_view)));
    ASSERT(!FAILED(d3d_device->CreateShaderResourceView(pipeline.normal_buffer, nullptr, &pipeline.normal_buffer_view)));
    ASSERT(!FAILED(d3d_device->CreateRenderTargetView(pipeline.color_buffer, nullptr, &pipeline.color_buffer_render_target)));
    ASSERT(!FAILED(d3d_device->CreateRenderTargetView(pipeline.roughness_buffer, nullptr, &pipeline.roughness_buffer_render_target)));
    ASSERT(!FAILED(d3d_device->CreateRenderTargetView(pipeline.metallic_buffer, nullptr, &pipeline.metallic_buffer_render_target)));
    ASSERT(!FAILED(d3d_device->CreateRenderTargetView(pipeline.normal_buffer, nullptr, &pipeline.normal_buffer_render_target)));
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
            
            case WM_KEYDOWN: inst->kbd[wparam] = true; break;
            case WM_KEYUP: inst->kbd[wparam] = false; break;
        }
    }
    return DefWindowProcA(hwnd, umsg, wparam, lparam);
}
