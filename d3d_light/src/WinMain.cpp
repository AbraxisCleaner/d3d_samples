#include "Application.cpp"

int main()
{
    Application app;
    app.init();

    /// MAIN LOOP
    float timestep = 0;
    int64_t clock1 = get_clock();
    int64_t clock2 = 0;

    while (!app.quit)
    {
        app.pump_events();

        /// LOGIC
        {
            if (app.kbd[VK_ESCAPE])
                app.quit = true;
        }

        /// RENDERING
        {
            if (app.resized)
                app.resize_pipeline_images();

            // -- Programmable state.
            float bg_color[4] = { 1, 182.0f/255.0f, 193.0f/255.0f, 1 };
            app.d3d_context->ClearRenderTargetView(app.pipeline.backbuffer_view, bg_color);
            app.d3d_context->ClearDepthStencilView(app.pipeline.depthbuffer_view, D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL, 1, 0);
            app.d3d_context->OMSetRenderTargets(1, &app.pipeline.backbuffer_view, app.pipeline.depthbuffer_view);

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

    app.release();
    return 0;
}
