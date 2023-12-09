#include "Application.h"

#define HANDMADE_MATH_USE_RADIANS
#include "HandmadeMath.h"

#include "camera.h"

struct Vertex {
    float position[3];
    float texcoord[2];
    float normal[3];
};

struct Transform {
    HMM_Vec3 position;
    HMM_Vec3 rotation;
    HMM_Vec3 scaling;

    HMM_Mat4 as_matrix() {
        auto tm = HMM_Translate(position);
        auto sm = HMM_Scale(scaling);
        auto rm = HMM_MulM4(HMM_Rotate_LH(rotation[0], HMM_V3(1, 0, 0)),
                            HMM_MulM4(HMM_Rotate_LH(rotation[1], HMM_V3(0, 1, 0)),
                                      HMM_Rotate_LH(rotation[2], HMM_V3(0, 0, 1))));
        return HMM_MulM4(tm, HMM_MulM4(rm, sm));
    }

    static Transform zero() {
        Transform tf;
        tf.position = HMM_V3(0, 0, 0);
        tf.rotation = HMM_V3(0, 0, 0);
        tf.scaling = HMM_V3(1, 1, 1);
        return tf;
    }
};

struct VS_CB0 {
    HMM_Mat4 world_matrix;
    HMM_Mat4 view_matrix;
    HMM_Mat4 proj_matrix;
};
struct PS_CB0 {
    HMM_Vec3 view_pos;
    HMM_Vec3 light_pos;
    HMM_Mat4 inverse_transpose_world_matrix;
    int      use_lighting;
};

