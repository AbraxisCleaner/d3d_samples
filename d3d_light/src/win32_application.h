#ifndef _WIN32_APPLICATION_H_
#define _WIN32_APPLICATION_H_
#include "stdafx.h"

/// ================== WIN32 ================== ///
struct Input_State {
    int8_t kbd[256];
    int8_t mouse_buttons;
    int mouse_position[2];
    int mouse_delta[2];
    int mouse_wheel_delta;
};

struct Win32_State {
    HINSTANCE hinstance;
    int64_t clock_freq;
    WNDCLASSEXA wndclass;
    HWND hwnd;
    uint hwnd_size[2];

    bool quit;
    bool resized;
    
    Input_State input;
    Input_State input_last_frame;
};

static Win32_State win32 = {};

static inline bool get_key(int key) {
    return win32.input.kbd[key];
}
static inline bool get_key_down(int key) {
    return (win32.input.kbd[key] && !win32.input_last_frame.kbd[key]);
}

static LRESULT CALLBACK win32_wnd_proc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam) {
    if (win32.hinstance) {
        switch (umsg) {
            case WM_CLOSE: win32.quit = true; break;
            case WM_SIZE:
            {
                RECT rect;
                GetClientRect(hwnd, &rect);
                win32.hwnd_size[0] = (rect.right - rect.left);
                win32.hwnd_size[1] = (rect.bottom - rect.top);
                win32.resized = true;
            } break;
            
            case WM_KEYDOWN: win32.input.kbd[wparam] = true; break;
            case WM_KEYUP: win32.input.kbd[wparam] = false; break;
                
            case WM_MOUSEWHEEL: win32.input.mouse_wheel_delta = GET_WHEEL_DELTA_WPARAM(wparam) / 120; break;
            case WM_MOUSEMOVE: {
                POINTS pts = MAKEPOINTS(lparam);
                win32.input.mouse_position[0] = pts.x;
                win32.input.mouse_position[1] = pts.y;
            } break;
                
            case WM_LBUTTONDOWN: win32.input.mouse_buttons |= 1; break;
            case WM_RBUTTONDOWN: win32.input.mouse_buttons |= 2; break;
            case WM_MBUTTONDOWN: win32.input.mouse_buttons |= 4; break;
            case WM_LBUTTONUP: win32.input.mouse_buttons &= ~1; break;
            case WM_RBUTTONUP: win32.input.mouse_buttons &= ~2; break;
            case WM_MBUTTONUP: win32.input.mouse_buttons &= ~4; break;

        }
    }

    return DefWindowProcA(hwnd, umsg, wparam, lparam);
}

static void initialize_win32() {
    win32.hinstance = GetModuleHandleA(NULL);

    win32.wndclass.cbSize = sizeof(WNDCLASSEXA);
    win32.wndclass.hInstance = win32.hinstance;
    win32.wndclass.hCursor = LoadCursor(win32.hinstance, IDC_ARROW);
    win32.wndclass.style = CS_VREDRAW|CS_HREDRAW;
    win32.wndclass.lpszClassName = "D3D_Light_WndClass";
    win32.wndclass.lpfnWndProc = win32_wnd_proc;
    ASSERT(RegisterClassExA(&win32.wndclass) != 0);

    MONITORINFO monitor = { sizeof(MONITORINFO) };
    GetMonitorInfoA(MonitorFromPoint({}, MONITOR_DEFAULTTOPRIMARY), &monitor);

    RECT wnd_rect = { 0, 0, 1600, 1200 };
    AdjustWindowRect(&wnd_rect, WS_OVERLAPPEDWINDOW, false);

    int px = ((monitor.rcWork.right / 2) - (wnd_rect.right / 2));
    int py = ((monitor.rcWork.bottom / 2) - (wnd_rect.bottom / 2));

    win32.hwnd = CreateWindowExA(0,
                                 win32.wndclass.lpszClassName,
                                 "D3D Light Sample",
                                 WS_OVERLAPPEDWINDOW,
                                 px,
                                 py,
                                 wnd_rect.right,
                                 wnd_rect.bottom,
                                 NULL,
                                 NULL,
                                 win32.hinstance,
                                 NULL);
    ASSERT(win32.hwnd);

    GetClientRect(win32.hwnd, &wnd_rect);
    win32.hwnd_size[0] = (wnd_rect.right - wnd_rect.left);
    win32.hwnd_size[1] = (wnd_rect.bottom - wnd_rect.top);

    ShowWindow(win32.hwnd, SW_SHOW);
    LOG("Finished.\n");
}

static void release_win32() {
    DestroyWindow( win32.hwnd );
    UnregisterClassA( win32.wndclass.lpszClassName, win32.hinstance );
}

