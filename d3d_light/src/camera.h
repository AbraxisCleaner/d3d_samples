#ifndef _CAMERA_H_
#define _CAMERA_H_
#include "stdafx.h"
#include "Application.h"

struct Camera {
    float    fov; // vertical fov
    HMM_Vec3 position;
    HMM_Vec3 orbit;
    HMM_Vec3 orbit_target;
    float    orbit_speed;
    HMM_Vec2 view_plane_distance;

    Camera() {
        fov = 80.0f;
        position = HMM_V3(0, 0, 0);
        orbit = HMM_V3(HMM_AngleDeg(45.0f), HMM_AngleDeg(45.0f), 3);
        orbit_target = HMM_V3(0, 0, 0);
        orbit_speed = 5;
        view_plane_distance = HMM_V2(0.1f, 100.0f);
    }
    
    void tick(float timestep) {
        auto app = Application::inst;

        if (app->mouse_buttons[0] & (1 << 0)) {
            orbit[0] += HMM_AngleDeg(app->mouse_delta[0]);
            orbit[1] += -HMM_AngleDeg(app->mouse_delta[1]);
        }
        if (app->mouse_buttons[0] & (1 << 3)) {
            HMM_Vec3 view_vector = HMM_NormV3(HMM_SubV3(position, orbit_target));
            HMM_Vec3 strafe_vector = HMM_NormV3(HMM_Cross(view_vector, HMM_V3(0, 1, 0)));
            orbit_target = HMM_AddV3(orbit_target, HMM_MulV3F(strafe_vector, (-app->mouse_delta[0] * orbit_speed * timestep)));
            orbit_target = HMM_AddV3(orbit_target, HMM_MulV3F(HMM_V3(0, 1, 0), (app->mouse_delta[1] * orbit_speed * timestep)));
        }

        // -- Vertical movement
        if (app->kbd[0][VK_SPACE])
            orbit_target[1] += orbit_speed * timestep;
        if (app->kbd[0]['C'])
            orbit_target[1] -= orbit_speed * timestep;
        // --

        // As the camera zooms farther away from the target, we want to speed up the rate of zoom.
        // In the inverse, this means that the closer the camera is to the target, the slower the rate of zoom.
        // This gives a much more natural feeling of zooming the camera.
        orbit[2] += (-(app->mouse_wheel_delta[0]) * (orbit[2] / 5.0f));

        position[0] = orbit_target[0] + orbit[2] * cosf(orbit[1]) * cosf(orbit[0]);
        position[1] = orbit_target[1] + orbit[2] * sinf(orbit[1]);
        position[2] = orbit_target[2] + orbit[2] * cosf(orbit[1]) * sinf(orbit[0]);
    }

    HMM_Mat4 get_matrix() {
        return HMM_LookAt_LH(position, orbit_target, HMM_V3(0, 1, 0)); 
    }
};

#endif 
