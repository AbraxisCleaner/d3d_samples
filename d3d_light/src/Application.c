#include "Application.h"
#include <dxgidebug.h>

int Application_Init(struct Application *self)
{
    LARGE_INTEGER li;
    QueryPerformanceFrequency(&li);
    self->ClockFreq = li.QuadPart;

    self->hInstance = GetModuleHandleA(NULL);
    
    ZeroMemory(&self->WndClass, sizeof(WNDCLASSEXA));
    self->WndClass.cbSize = sizeof(WNDCLASSEXA);
    self->WndClass.cbWndExtra = sizeof(LONG_PTR);
    self->WndClass.hInstance = self->hInstance;
    self->WndClass.hCursor = LoadCursor(self->hInstance, IDC_ARROW);
    self->WndClass.lpfnWndProc = Application_WndProc;
    self->WndClass.lpszClassName = "d3d_light_wndclass";
    self->WndClass.style = CS_VREDRAW|CS_HREDRAW;
    ASSERT(RegisterClassExA(&self->WndClass) != 0);

    MONITORINFO monitor;
    POINT pt;
    RECT wnd_rect;
    ZeroMemory(&monitor, sizeof(monitor));
    ZeroMemory(&pt, sizeof(pt));
    ZeroMemory(&wnd_rect, sizeof(wnd_rect));
    
    monitor.cbSize = sizeof(monitor);
    wnd_rect.right = 1600;
    wnd_rect.bottom = 1200;

    GetMonitorInfoA(MonitorFromPoint(pt, MONITOR_DEFAULTTOPRIMARY), &monitor);
    AdjustWindowRect(&wnd_rect, WS_OVERLAPPEDWINDOW, FALSE);

    int px = ((monitor.rcWork.right / 2) - (wnd_rect.right / 2));
    int py = ((monitor.rcWork.bottom / 2) - (wnd_rect.bottom / 2));

    self->hWnd = CreateWindowExA(
        0,
        self->WndClass.lpszClassName,
        "D3D Light", 
        WS_OVERLAPPEDWINDOW,
        px,
        py,
        wnd_rect.right,
        wnd_rect.bottom,
        NULL,
        NULL,
        self->hInstance,
        NULL);
    ASSERT(self->hWnd);
    SetWindowLongPtrA(self->hWnd, 0, (LONG_PTR)self);

    self->Quit = FALSE;
    self->Resized = FALSE;

    GetClientRect(self->hWnd, &wnd_rect);
    self->hWndSize[0] = (wnd_rect.right - wnd_rect.left);
    self->hWndSize[1] = (wnd_rect.bottom - wnd_rect.top);

    /// DXGI && D3D
    D3D_FEATURE_LEVEL feature_level = D3D_FEATURE_LEVEL_11_0;

    DXGI_SWAP_CHAIN_DESC1 sc_desc;
    ZeroMemory(&sc_desc, sizeof(sc_desc));
    sc_desc.BufferCount = 1;
    sc_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sc_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    sc_desc.SampleDesc.Count = 1;
    sc_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sc_desc.Width = self->hWndSize[0];
    sc_desc.Height = self->hWndSize[1];

    ASSERT(!FAILED(CreateDXGIFactory1(&IID_IDXGIFactory2, &self->DxFactory)));
    ASSERT(!FAILED(D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, D3D11_CREATE_DEVICE_DEBUG, &feature_level, 1, D3D11_SDK_VERSION, &self->DxDevice, NULL, &self->DxCon)));
    ASSERT(!FAILED(IDXGIFactory2_CreateSwapChainForHwnd(self->DxFactory, (IUnknown *)self->DxDevice, self->hWnd, &sc_desc, NULL, NULL, &self->DxSwapChain)));

    D3D11_TEXTURE2D_DESC dsv_image_desc;
    ZeroThat(&dsv_image_desc);
    dsv_image_desc.Format = DXGI_FORMAT_R24G8_TYPELESS;
    dsv_image_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE|D3D11_BIND_DEPTH_STENCIL;
    dsv_image_desc.ArraySize = 1;
    dsv_image_desc.MipLevels = 1;
    dsv_image_desc.SampleDesc.Count = 1;
    dsv_image_desc.Width = self->hWndSize[0];
    dsv_image_desc.Height = self->hWndSize[1];
    ASSERT(!FAILED(ID3D11Device_CreateTexture2D(self->DxDevice, &dsv_image_desc, NULL, &self->Pipeline.DepthStencil)));

    D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc;
    ZeroThat(&dsv_desc);
    dsv_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    ASSERT(!FAILED(ID3D11Device_CreateDepthStencilView(self->DxDevice, (ID3D11Resource *)self->Pipeline.DepthStencil, &dsv_desc, &self->Pipeline.DepthStencilView)));

    D3D11_RASTERIZER_DESC rz_desc;
    ZeroThat(&rz_desc);
    rz_desc.FillMode = D3D11_FILL_WIREFRAME;
    rz_desc.CullMode = D3D11_CULL_NONE;
    rz_desc.DepthBias = 1;
    ID3D11Device_CreateRasterizerState(self->DxDevice, &rz_desc, &self->Pipeline.WfRasterizer);
    rz_desc.FillMode = D3D11_FILL_SOLID;
    ID3D11Device_CreateRasterizerState(self->DxDevice, &rz_desc, &self->Pipeline.CwRasterizer);

    D3D11_SAMPLER_DESC sampler_desc;
    ZeroThat(&sampler_desc);
    sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    ID3D11Device_CreateSamplerState(self->DxDevice, &sampler_desc, &self->Pipeline.ImageSampler);

    ShowWindow(self->hWnd, SW_SHOW);
    LOG("Finished.\n");
    return TRUE;
}

