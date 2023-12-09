#include "win32_application.h"
#include "d3d_types.h"

#define HANDMADE_MATH_USE_RADIANS
#include "HandmadeMath.h"

#include "camera.h"
//#include "obj_import.h"

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
    initialize_win32();
    initialize_d3d();
    
    //ASSERT(import_obj("data\\teapot.obj"));

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

    create_gpu_buffer(vbo, cube_vertices, 24, sizeof(Vertex), D3D11_BIND_VERTEX_BUFFER);
    create_gpu_buffer(ibo, cube_indices, 36, sizeof(uint), D3D11_BIND_INDEX_BUFFER);
    
    Gpu_Shader vs, ps;
    ASSERT(compile_gpu_shader(vs, "src\\shaders\\static.hlsl", D3D11_SHVER_VERTEX_SHADER));
    ASSERT(compile_gpu_shader(ps, "src\\shaders\\lit.hlsl", D3D11_SHVER_PIXEL_SHADER));

    Transform cube_tf = Transform::zero();
    Transform light_tf = Transform::zero();
    light_tf.position = HMM_V3(-2, -2, 2);
    
    Camera camera;

    int renderer_flags = 0; // 1 << 0: wireframe;
                            // 1 << 1: use_lighting;
    
    /// MAIN LOOP
    float timestep = 0;
    int64_t clock1 = get_clock();
    int64_t clock2 = 0;
    
    while (!win32.quit)
    {
        pump_events();

        /// LOGIC
        {
            if (get_key(VK_ESCAPE))
                win32.quit = true;
            
            if (get_key_down('T')) {
                if (renderer_flags & 1)
                    renderer_flags &= ~1;
                else
                    renderer_flags |= 1;
            }
            if (get_key_down('L')) {
                if (renderer_flags & 2)
                    renderer_flags &= ~2;
                else
                    renderer_flags |= 2;
            }
            
            camera.tick(timestep);
        }

        if (win32.quit)
            break;

        // RENDERING
        {
            if (win32.resized)
                LOG("TODO: Resizing.\n");

            D3D11_VIEWPORT d3d_viewport = {};
            d3d_viewport.MaxDepth = 1;
            d3d_viewport.Width = (float)win32.hwnd_size[0];
            d3d_viewport.Height = (float)win32.hwnd_size[1];
            d3d.context->RSSetViewports(1, &d3d_viewport);

            if (renderer_flags & 1)
                d3d.context->RSSetState(d3d.wf_rasterizer);
            else
                d3d.context->RSSetState(d3d.cw_rasterizer);
            
            float bg_color[4] = { 0, 0, 0, 1 };
            d3d.context->ClearRenderTargetView(d3d.backbuffer_view, bg_color);
            d3d.context->ClearDepthStencilView(d3d.depthbuffer_view, D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL, 1, 0);
            d3d.context->OMSetRenderTargets(1, &d3d.backbuffer_view, d3d.depthbuffer_view);
            d3d.context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            VS_CB0 *vs_cb = (VS_CB0 *)vs.cbuffers.data[0].data;
            PS_CB0 *ps_cb = (PS_CB0 *)ps.cbuffers.data[0].data;

            vs_cb->world_matrix = cube_tf.as_matrix();
            vs_cb->view_matrix = camera.get_matrix();
            vs_cb->proj_matrix = HMM_Perspective_LH_ZO(HMM_AngleDeg(camera.fov),
                                                                    (d3d_viewport.Width / d3d_viewport.Height),
                                                                    camera.view_plane_distance[0],
                                                                    camera.view_plane_distance[1]);

            ps_cb->view_pos = camera.position;
            ps_cb->light_pos = light_tf.position;
            ps_cb->inverse_transpose_world_matrix = HMM_InvGeneralM4(HMM_TransposeM4(vs_cb->world_matrix));
            ps_cb->use_lighting = false;
            if (renderer_flags & 2)
                ps_cb->use_lighting = true;

            bind_gpu_shader(vs);
            bind_gpu_shader(ps);
            bind_gpu_buffer(vbo);
            bind_gpu_buffer(ibo);
            d3d.context->DrawIndexed(ibo.element_count, 0, 0);

            // Light gizmo.
            d3d.context->RSSetState(d3d.wf_rasterizer);
            vs_cb->world_matrix = HMM_MulM4(HMM_Translate(light_tf.position), HMM_Scale(HMM_V3(0.5f, 0.5f, 0.5f)));
            ps_cb->use_lighting = false;
            bind_gpu_shader(vs);
            bind_gpu_shader(ps);
            d3d.context->DrawIndexed(ibo.element_count, 0, 0);
            
            // -- Finish
            d3d.swapchain->Present(0, 0);
        }

        /// TIMING
        {
            clock2 = get_clock();
            timestep = (((float)(clock2 - clock1)) / (float)win32.clock_freq);
            clock1 = clock2;
        }
    }

    release_gpu_shader(ps);
    release_gpu_shader(vs);
    release_gpu_buffer(ibo);
    release_gpu_buffer(vbo);
    
    release_d3d();
    release_win32();
    LOG("No error.\n");
    return 0;
}
