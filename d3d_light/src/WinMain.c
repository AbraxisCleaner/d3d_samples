#define HANDMADE_MATH_USE_RADIANS
#include <HandmadeMath.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "array.c"
#include "Application.c"
#include "d3d_types.c"


struct Vertex 
{
    float Position[3];
    float TexCoord[2];
};


void main()
{
    struct Application app;
    Application_Init(&app);
    
    /// MAIN LOOP
    float timestep = 0;
    int64_t clock1 = GetClock();
    int64_t clock2 = 0;

    struct Vertex vertices[] = {
        // +z face slice
        { { -0.5f, -0.5f,  0.5f }, { 0, 1 } },
        { { -0.5f,  0.5f,  0.5f }, { 0, 0 } },
        { {  0.5f,  0.5f,  0.5f }, { 1, 0 } },
        { {  0.5f, -0.5f,  0.5f }, { 1, 1 } },

        // -z face slice
        { { -0.5f, -0.5f, -0.5f }, { 0, 1 } },
        { { -0.5f,  0.5f, -0.5f }, { 0, 0 } },
        { {  0.5f,  0.5f, -0.5f }, { 1, 0 } },
        { {  0.5f, -0.5f, -0.5f }, { 1, 1 } },

        // -x face slice
        { { -0.5f, -0.5f,  0.5f }, { 0, 1 } },
        { { -0.5f, -0.5f, -0.5f }, { 0, 0 } },
        { { -0.5f,  0.5f, -0.5f }, { 1, 0 } },
        { { -0.5f,  0.5f,  0.5f }, { 1, 1 } },

        // +x face slice
        { {  0.5f, -0.5f,  0.5f }, { 0, 1 } },
        { {  0.5f,  0.5f,  0.5f }, { 0, 0 } },
        { {  0.5f,  0.5f, -0.5f }, { 1, 0 } },
        { {  0.5f, -0.5f, -0.5f }, { 1, 1 } },

        // -y face slice
        { { -0.5f, -0.5f,  0.5f }, { 0, 1 } },
        { { -0.5f, -0.5f, -0.5f }, { 0, 0 } },
        { {  0.5f, -0.5f, -0.5f }, { 1, 0 } },
        { {  0.5f, -0.5f,  0.5f }, { 1, 1 } },

        //  y face slice
        { { -0.5f, 0.5f,  0.5f }, { 0, 1 } },
        { { -0.5f, 0.5f, -0.5f }, { 0, 0 } },
        { {  0.5f, 0.5f, -0.5f }, { 1, 0 } },
        { {  0.5f, 0.5f,  0.5f }, { 1, 1 } },
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
        // +y
        20, 21, 22, 22, 23, 20
    };

    ASSERT(GpuBuffer_Create(&app.Pipeline.Vbo, app.DxDevice, vertices, 24, sizeof(struct Vertex), D3D11_BIND_VERTEX_BUFFER));
    ASSERT(GpuBuffer_Create(&app.Pipeline.Ibo, app.DxDevice, indices, 36, sizeof(unsigned int), D3D11_BIND_INDEX_BUFFER));
    ASSERT(GpuShader_Compile(&app.Pipeline.Vs, app.DxDevice, "src\\shaders\\static.hlsl", D3D11_SHVER_VERTEX_SHADER));
    ASSERT(GpuShader_Compile(&app.Pipeline.Ps, app.DxDevice, "src\\shaders\\lit.hlsl", D3D11_SHVER_PIXEL_SHADER));

    int w, h;
    unsigned char *image_data = stbi_load("data\\concrete_col.png", &w, &h, NULL, 4);
    ASSERT(image_data);
    ASSERT(GpuImage_Create(&app.Pipeline.Image, app.DxDevice, image_data, w, h, D3D11_BIND_SHADER_RESOURCE));
    stbi_image_free(image_data);

    HMM_Mat4 *cb0 = (HMM_Mat4 *)(app.Pipeline.Vs.CBuffers.Data[0].Data);
    cb0[1] = HMM_LookAt_LH(HMM_V3(2, 2, 2), HMM_V3(0, 0, 0), HMM_V3(0, 1, 0));

    struct
    {
        HMM_Vec3 Position, Rotation, Scaling;
    }  transform;
    transform.Position = HMM_V3(0, 0, 0);
    transform.Rotation = HMM_V3(0, 0, 0);
    transform.Scaling = HMM_V3(1, 1, 1);
    cb0[0] = HMM_M4D(1);

    while (1)
    {
        Application_PumpEvents(&app);
        if (app.Quit)
            break;

        /// LOGIC
        {
            transform.Rotation.Y += timestep;
            HMM_Mat4 trm = HMM_MulM4(HMM_Rotate_LH(transform.Rotation.X, HMM_V3(1, 0, 0)), HMM_MulM4(HMM_Rotate_LH(transform.Rotation.Y, HMM_V3(0, 1, 0)), HMM_Rotate_LH(transform.Rotation.Z, HMM_V3(0, 0, 1))));
            cb0[0] = HMM_MulM4(HMM_Translate(transform.Position), HMM_MulM4(trm, HMM_Scale(transform.Scaling)));
        }

        /// RENDERING 
        {
            // -- State Setting
            if (app.Resized) {
                IDXGISwapChain1_ResizeBuffers(app.DxSwapChain, 1, app.hWndSize[0], app.hWndSize[1], DXGI_FORMAT_R8G8B8A8_UNORM, 0);
            }

            IDXGISwapChain1_GetBuffer(app.DxSwapChain, 0, &IID_ID3D11Resource, &app.Pipeline.Backbuffer);
            ID3D11Device_CreateRenderTargetView(app.DxDevice, app.Pipeline.Backbuffer, NULL, &app.Pipeline.BackbufferView);

            // -- Programmable State
            float bg_color[4] = { 1, 182.0f/255.0f, 193.0f/255.0f, 1 };
            ID3D11DeviceContext_ClearRenderTargetView(app.DxCon, app.Pipeline.BackbufferView, bg_color);
            ID3D11DeviceContext_ClearDepthStencilView(app.DxCon, app.Pipeline.DepthStencilView, D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL, 1, 0);
            ID3D11DeviceContext_OMSetRenderTargets(app.DxCon, 1, &app.Pipeline.BackbufferView, app.Pipeline.DepthStencilView);

            D3D11_VIEWPORT d3d_viewport;
            ZeroThat(&d3d_viewport);
            d3d_viewport.Width = (float)app.hWndSize[0];
            d3d_viewport.Height = (float)app.hWndSize[1];
            d3d_viewport.MaxDepth = 1;
            ID3D11DeviceContext_RSSetViewports(app.DxCon, 1, &d3d_viewport);
            ID3D11DeviceContext_RSSetState(app.DxCon, app.Pipeline.CwRasterizer);

            ID3D11DeviceContext_IASetPrimitiveTopology(app.DxCon, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            cb0[2] = HMM_Perspective_LH_ZO(HMM_AngleDeg(80.0f), d3d_viewport.Width / d3d_viewport.Height, 0.1f, 100.0f);
            GpuShader_Bind(&app.Pipeline.Vs, app.DxCon);

            GpuShader_Bind(&app.Pipeline.Ps, app.DxCon);
            ID3D11ShaderResourceView *diffuse_image_view = GpuImage_CreateView(&app.Pipeline.Image, app.DxDevice);
            ID3D11DeviceContext_PSSetShaderResources(app.DxCon, 0, 1, &diffuse_image_view);
            ID3D11DeviceContext_PSSetSamplers(app.DxCon, 0, 1, &app.Pipeline.ImageSampler);

            // -- Drawing
            ID3D11DeviceContext_IASetVertexBuffers(app.DxCon, 0, 1, &app.Pipeline.Vbo.Handle, &app.Pipeline.Vbo.ElementStride, &app.Pipeline.Vbo.ElementOffset);
            ID3D11DeviceContext_IASetIndexBuffer(app.DxCon, app.Pipeline.Ibo.Handle, DXGI_FORMAT_R32_UINT, 0);
            ID3D11DeviceContext_DrawIndexed(app.DxCon, app.Pipeline.Ibo.ElementCount, 0, 0);

            // -- Finish
            IDXGISwapChain1_Present(app.DxSwapChain, 0, 0);

            ID3D11ShaderResourceView_Release(diffuse_image_view);
            ID3D11RenderTargetView_Release(app.Pipeline.BackbufferView);
            ID3D11Resource_Release(app.Pipeline.Backbuffer);
        }

        /// TIMING
        {
            clock2 = GetClock();
            timestep = (float)(clock2 - clock1) / (float)app.ClockFreq;
            clock1 = clock2;
        }
    }

    GpuImage_Release(&app.Pipeline.Image);
    GpuShader_Release(&app.Pipeline.Vs);
    GpuShader_Release(&app.Pipeline.Ps);
    GpuBuffer_Release(&app.Pipeline.Vbo);
    GpuBuffer_Release(&app.Pipeline.Ibo);

    Application_Release(&app);
    LOG("No error.\n");
}