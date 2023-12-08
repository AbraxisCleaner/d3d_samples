#include "Application.h"

#include <dxgidebug.h>

#define HANDMADE_MATH_USE_RADIANS
#include "vendor/HandmadeMath.h"

#define STB_IMAGE_IMPLEMENTATION
#include "vendor/stb_image.h"

Application *Application::instance = nullptr;


void Application::Run() 
{
    float timestep = 0;
    auto timestep_clock1 = get_clock();

    Vertex vertices[] = {
        // +z face slice
        { { -0.5f, -0.5f,  0.5f }, { 0, 1 }, {} },
        { { -0.5f,  0.5f,  0.5f }, { 0, 0 }, {} },
        { {  0.5f,  0.5f,  0.5f }, { 1, 0 }, {} },
        { {  0.5f, -0.5f,  0.5f }, { 1, 1 }, {} },

        // -z face slice
        { { -0.5f, -0.5f, -0.5f }, { 0, 1 }, {} },
        { { -0.5f,  0.5f, -0.5f }, { 0, 0 }, {} },
        { {  0.5f,  0.5f, -0.5f }, { 1, 0 }, {} },
        { {  0.5f, -0.5f, -0.5f }, { 1, 1 }, {} },

        // -x face slice
        { { -0.5f, -0.5f,  0.5f }, { 0, 1 }, {} },
        { { -0.5f, -0.5f, -0.5f }, { 0, 0 }, {} },
        { { -0.5f,  0.5f, -0.5f }, { 1, 0 }, {} },
        { { -0.5f,  0.5f,  0.5f }, { 1, 1 }, {} },

        // +x face slice
        { {  0.5f, -0.5f,  0.5f }, { 0, 1 }, {} },
        { {  0.5f,  0.5f,  0.5f }, { 0, 0 }, {} },
        { {  0.5f,  0.5f, -0.5f }, { 1, 0 }, {} },
        { {  0.5f, -0.5f, -0.5f }, { 1, 1 }, {} },

        // -y face slice
        { { -0.5f, -0.5f,  0.5f }, { 0, 1 }, {} },
        { { -0.5f, -0.5f, -0.5f }, { 0, 0 }, {} },
        { {  0.5f, -0.5f, -0.5f }, { 1, 0 }, {} },
        { {  0.5f, -0.5f,  0.5f }, { 1, 1 }, {} },

        //  y face slice
        { { -0.5f, 0.5f,  0.5f }, { 0, 1 }, {} },
        { { -0.5f, 0.5f, -0.5f }, { 0, 0 }, {} },
        { {  0.5f, 0.5f, -0.5f }, { 1, 0 }, {} },
        { {  0.5f, 0.5f,  0.5f }, { 1, 1 }, {} },
    };

    unsigned int indices[] = {
        // +z
        0, 1, 2, 2, 3, 0,
        // -z
        4, 5, 6, 6, 7, 4,
        // -x
        8, 9, 10, 10, 11, 8,
        // +x
        12, 13, 14, 14, 15, 12,
        // -y
        16, 17, 18, 18, 19, 16,
        // +x
        20, 21, 22, 22, 23, 20
    };

    auto vbo = pipeline.vbo_storage.Append();
    auto ibo = pipeline.ibo_storage.Append();
    auto vs = pipeline.vs_storage.Append();
    auto ps = pipeline.ps_storage.Append();
    auto material = pipeline.material_storage.Append();
    vbo->Create(dx_device, vertices, sizeof(Vertex), 24);
    ibo->Create(dx_device, indices, 36);
    vs->Compile(dx_device, "src\\shaders\\static.hlsl");
    ps->Compile(dx_device, "src\\shaders\\lit.hlsl");
    material->ps = ps;
    material->diffuse_image = nullptr;

    HMM_Mat4 *cb0 = (HMM_Mat4 *)pipeline.vs_storage[0].m_cbuffers[0].data;
    cb0[0] = HMM_M4D(1);
    cb0[1] = HMM_Translate(HMM_V3(-1, -2, -1));
    cb0[2] = HMM_Translate(HMM_V3( 1, -2, -1));

    while (!quit)
    {
        PumpEvents();

        /// LOGIC
        {
            if (GetKey(VK_ESCAPE))
                quit = true;

            camera.Tick(timestep);
        }

        /// RENDERING
        {
            /// -- Static state setting.
            if (resized) {
                dx_swapchain->ResizeBuffers(1, hwnd_size[0], hwnd_size[1], DXGI_FORMAT_R8G8B8A8_UNORM, 0);
            }   

            ID3D11Resource *backbuffer;
            ID3D11RenderTargetView *backbuffer_view;
            dx_swapchain->GetBuffer(0, IID_PPV_ARGS(&backbuffer));
            dx_device->CreateRenderTargetView(backbuffer, nullptr, &backbuffer_view);

            float bg_color[4] = { 1, 182.0f/255.0f, 193.0f/255.0f, 1 };
            dx_con->ClearRenderTargetView(backbuffer_view, bg_color);
            dx_con->OMSetRenderTargets(1, &backbuffer_view, nullptr);

            D3D11_VIEWPORT d3d_viewport = {};
            d3d_viewport.Width = hwnd_size[0];
            d3d_viewport.Height = hwnd_size[1];
            d3d_viewport.MaxDepth = 1;
            dx_con->RSSetViewports(1, &d3d_viewport);

            /// -- Programmable state.
            dx_con->RSSetState(pipeline.wf_rasterizer);
            dx_con->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            /// -- Pipeline
            cb0 = (HMM_Mat4 *)vs->m_cbuffers[0].data;
            cb0[32] = camera.GetMatrix();
            cb0[33] = HMM_Perspective_LH_ZO(HMM_AngleDeg(camera.fov), (float)hwnd_size[0] / (float)hwnd_size[1], camera.view_plane_distance[0], camera.view_plane_distance[1]);

            vs->UpdateAllCBuffers(dx_con)->Bind(dx_con);

            // Material
            material->ps->Bind(dx_con);
            
            // Drawing
            vbo->Bind(dx_con);
            ibo->Bind(dx_con);
            dx_con->DrawIndexedInstanced(ibo->m_num_elements, 3, 0, 0, 0);

            // -- Finalization
            dx_swapchain->Present(0, 0);

            backbuffer_view->Release();
            backbuffer->Release();
        }

        // TIMING
        {
            auto timestep_clock2 = get_clock();
            timestep = (((float)(timestep_clock2 - timestep_clock1)) / (float)clock_freq);
            timestep_clock1 = timestep_clock2;
        }
    }
}