void Application_Release(struct Application *self)
{
    ID3D11SamplerState_Release(self->Pipeline.ImageSampler);
    ID3D11RasterizerState_Release(self->Pipeline.CwRasterizer);
    ID3D11RasterizerState_Release(self->Pipeline.WfRasterizer);

    ID3D11DepthStencilView_Release(self->Pipeline.DepthStencilView);
    ID3D11Texture2D_Release(self->Pipeline.DepthStencil);

    IDXGISwapChain1_Release(self->DxSwapChain);
    ID3D11DeviceContext_Release(self->DxCon);
    ID3D11Device_Release(self->DxDevice);
    IDXGIFactory2_Release(self->DxFactory);

    IDXGIDebug1 *dxgi_debug;
    DXGIGetDebugInterface1(0, &IID_IDXGIDebug1, &dxgi_debug);
    IDXGIDebug1_ReportLiveObjects(dxgi_debug, DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
    IDXGIDebug1_Release(dxgi_debug);

    DestroyWindow(self->hWnd);
    UnregisterClassA(self->WndClass.lpszClassName, self->hInstance);
}

void Application_PumpEvents(struct Application *self)
{
    self->Resized = FALSE;

    MSG msg;
    while (PeekMessageA(&msg, NULL, 0, 0, PM_NOREMOVE))
    {
        GetMessageA(&msg, NULL, 0, 0);
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
}

LRESULT CALLBACK Application_WndProc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
    struct Application *app = (struct Application *)GetWindowLongPtrA(hwnd, 0);
    if (app)
    {
        switch (umsg)
        {
            case WM_CLOSE: app->Quit = TRUE; break;
            case WM_SIZE: 
            {   
                RECT rect;
                GetClientRect(app->hWnd, &rect);
                app->hWndSize[0] = (rect.right - rect.left);
                app->hWndSize[1] = (rect.bottom - rect.top);
                app->Resized = TRUE;
            } break;
        }
    }
    return DefWindowProcA(hwnd, umsg, wparam, lparam);
}