#ifndef _CAMERA_H_
#define _CAMERA_H_
#include "stdafx.h"
#include "win32_application.h"
#include "HandmadeMath.h"

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
        orbit = HMM_V3(2, HMM_AngleDeg(-25.0f), 3);
        orbit_target = HMM_V3(0, 0, 0);
        orbit_speed = 5;
        view_plane_distance = HMM_V2(0.1f, 100.0f);
    }
    
    void tick(float timestep) {
        if (win32.input.mouse_buttons & (1 << 0)) {
            orbit[0] += -HMM_AngleDeg(win32.input.mouse_delta[0]);
            orbit[1] += HMM_AngleDeg(win32.input.mouse_delta[1]);
        }
        if (win32.input.mouse_buttons & (1 << 3)) {
            HMM_Vec3 view_vector = HMM_NormV3(HMM_SubV3(position, orbit_target));
            HMM_Vec3 strafe_vector = HMM_NormV3(HMM_Cross(view_vector, HMM_V3(0, 1, 0)));
            orbit_target = HMM_AddV3(orbit_target, HMM_MulV3F(strafe_vector, (-win32.input.mouse_delta[0] * orbit_speed * timestep)));
            orbit_target = HMM_AddV3(orbit_target, HMM_MulV3F(HMM_V3(0, 1, 0), (win32.input.mouse_delta[1] * orbit_speed * timestep)));
        }

        // -- Vertical movement
        if (win32.input.kbd[VK_SPACE])
            orbit_target[1] += orbit_speed * timestep;
        if (win32.input.kbd['C'])
            orbit_target[1] -= orbit_speed * timestep;
        // --

        // As the camera zooms farther away from the target, we want to speed up the rate of zoom.
        // In the inverse, this means that the closer the camera is to the target, the slower the rate of zoom.
        // This gives a much more natural feeling of zooming the camera.
        orbit[2] += (-(win32.input.mouse_wheel_delta) * (orbit[2] / 5.0f));

        position[0] = orbit_target[0] + orbit[2] * cosf(orbit[1]) * cosf(orbit[0]);
        position[1] = orbit_target[1] + orbit[2] * sinf(orbit[1]);
        position[2] = orbit_target[2] + orbit[2] * cosf(orbit[1]) * sinf(orbit[0]);
    }

    HMM_Mat4 get_matrix() {
        return HMM_LookAt_LH(position, orbit_target, HMM_V3(0, 1, 0)); 
    }
};

#endif 