Application::Application() 
{
    hinstance = GetModuleHandleA(nullptr);

    LARGE_INTEGER li;
    QueryPerformanceFrequency(&li);
    clock_freq = li.QuadPart;

    ZeroThat(&wndclass);
    wndclass.cbSize = sizeof(WNDCLASSEXA);
    wndclass.lpszClassName = "CamWndClass";
    wndclass.lpfnWndProc = Application::WndProc;
    wndclass.hInstance = hinstance;
    wndclass.hCursor = LoadCursor(hinstance, IDC_ARROW);
    wndclass.style = CS_VREDRAW|CS_HREDRAW;
    ASSERT(RegisterClassExA(&wndclass) != 0);

    MONITORINFO monitor = { sizeof(MONITORINFO) };
    RECT rect = { 0, 0, 1600, 1200 };
    GetMonitorInfoA(MonitorFromPoint({}, MONITOR_DEFAULTTOPRIMARY), &monitor);
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, false);

    auto px = ((monitor.rcWork.right / 2) - (rect.right / 2));
    auto py = ((monitor.rcWork.bottom / 2) - (rect.bottom / 2));

    hwnd = CreateWindowExA(0, wndclass.lpszClassName, "Window", WS_OVERLAPPEDWINDOW, px, py, rect.right, rect.bottom, nullptr, nullptr, hinstance, nullptr);
    ASSERT(hwnd);

    quit = false;
    resized = false;
    memset(kbd[0], 0, 256);
    mouse_buttons[0] = 0;
    mouse_wheel_delta[0] = 0;

    GetClientRect(hwnd, &rect);
    hwnd_size[0] = (rect.right - rect.left);
    hwnd_size[1] = (rect.bottom - rect.top);

    /// DXGI
    ASSERT(!FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&dx_factory))));

    dx_adapter = nullptr;
    IDXGIAdapter1 *adapter = nullptr;
    DXGI_ADAPTER_DESC1 adapter_desc;
    ZeroThat(&adapter_desc);
    int adapter_index = 0;

    for (auto i = 0; dx_factory->EnumAdapters1(i, &adapter) == S_OK; ++i)
    {
        DXGI_ADAPTER_DESC1 ad;
        adapter->GetDesc1(&ad);

        if (ad.DedicatedVideoMemory > adapter_desc.DedicatedVideoMemory) {
            if (dx_adapter)
                dx_adapter->Release();

            dx_adapter = adapter;
            adapter_index = i;
        } 
        else {
            adapter->Release();
        }
    }

    ASSERT(dx_adapter);
    dx_display_device_descriptor.cb = sizeof(DISPLAY_DEVICEA);
    EnumDisplayDevicesA(nullptr, adapter_index, &dx_display_device_descriptor, 0);
    LOGF("Chosen display device: %s | %s\n", dx_display_device_descriptor.DeviceString, dx_display_device_descriptor.DeviceName);

    /// D3D
    auto feature_level = D3D_FEATURE_LEVEL_11_0;
    ASSERT(!FAILED(D3D11CreateDevice(dx_adapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr, D3D11_CREATE_DEVICE_DEBUG, &feature_level, 1, D3D11_SDK_VERSION, &dx_device, nullptr, &dx_con)));
    
    DXGI_SWAP_CHAIN_DESC1 sc_desc;
    ZeroThat(&sc_desc);
    sc_desc.BufferCount = 1;
    sc_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sc_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sc_desc.SampleDesc.Count = 1;
    sc_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    sc_desc.Width = hwnd_size[0];
    sc_desc.Height = hwnd_size[1];

    ASSERT(!FAILED(dx_factory->CreateSwapChainForHwnd(dx_device, hwnd, &sc_desc, nullptr, nullptr, &dx_swapchain)));

    /// PIPELINE
    D3D11_RASTERIZER_DESC rz_desc = {};
    rz_desc.FillMode = D3D11_FILL_SOLID;
    rz_desc.CullMode = D3D11_CULL_BACK;
    rz_desc.DepthBias = 1;
    ASSERT(!FAILED(dx_device->CreateRasterizerState(&rz_desc, &pipeline.cw_rasterizer)));
    rz_desc.CullMode = D3D11_CULL_NONE;
    rz_desc.FillMode = D3D11_FILL_WIREFRAME;
    ASSERT(!FAILED(dx_device->CreateRasterizerState(&rz_desc, &pipeline.wf_rasterizer)));

    /// WORLD STATE
    camera.fov = 75.0f;
    camera.orbit = HMM_V3(HMM_AngleDeg(25), HMM_AngleDeg(25), 4);
    camera.target = HMM_V3(0, 0, 0);
    camera.view_plane_distance = HMM_V2(0.1f, 100.0f);
    camera.speed = 5;

    /// FINISH
    ShowWindow(hwnd, SW_SHOW);
    instance = this;
    LOG("Finished.\n");    
}

