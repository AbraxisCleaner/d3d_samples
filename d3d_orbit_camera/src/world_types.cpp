#include "camera.h"
#include "Application.h"

void Camera::Tick(float timestep) {
    auto app = Application::instance;

    if (app->mouse_buttons[0] & (1 << 0)) {
        orbit[0] += -HMM_AngleDeg(app->mouse_delta[0]);
        orbit[1] += HMM_AngleDeg(app->mouse_delta[1]);
    }
    if (Application::instance->mouse_buttons[0] & 4) {
        HMM_Vec3 view_vector = HMM_NormV3(HMM_SubV3(position, target));
        HMM_Vec3 strafe_vector = HMM_NormV3(HMM_Cross(view_vector, HMM_V3(0, 1, 0)));
        target = HMM_AddV3(target, HMM_MulV3F(strafe_vector, ((-Application::instance->mouse_delta[0]) * speed * timestep)));
        target = HMM_AddV3(target, HMM_MulV3F(HMM_V3(0, 1, 0), ((Application::instance->mouse_delta[1]) * speed * timestep)));
    }
    if (Application::instance->kbd[0][VK_SPACE]) {
        target[1] += speed * timestep;
    }
    if (Application::instance->kbd[0]['C']) {
        target[1] -= speed * timestep;
    }
    
    orbit[2] += (-(Application::instance->mouse_wheel_delta[0] / 120)) * (orbit[2] / 5.0f);

    position[0] = target[0] + orbit[2] * cosf(orbit[1]) * cosf(orbit[0]);
    position[1] = target[1] + orbit[2] * sinf(orbit[1]);
    position[2] = target[2] + orbit[2] * cosf(orbit[1]) * sinf(orbit[0]);
}

HMM_Mat4 Camera::GetMatrix() { 
    return HMM_LookAt_LH(position, target, HMM_V3(0, 1, 0)); 
}