int main() {
    Application app;
    app.init();

    Gpu_Buffer vbo, ibo;
    Vertex cube_vertices[] = {
        // +z face slice
        { { -0.5f, -0.5f,  0.5f }, { 0, 1 }, { 0, 0, 1 } },
        { { -0.5f,  0.5f,  0.5f }, { 0, 0 }, { 0, 0, 1 } },
        { {  0.5f,  0.5f,  0.5f }, { 1, 0 }, { 0, 0, 1 } },
        { {  0.5f, -0.5f,  0.5f }, { 1, 1 }, { 0, 0, 1 } },

        // -z face slice
        { { -0.5f, -0.5f, -0.5f }, { 0, 1 }, { 0, 0, -1 } },
        { { -0.5f,  0.5f, -0.5f }, { 0, 0 }, { 0, 0, -1 } },
        { {  0.5f,  0.5f, -0.5f }, { 1, 0 }, { 0, 0, -1 } },
        { {  0.5f, -0.5f, -0.5f }, { 1, 1 }, { 0, 0, -1 } },

        // -x face slice
        { { -0.5f, -0.5f,  0.5f }, { 0, 1 }, { -1, 0, 0 } },
        { { -0.5f, -0.5f, -0.5f }, { 0, 0 }, { -1, 0, 0 } },
        { { -0.5f,  0.5f, -0.5f }, { 1, 0 }, { -1, 0, 0 } },
        { { -0.5f,  0.5f,  0.5f }, { 1, 1 }, { -1, 0, 0 } },

        // +x face slice
        { {  0.5f, -0.5f,  0.5f }, { 0, 1 }, { 1, 0, 0 } },
        { {  0.5f,  0.5f,  0.5f }, { 0, 0 }, { 1, 0, 0 } },
        { {  0.5f,  0.5f, -0.5f }, { 1, 0 }, { 1, 0, 0 } },
        { {  0.5f, -0.5f, -0.5f }, { 1, 1 }, { 1, 0, 0 } },

        // -y face slice
        { { -0.5f, -0.5f,  0.5f }, { 0, 1 }, { 0, 1, 0 } },
        { { -0.5f, -0.5f, -0.5f }, { 0, 0 }, { 0, 1, 0 } },
        { {  0.5f, -0.5f, -0.5f }, { 1, 0 }, { 0, 1, 0 } },
        { {  0.5f, -0.5f,  0.5f }, { 1, 1 }, { 0, 1, 0 } },

        //  y face slice
        { { -0.5f, 0.5f,  0.5f }, { 0, 1 }, { 0, -1, 0 } },
        { { -0.5f, 0.5f, -0.5f }, { 0, 0 }, { 0, -1, 0 } },
        { {  0.5f, 0.5f, -0.5f }, { 1, 0 }, { 0, -1, 0 } },
        { {  0.5f, 0.5f,  0.5f }, { 1, 1 }, { 0, -1, 0 } },
    };

    unsigned int cube_indices[] = {
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

    create_gpu_buffer(vbo, app.d3d_device, cube_vertices, 24, sizeof(Vertex), D3D11_BIND_VERTEX_BUFFER);
    create_gpu_buffer(ibo, app.d3d_device, cube_indices, 36, sizeof(uint), D3D11_BIND_INDEX_BUFFER);
    
    Gpu_Shader vs, ps;
    ASSERT(compile_gpu_shader(vs, app.d3d_device, "src\\shaders\\static.hlsl", D3D11_SHVER_VERTEX_SHADER));
    ASSERT(compile_gpu_shader(ps, app.d3d_device, "src\\shaders\\lit.hlsl", D3D11_SHVER_PIXEL_SHADER));

    // @TODO: We're currently hardcoding light information on the pixel shader.
    //        We shouldn't be doing that.
    
    Transform cube_tf = Transform::zero();

    Transform light_tf = Transform::zero();
    light_tf.position = HMM_V3(-2, -2, 2);
    
    Camera camera;
    
    /// MAIN LOOP
    float timestep = 0;
    int64_t clock1 = get_clock();
    int64_t clock2 = 0;
    
    while (!app.quit)
    {
        app.pump_events();

        /// LOGIC
        {
            if (app.kbd[0][VK_ESCAPE])
                app.quit = true;

            camera.tick(timestep);
        }

        if (app.quit)
            break;

        /// RENDERING
        {
            if (app.resized)
                app.resize_pipeline_images();

            D3D11_VIEWPORT d3d_viewport = {};
            d3d_viewport.MaxDepth = 1;
            d3d_viewport.Width = (float)app.hwnd_size[0];
            d3d_viewport.Height = (float)app.hwnd_size[1];
            
            app.d3d_context->RSSetViewports(1, &d3d_viewport);
            app.d3d_context->RSSetState(app.pipeline.cw_rasterizer);
            app.d3d_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            
            // -- Programmable state.
            //float bg_color[4] = { 1, 182.0f/255.0f, 193.0f/255.0f, 1 }; // pink
            float bg_color[4] = { 0, 0, 0, 1 };
            app.d3d_context->ClearRenderTargetView(app.pipeline.backbuffer_view, bg_color);
            app.d3d_context->ClearDepthStencilView(app.pipeline.depthbuffer_view, D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL, 1, 0);
            app.d3d_context->OMSetRenderTargets(1, &app.pipeline.backbuffer_view, app.pipeline.depthbuffer_view);

            auto *vs_cb0 = (VS_CB0 *)(vs.cbuffers.data[0].data);
            auto *ps_cb0 = (PS_CB0 *)(ps.cbuffers.data[0].data);
            
            // vs
            vs_cb0->world_matrix = cube_tf.as_matrix();
            vs_cb0->view_matrix = camera.get_matrix();
            vs_cb0->proj_matrix = HMM_Perspective_LH_ZO(camera.fov,
                                                        d3d_viewport.Width / d3d_viewport.Height,
                                                        camera.view_plane_distance[0],
                                                        camera.view_plane_distance[1]);
            
            bind_gpu_shader(vs, app.d3d_context);
            
            // ps
            ps_cb0->view_pos = camera.position;
            ps_cb0->light_pos = light_tf.position;
            ps_cb0->inverse_transpose_world_matrix = HMM_InvGeneralM4(HMM_TransposeM4(vs_cb0->world_matrix));
            ps_cb0->use_lighting = true;
            bind_gpu_shader(ps, app.d3d_context);
            
            // -- Drawing
            bind_gpu_buffer(vbo, app.d3d_context);
            bind_gpu_buffer(ibo, app.d3d_context);
            app.d3d_context->DrawIndexed(ibo.element_count, 0, 0);

            // @TODO (Really just a @hack):
            // Add a little cube where the light is supposed to be.
            app.d3d_context->RSSetState(app.pipeline.wf_rasterizer);

            vs_cb0->world_matrix = HMM_MulM4(HMM_Translate(light_tf.position), HMM_Scale(HMM_V3(0.5f, 0.5f, 0.5f)));
            ps_cb0->use_lighting = false;
            bind_gpu_shader(ps, app.d3d_context);
            bind_gpu_shader(vs, app.d3d_context);

            app.d3d_context->DrawIndexed(ibo.element_count, 0, 0);
            
            // -- Finish
            app.dx_swapchain->Present(0, 0);
        }

        /// TIMING
        {
            clock2 = get_clock();
            timestep = (((float)(clock2 - clock1)) / (float)app.clock_freq);
            clock1 = clock2;
        }
    }

    release_gpu_buffer(ibo);
    release_gpu_buffer(vbo);
    release_gpu_shader(ps);
    release_gpu_shader(vs);

    app.release();
    LOG("No error.\n");
    return 0;
}