Application::~Application() 
{
    pipeline.wf_rasterizer->Release();
    pipeline.cw_rasterizer->Release();

    pipeline.ps_storage.~TArray();
    pipeline.vs_storage.~TArray();
    pipeline.ibo_storage.~TArray();
    pipeline.vbo_storage.~TArray();

    dx_swapchain->Release();
    dx_con->Release();
    dx_device->Release();
    dx_adapter->Release();
    dx_factory->Release();

    IDXGIDebug1 *dxgi_debug;
    DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgi_debug));
    dxgi_debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
    dxgi_debug->Release();

    DestroyWindow(hwnd);
    UnregisterClassA(wndclass.lpszClassName, hinstance);
    LOG("No error.\n");
}

void Application::PumpEvents() {
    memcpy(kbd[1], kbd[0], 256);
    memcpy(mouse_position[1], mouse_position[0], 2 * sizeof(int));
    mouse_buttons[1] = mouse_buttons[0];
    mouse_wheel_delta[1] = mouse_wheel_delta[0];
    mouse_wheel_delta[0] = 0;
    mouse_delta[0] = 0;
    mouse_delta[1] = 0;
    resized = false;
    
    MSG msg;
    while (PeekMessageA(&msg, 0, 0, 0, PM_NOREMOVE))
    {
        GetMessageA(&msg, 0, 0, 0);
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    mouse_delta[0] = mouse_position[0][0] - mouse_position[1][0];
    mouse_delta[1] = mouse_position[0][1] - mouse_position[1][1];
}

LRESULT CALLBACK Application::WndProc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
    if (instance)
    {
        switch (umsg)
        {
            case WM_CLOSE: instance->quit = true; break;
            case WM_SIZE: {
                instance->resized = true;
                RECT rect;
                GetClientRect(instance->hwnd, &rect);
                instance->hwnd_size[0] = (rect.right - rect.left);
                instance->hwnd_size[1] = (rect.bottom - rect.top);
            } break;
            case WM_KEYDOWN: instance->kbd[0][wparam] = true; break;
            case WM_KEYUP: instance->kbd[0][wparam] = false; break;
            case WM_MOUSEWHEEL: instance->mouse_wheel_delta[0] = GET_WHEEL_DELTA_WPARAM(wparam); break;
            case WM_MOUSEMOVE: { 
                POINTS pts = MAKEPOINTS(lparam); 
                instance->mouse_position[0][0] = pts.x;
                instance->mouse_position[0][1] = pts.y;
            } break;
            case WM_LBUTTONDOWN: instance->mouse_buttons[0] |= 1; break;
            case WM_RBUTTONDOWN: instance->mouse_buttons[0] |= 2; break;
            case WM_MBUTTONDOWN: instance->mouse_buttons[0] |= 4; break;
            case WM_LBUTTONUP: instance->mouse_buttons[0] &= ~1; break;
            case WM_RBUTTONUP: instance->mouse_buttons[0] &= ~2; break;
            case WM_MBUTTONUP: instance->mouse_buttons[0] &= ~4; break;
        }
    }

    return DefWindowProcA(hwnd, umsg, wparam, lparam);
}