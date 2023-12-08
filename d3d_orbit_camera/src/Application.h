#ifndef _APPLICATION_H_
#define _APPLICATION_H_
#include <pch.h>

#include <dxgi1_3.h>
#include <d3d11.h>

#include <stl\Array.h>
#include <dx_types.h>
#include <material.h>
#include "camera.h"

////////////////////////////////////////////////
struct Vertex
{
    float position[3];
    float texcoord[2];
    float normal[3];
};

////////////////////////////////////////////////
struct Application
{
    Application();
    ~Application();

    void PumpEvents();
    void ResizeBackbuffer();
    void Run();

    static Application *instance;
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam);

    __forceinline bool GetKey(int key) { return kbd[0][key]; }
    __forceinline bool GetKeyDown(int key) { return kbd[0][key] && !kbd[1][key]; }

    ///////////////
    int64_t clock_freq;
    HINSTANCE hinstance;
    WNDCLASSEXA wndclass;
    HWND hwnd;
    int hwnd_size[2];
    int8_t kbd[2][256];
    int mouse_wheel_delta[2];
    int mouse_position[2][2];
    int mouse_delta[2];
    int mouse_buttons[2];
    
    bool quit;
    bool resized;
    
    IDXGIFactory2 *dx_factory;
    IDXGIAdapter1 *dx_adapter;
    DISPLAY_DEVICEA dx_display_device_descriptor;
    
    ID3D11Device *dx_device;
    ID3D11DeviceContext *dx_con;
    IDXGISwapChain1 *dx_swapchain;
    
    struct 
    {
        ID3D11Resource *backbuffer_resource;
        ID3D11RenderTargetView *backbuffer_view;
        ID3D11RasterizerState *cw_rasterizer;
        ID3D11RasterizerState *wf_rasterizer;

        TArray<DxVertexBuffer> vbo_storage;
        TArray<DxIndexBuffer> ibo_storage;
        TArray<DxImage> image_storage;
        TArray<DxVertexShader> vs_storage;
        TArray<DxPixelShader> ps_storage;
        TArray<Material> material_storage;
    } pipeline;

    Camera camera;
};

#define ELAPSED_MS(start, end) (((float)(end - start)) / (float)Application::instance->clock_freq)

inline int64_t get_clock()
{
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    return li.QuadPart;
}

#endif