static void pump_events() {
    win32.resized = false;
    memcpy(&win32.input_last_frame, &win32.input, sizeof(Input_State));
    win32.input.mouse_wheel_delta = 0;
    win32.input.mouse_delta[0] = 0;
    win32.input.mouse_delta[1] = 0;
    
    MSG msg;
    while (PeekMessageA(&msg, NULL, 0, 0, PM_NOREMOVE)) {
        GetMessageA(&msg, NULL, 0, 0);
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    win32.input.mouse_delta[0] = win32.input.mouse_position[0] - win32.input_last_frame.mouse_position[0];
    win32.input.mouse_delta[1] = win32.input.mouse_position[1] - win32.input_last_frame.mouse_position[1];
}

static inline int64_t get_clock() {
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    return li.QuadPart;
}

/// ================== D3D ================== ///
#include <d3d11.h>
#include <dxgi1_3.h>
#include <dxgidebug.h>
#include <d3d11shader.h>
#include <d3dcompiler.h>

struct D3D_State {
    IDXGIFactory2 *factory;
    IDXGIAdapter1 *adapter;
    ID3D11Device *device;
    ID3D11DeviceContext *context;
    IDXGISwapChain1 *swapchain;

    ID3D11RasterizerState *wf_rasterizer;
    ID3D11RasterizerState *cw_rasterizer;
    
    ID3D11Resource *backbuffer;
    ID3D11Texture2D *depthbuffer;
    ID3D11RenderTargetView *backbuffer_view;
    ID3D11DepthStencilView *depthbuffer_view;
};

static D3D_State d3d = {};

int initialize_d3d() {
    CreateDXGIFactory1(IID_PPV_ARGS(&d3d.factory));

    ///
    IDXGIAdapter1 *adapter = nullptr;
    DXGI_ADAPTER_DESC1 adapter_desc = {};
    int adapter_index = 0;

    for (auto i = 0; d3d.factory->EnumAdapters1(i, &adapter) == S_OK; ++i) {
        DXGI_ADAPTER_DESC1 adapter_desc2;
        adapter->GetDesc1(&adapter_desc2);

        if (adapter_desc2.DedicatedVideoMemory > adapter_desc.DedicatedVideoMemory) {
            if (d3d.adapter) 
                d3d.adapter->Release();

            d3d.adapter = adapter;
            adapter_index = i;
        }
        else {
            adapter->Release();
        }
    }

    ASSERT(d3d.adapter);

    DISPLAY_DEVICEA display_device;
    display_device.cb = sizeof(DISPLAY_DEVICEA);
    EnumDisplayDevicesA(NULL, adapter_index, &display_device, 0);
    LOGF("Chosen video adapter: %s\n", display_device.DeviceString);

    ///
    D3D_FEATURE_LEVEL feature_level = D3D_FEATURE_LEVEL_11_0;
    ASSERT(!FAILED(D3D11CreateDevice(d3d.adapter,
                                     D3D_DRIVER_TYPE_UNKNOWN,
                                     NULL,
                                     D3D11_CREATE_DEVICE_DEBUG,
                                     &feature_level,
                                     1,
                                     D3D11_SDK_VERSION,
                                     &d3d.device,
                                     NULL,
                                     &d3d.context)));

    ///
    DXGI_SWAP_CHAIN_DESC1 sc_desc = {};
    sc_desc.BufferCount = 1;
    sc_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sc_desc.SampleDesc.Count = 1;
    sc_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    sc_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sc_desc.Width = win32.hwnd_size[0];
    sc_desc.Height = win32.hwnd_size[1];

    ASSERT(!FAILED(d3d.factory->CreateSwapChainForHwnd(d3d.device, win32.hwnd, &sc_desc, NULL, NULL, &d3d.swapchain)));
    d3d.swapchain->GetBuffer(0, IID_PPV_ARGS(&d3d.backbuffer));
    d3d.device->CreateRenderTargetView(d3d.backbuffer, NULL, &d3d.backbuffer_view);

    ///
    D3D11_RASTERIZER_DESC rz_desc = {};
    rz_desc.FillMode = D3D11_FILL_SOLID;
    rz_desc.CullMode = D3D11_CULL_NONE;
    rz_desc.DepthBias = 1;
    d3d.device->CreateRasterizerState(&rz_desc, &d3d.cw_rasterizer);
    rz_desc.FillMode = D3D11_FILL_WIREFRAME;
    d3d.device->CreateRasterizerState(&rz_desc, &d3d.wf_rasterizer);
    
    ///
    D3D11_TEXTURE2D_DESC texture_desc = {};
    texture_desc.Width = win32.hwnd_size[0];
    texture_desc.Height = win32.hwnd_size[1];
    texture_desc.Format = DXGI_FORMAT_R24G8_TYPELESS;
    texture_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE|D3D11_BIND_DEPTH_STENCIL;
    texture_desc.ArraySize = 1;
    texture_desc.MipLevels = 1;
    texture_desc.SampleDesc.Count = 1;
    ASSERT(!FAILED(d3d.device->CreateTexture2D(&texture_desc, NULL, &d3d.depthbuffer)));

    D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc = {};
    dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    dsv_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    ASSERT(!FAILED(d3d.device->CreateDepthStencilView(d3d.depthbuffer, &dsv_desc, &d3d.depthbuffer_view)));
    
    ///
    LOG("Finished.\n");
    return true;
}

void release_d3d() {
    d3d.depthbuffer_view->Release();
    d3d.depthbuffer->Release();
    d3d.backbuffer_view->Release();
    d3d.backbuffer->Release();

    d3d.wf_rasterizer->Release();
    d3d.cw_rasterizer->Release();
    
    d3d.swapchain->Release();
    d3d.context->Release();
    d3d.device->Release();

    d3d.adapter->Release();
    d3d.factory->Release();

    // Debug
    IDXGIDebug1 *dxgi_debug;
    DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgi_debug));
    dxgi_debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
    dxgi_debug->Release();
}

#endif
